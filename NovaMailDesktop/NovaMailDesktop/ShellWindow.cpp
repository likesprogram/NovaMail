#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <windowsx.h>
#include <objidl.h>
#include <olectl.h>
#include <shlwapi.h>
#include <gdiplus.h>

#include "FallbackContentRenderer.h"
#include "ShellDebugLog.h"
#include "ShellWindow.h"
#include "VueDistResourceExtractor.h"
#include "WebViewHost.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cwctype>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "shlwapi.lib")

using Gdiplus::ARGB;
using Gdiplus::Bitmap;
using Gdiplus::BitmapData;
using Gdiplus::Brush;
using Gdiplus::Color;
using Gdiplus::Font;
using Gdiplus::FontCollection;
using Gdiplus::FontFamily;
using Gdiplus::FontStyleBold;
using Gdiplus::FontStyleRegular;
using Gdiplus::Graphics;
using Gdiplus::GraphicsPath;
using Gdiplus::GraphicsState;
using Gdiplus::ImageLockModeRead;
using Gdiplus::LineCapRound;
using Gdiplus::Ok;
using Gdiplus::Pen;
using Gdiplus::Rect;
using Gdiplus::RectF;
using Gdiplus::SmoothingModeAntiAlias;
using Gdiplus::SolidBrush;
using Gdiplus::StringAlignmentCenter;
using Gdiplus::StringFormat;
using Gdiplus::StringFormatFlagsNoWrap;
using Gdiplus::StringTrimmingEllipsisCharacter;
using Gdiplus::TextRenderingHintClearTypeGridFit;
using Gdiplus::PrivateFontCollection;

namespace {

constexpr wchar_t kWindowClassName[] = L"NovaMailDesktopShellWindow";
constexpr wchar_t kDefaultWindowTitle[] = L"Nova Mail";
constexpr int kIconFontResourceId = 101;
constexpr int kAppIconPngResourceId = 102;
constexpr UINT kShellMessageSetTitle = WM_APP + 0x40;
constexpr UINT kShellMessageNotify = WM_APP + 0x41;
constexpr UINT_PTR kNotificationToastTimer = 0x100;
constexpr wchar_t kNewMailLabel[] = L"\u65b0\u90ae\u4ef6";
constexpr wchar_t kNewMailNotificationTitle[] = L"\u65b0\u90ae\u4ef6\u901a\u77e5";
constexpr wchar_t kDefaultNewMailBody[] = L"\u4f60\u6536\u5230\u4e86\u4e00\u5c01\u65b0\u90ae\u4ef6\u3002";
constexpr wchar_t kIconMail[] = L"\ue69f";
constexpr wchar_t kIconMinimize[] = L"\ue68b";
constexpr wchar_t kIconMaximize[] = L"\ue68c";
constexpr wchar_t kIconRestore[] = L"\ue68d";
constexpr wchar_t kIconClose[] = L"\ue620";

enum class ShellButton {
    None,
    Minimize,
    Maximize,
    Close,
};

struct ShellLayout {
    RectF frame;
    RectF titlebar;
    RectF content;
    RectF icon;
    RectF title;
    RectF notificationPill;
    RectF minimizeButton;
    RectF maximizeButton;
    RectF closeButton;
    float cornerRadius = 0.0f;
    float shadowMargin = 0.0f;
};

struct ShellNotification {
    std::wstring title;
    std::wstring body;
    int unreadCount = 0;
    bool toastVisible = false;
};

struct ShellNotificationRequest {
    std::wstring title;
    std::wstring body;
    int unreadCount = 1;
};

struct ShellWindowMetrics {
    int initialWidth = 1044;
    int initialHeight = 744;
    int minWidth = 560;
    int minHeight = 440;
};

int RoundToInt(float value) {
    return static_cast<int>(std::lround(value));
}

float ClampFloat(float value, float low, float high) {
    if (value < low) {
        return low;
    }

    if (value > high) {
        return high;
    }

    return value;
}

int ClampInt(int value, int low, int high) {
    if (value < low) {
        return low;
    }

    if (value > high) {
        return high;
    }

    return value;
}

Color Rgba(BYTE red, BYTE green, BYTE blue, BYTE alpha = 255) {
    return Color(alpha, red, green, blue);
}

bool Contains(const RectF& rect, float x, float y) {
    return x >= rect.X && x < rect.X + rect.Width && y >= rect.Y &&
                  y < rect.Y + rect.Height;
}

void AddRoundedRect(GraphicsPath& path, const RectF& rect, float radius) {
    const float diameter = radius * 2.0f;

    if (diameter <= 0.0f) {
        path.AddRectangle(rect);
        path.CloseFigure();
        return;
    }

    path.AddArc(rect.X, rect.Y, diameter, diameter, 180.0f, 90.0f);
    path.AddArc(rect.X + rect.Width - diameter, rect.Y, diameter, diameter, 270.0f,
                            90.0f);
    path.AddArc(rect.X + rect.Width - diameter, rect.Y + rect.Height - diameter,
                            diameter, diameter, 0.0f, 90.0f);
    path.AddArc(rect.X, rect.Y + rect.Height - diameter, diameter, diameter, 90.0f,
                            90.0f);
    path.CloseFigure();
}

RectF InflateRect(const RectF& rect, float dx, float dy) {
    return RectF(rect.X - dx, rect.Y - dy, rect.Width + dx * 2.0f,
                              rect.Height + dy * 2.0f);
}

std::wstring TrimCopy(const std::wstring& text) {
    size_t first = 0;
    while (first < text.size() && std::iswspace(text[first])) {
        ++first;
    }

    size_t last = text.size();
    while (last > first && std::iswspace(text[last - 1])) {
        --last;
    }

    return text.substr(first, last - first);
}

int HexValue(wchar_t value) {
    if (value >= L'0' && value <= L'9') {
        return value - L'0';
    }

    if (value >= L'a' && value <= L'f') {
        return value - L'a' + 10;
    }

    if (value >= L'A' && value <= L'F') {
        return value - L'A' + 10;
    }

    return -1;
}

std::wstring DecodeJsonString(const std::wstring& value) {
    std::wstring decoded;
    decoded.reserve(value.size());

    for (size_t i = 0; i < value.size(); ++i) {
        if (value[i] != L'\\' || i + 1 >= value.size()) {
            decoded.push_back(value[i]);
            continue;
        }

        const wchar_t escape = value[++i];
        switch (escape) {
            case L'"':
            case L'\\':
            case L'/':
                decoded.push_back(escape);
                break;
            case L'b':
                decoded.push_back(L'\b');
                break;
            case L'f':
                decoded.push_back(L'\f');
                break;
            case L'n':
                decoded.push_back(L'\n');
                break;
            case L'r':
                decoded.push_back(L'\r');
                break;
            case L't':
                decoded.push_back(L'\t');
                break;
            case L'u': {
                if (i + 4 >= value.size()) {
                    decoded.push_back(escape);
                    break;
                }

                int code = 0;
                bool valid = true;
                for (int j = 0; j < 4; ++j) {
                    const int hex = HexValue(value[i + 1 + j]);
                    if (hex < 0) {
                        valid = false;
                        break;
                    }
                    code = (code << 4) | hex;
                }

                if (valid) {
                    decoded.push_back(static_cast<wchar_t>(code));
                    i += 4;
                } else {
                    decoded.push_back(escape);
                }
                break;
            }
            default:
                decoded.push_back(escape);
                break;
        }
    }

    return decoded;
}

bool ExtractJsonStringValue(const std::wstring& json, const std::wstring& key,
                                                        std::wstring* output) {
    const std::wstring quotedKey = L"\"" + key + L"\"";
    size_t keyPosition = json.find(quotedKey);
    if (keyPosition == std::wstring::npos) {
        return false;
    }

    size_t colonPosition = json.find(L':', keyPosition + quotedKey.size());
    if (colonPosition == std::wstring::npos) {
        return false;
    }

    size_t valueStart = colonPosition + 1;
    while (valueStart < json.size() && std::iswspace(json[valueStart])) {
        ++valueStart;
    }

    if (valueStart >= json.size() || json[valueStart] != L'"') {
        return false;
    }

    std::wstring rawValue;
    bool escaped = false;
    for (size_t i = valueStart + 1; i < json.size(); ++i) {
        const wchar_t ch = json[i];
        if (escaped) {
            rawValue.push_back(L'\\');
            rawValue.push_back(ch);
            escaped = false;
            continue;
        }

        if (ch == L'\\') {
            escaped = true;
            continue;
        }

        if (ch == L'"') {
            *output = DecodeJsonString(rawValue);
            return true;
        }

        rawValue.push_back(ch);
    }

    return false;
}

bool ExtractJsonIntValue(const std::wstring& json, const std::wstring& key,
                                                  int* output) {
    const std::wstring quotedKey = L"\"" + key + L"\"";
    size_t keyPosition = json.find(quotedKey);
    if (keyPosition == std::wstring::npos) {
        return false;
    }

    size_t colonPosition = json.find(L':', keyPosition + quotedKey.size());
    if (colonPosition == std::wstring::npos) {
        return false;
    }

    size_t valueStart = colonPosition + 1;
    while (valueStart < json.size() && std::iswspace(json[valueStart])) {
        ++valueStart;
    }

    size_t valueEnd = valueStart;
    if (valueEnd < json.size() && json[valueEnd] == L'-') {
        ++valueEnd;
    }

    while (valueEnd < json.size() && json[valueEnd] >= L'0' &&
                  json[valueEnd] <= L'9') {
        ++valueEnd;
    }

    if (valueEnd == valueStart) {
        return false;
    }

    *output = std::stoi(json.substr(valueStart, valueEnd - valueStart));
    return true;
}

bool HasMessageType(const std::wstring& json, const wchar_t* type) {
    return json.find(L"\"type\"") != std::wstring::npos &&
                  json.find(type) != std::wstring::npos;
}

std::wstring FormatUnreadCount(int unreadCount) {
    return unreadCount > 99 ? L"99+" : std::to_wstring(std::max(0, unreadCount));
}

void FillRoundedRect(Graphics& graphics, const RectF& rect, float radius,
                                          const Color& color) {
    SolidBrush brush(color);
    GraphicsPath path;
    AddRoundedRect(path, rect, radius);
    graphics.FillPath(&brush, &path);
}

void StrokeRoundedRect(Graphics& graphics, const RectF& rect, float radius,
                                              const Color& color, float width) {
    Pen pen(color, width);
    GraphicsPath path;
    AddRoundedRect(path, rect, radius);
    graphics.DrawPath(&pen, &path);
}

}  // namespace

class ShellWindow::Impl {
public:
    Impl() = default;

    ~Impl() {
        if (appIcon_) {
            DestroyIcon(appIcon_);
            appIcon_ = nullptr;
        }
    }

    bool Create(HINSTANCE instance) {
        instance_ = instance;
        shellTitle_ = kDefaultWindowTitle;
        LoadIconFont();
        LoadAppIcon();

        WNDCLASSEXW windowClass = {};
        windowClass.cbSize = sizeof(windowClass);
        windowClass.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
        windowClass.lpfnWndProc = ShellWindow::Impl::WindowProc;
        windowClass.hInstance = instance_;
        windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        windowClass.hIcon = appIcon_;
        windowClass.hIconSm = appIcon_;
        windowClass.lpszClassName = kWindowClassName;

        if (!RegisterClassExW(&windowClass)) {
            return false;
        }

        const int width = Scale(static_cast<float>(metrics_.initialWidth));
        const int height = Scale(static_cast<float>(metrics_.initialHeight));

        RECT workArea = {};
        SystemParametersInfoW(SPI_GETWORKAREA, 0, &workArea, 0);
        const int left = workArea.left + ((workArea.right - workArea.left) - width) / 2;
        const int top = workArea.top + ((workArea.bottom - workArea.top) - height) / 2;

        hwnd_ = CreateWindowExW(
                WS_EX_APPWINDOW | WS_EX_LAYERED, kWindowClassName, shellTitle_.c_str(),
                WS_POPUP | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX, left, top,
                width, height, nullptr, nullptr, instance_, this);
        if (!hwnd_) {
            WriteShellDebugLog(L"CreateWindowExW failed: " + std::to_wstring(GetLastError()));
        } else {
            WriteShellDebugLog(L"CreateWindowExW succeeded");
        }

        if (hwnd_ && appIcon_) {
            SendMessageW(hwnd_, WM_SETICON, ICON_BIG,
                                      reinterpret_cast<LPARAM>(appIcon_));
            SendMessageW(hwnd_, WM_SETICON, ICON_SMALL,
                                      reinterpret_cast<LPARAM>(appIcon_));
        }

        if (hwnd_) {
            SetWindowPos(hwnd_, nullptr, 0, 0, 0, 0,
                                      SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE |
                                              SWP_NOZORDER | SWP_NOACTIVATE);
            InitializeWebView();
        }

        return hwnd_ != nullptr;
    }

    void InitializeWebView() {
        VueDistResourceExtractor extractor(instance_);
        if (!extractor.Extract()) {
            WriteShellDebugLog(L"vue dist extraction failed");
            return;
        }

        WriteShellDebugLog(L"vue dist extracted: " + extractor.OutputRoot());
        webViewHost_.Initialize(
            hwnd_, extractor.OutputRoot(), extractor.EntryFileName(),
            [this](const std::wstring& message) {
                HandleVueBridgeMessage(message);
            });
    }

    void LoadIconFont() {
        HRSRC resource =
                FindResourceW(instance_, MAKEINTRESOURCEW(kIconFontResourceId), RT_RCDATA);
        if (!resource) {
            return;
        }

        HGLOBAL loadedResource = LoadResource(instance_, resource);
        if (!loadedResource) {
            return;
        }

        const void* data = LockResource(loadedResource);
        const DWORD size = SizeofResource(instance_, resource);
        if (!data || size == 0) {
            return;
        }

        iconFontBuffer_.assign(static_cast<const BYTE*>(data),
                                                      static_cast<const BYTE*>(data) + size);
        if (iconFontCollection_.AddMemoryFont(iconFontBuffer_.data(),
                                                                                    static_cast<INT>(iconFontBuffer_.size())) != Ok) {
            iconFontBuffer_.clear();
        }
    }

    void LoadAppIcon() {
        HRSRC resource = FindResourceW(instance_, MAKEINTRESOURCEW(kAppIconPngResourceId),
                                                                      RT_RCDATA);
        if (!resource) {
            return;
        }

        HGLOBAL loadedResource = LoadResource(instance_, resource);
        if (!loadedResource) {
            return;
        }

        const void* data = LockResource(loadedResource);
        const DWORD size = SizeofResource(instance_, resource);
        if (!data || size == 0) {
            return;
        }

        appIconBuffer_.assign(static_cast<const BYTE*>(data),
                                                    static_cast<const BYTE*>(data) + size);

        IStream* stream = SHCreateMemStream(appIconBuffer_.data(),
                                                                                static_cast<UINT>(appIconBuffer_.size()));
        if (!stream) {
            appIconBuffer_.clear();
            return;
        }

        auto iconBitmap = std::make_unique<Bitmap>(stream);
        stream->Release();
        if (iconBitmap->GetLastStatus() == Ok) {
            HICON icon = nullptr;
            if (iconBitmap->GetHICON(&icon) == Ok && icon) {
                appIcon_ = icon;
            }
            appIconBitmap_ = std::move(iconBitmap);
        } else {
            appIconBuffer_.clear();
            appIconBitmap_.reset();
        }
    }

    std::unique_ptr<Font> CreateIconFont(float size, int style = FontStyleRegular) const {
        if (!iconFontBuffer_.empty()) {
            INT familyCount = iconFontCollection_.GetFamilyCount();
            if (familyCount > 0) {
                std::vector<FontFamily> families(static_cast<size_t>(familyCount));
                INT found = 0;
                if (iconFontCollection_.GetFamilies(familyCount, families.data(), &found) == Ok &&
                        found > 0) {
                    return std::make_unique<Font>(&families[0], size, style,
                                                                                Gdiplus::UnitPixel);
                }
            }
        }

        FontFamily fallbackFamily(L"Segoe UI Symbol");
        return std::make_unique<Font>(&fallbackFamily, size, style, Gdiplus::UnitPixel);
    }

    void DrawIconText(Graphics& graphics, const wchar_t* glyph, const RectF& rect,
                                        float size, const Color& color) const {
        if (!glyph || glyph[0] == L'\0') {
            return;
        }

        auto font = CreateIconFont(size);
        SolidBrush brush(color);
        StringFormat format;
        format.SetAlignment(StringAlignmentCenter);
        format.SetLineAlignment(StringAlignmentCenter);
        format.SetFormatFlags(StringFormatFlagsNoWrap);
        graphics.DrawString(glyph, -1, font.get(), rect, &format, &brush);
    }

    void SetShellTitle(std::wstring title) {
        title = TrimCopy(title);
        if (title.empty()) {
            title = kDefaultWindowTitle;
        }

        shellTitle_ = std::move(title);
        WriteShellDebugLog(L"shell title set: " + shellTitle_);
        if (hwnd_) {
            SetWindowTextW(hwnd_, shellTitle_.c_str());
            Render();
        }
    }

    void ShowShellNotification(std::wstring title, std::wstring body,
                                                          int unreadCount) {
        notification_.title = TrimCopy(title);
        notification_.body = TrimCopy(body);
        notification_.unreadCount = std::max(0, unreadCount);
        notification_.toastVisible =
                !notification_.title.empty() || !notification_.body.empty();

        if (notification_.toastVisible && notification_.unreadCount == 0) {
            notification_.unreadCount = 1;
        }

        if (hwnd_) {
            FlashWindow(hwnd_, TRUE);
            KillTimer(hwnd_, kNotificationToastTimer);
            if (notification_.toastVisible) {
                SetTimer(hwnd_, kNotificationToastTimer, 5200, nullptr);
            }
            Render();
        }
    }

    void SetWindowMetrics(ShellWindowMetrics metrics, bool applyInitialSize) {
        constexpr int kMinimumUsableWidth = 360;
        constexpr int kMinimumUsableHeight = 260;

        RECT workArea = {};
        SystemParametersInfoW(SPI_GETWORKAREA, 0, &workArea, 0);
        const int workAreaWidth = static_cast<int>(workArea.right - workArea.left);
        const int workAreaHeight = static_cast<int>(workArea.bottom - workArea.top);
        const int maxPixelWidth = std::max(kMinimumUsableWidth, workAreaWidth);
        const int maxPixelHeight = std::max(kMinimumUsableHeight, workAreaHeight);
        const int maxLogicalWidth =
                std::max(kMinimumUsableWidth,
                         RoundToInt(static_cast<float>(maxPixelWidth) / dpi_));
        const int maxLogicalHeight =
                std::max(kMinimumUsableHeight,
                         RoundToInt(static_cast<float>(maxPixelHeight) / dpi_));

        metrics.minWidth = std::max(kMinimumUsableWidth, metrics.minWidth);
        metrics.minHeight = std::max(kMinimumUsableHeight, metrics.minHeight);
        metrics.minWidth = std::min(metrics.minWidth, maxLogicalWidth);
        metrics.minHeight = std::min(metrics.minHeight, maxLogicalHeight);
        metrics.initialWidth = std::max(metrics.minWidth, metrics.initialWidth);
        metrics.initialHeight = std::max(metrics.minHeight, metrics.initialHeight);
        metrics.initialWidth = std::min(metrics.initialWidth, maxLogicalWidth);
        metrics.initialHeight = std::min(metrics.initialHeight, maxLogicalHeight);
        metrics_ = metrics;

        if (!hwnd_) {
            return;
        }

        if (applyInitialSize && !IsZoomed(hwnd_)) {
            const int width = ClampInt(Scale(static_cast<float>(metrics_.initialWidth)),
                                       Scale(static_cast<float>(kMinimumUsableWidth)),
                                       maxPixelWidth);
            const int height = ClampInt(Scale(static_cast<float>(metrics_.initialHeight)),
                                        Scale(static_cast<float>(kMinimumUsableHeight)),
                                        maxPixelHeight);
            const int left = workArea.left + ((workArea.right - workArea.left) - width) / 2;
            const int top = workArea.top + ((workArea.bottom - workArea.top) - height) / 2;
            SetWindowPos(hwnd_, nullptr, left, top, width, height,
                                      SWP_NOZORDER | SWP_NOACTIVATE);
        }

        WriteShellDebugLog(L"shell metrics set: " +
                           std::to_wstring(metrics_.initialWidth) + L"x" +
                           std::to_wstring(metrics_.initialHeight) + L" min " +
                           std::to_wstring(metrics_.minWidth) + L"x" +
                           std::to_wstring(metrics_.minHeight));
        Render();
    }

    void HandleVueBridgeMessage(const std::wstring& json) {
        if (HasMessageType(json, L"nova-mail:ready") ||
                HasMessageType(json, L"nova-mail:shell:set-title")) {
            std::wstring title;
            if (ExtractJsonStringValue(json, L"title", &title)) {
                SetShellTitle(title);
            }
        }

        if (HasMessageType(json, L"nova-mail:ready") ||
                HasMessageType(json, L"nova-mail:shell:set-window-metrics")) {
            ShellWindowMetrics metrics = metrics_;
            bool hasMetrics = false;
            const bool hasInitialWidth =
                    ExtractJsonIntValue(json, L"initialWidth", &metrics.initialWidth);
            const bool hasInitialHeight =
                    ExtractJsonIntValue(json, L"initialHeight", &metrics.initialHeight);
            hasMetrics |= hasInitialWidth;
            hasMetrics |= hasInitialHeight;
            hasMetrics |= ExtractJsonIntValue(json, L"minWidth", &metrics.minWidth);
            hasMetrics |= ExtractJsonIntValue(json, L"minHeight", &metrics.minHeight);
            if (hasMetrics) {
                SetWindowMetrics(metrics, hasInitialWidth || hasInitialHeight);
            }

            if (HasMessageType(json, L"nova-mail:ready")) {
                return;
            }
        }

        if (HasMessageType(json, L"nova-mail:shell:notify")) {
            ShellNotificationRequest request;
            ExtractJsonStringValue(json, L"title", &request.title);
            ExtractJsonStringValue(json, L"body", &request.body);
            ExtractJsonIntValue(json, L"unreadCount", &request.unreadCount);
            ShowShellNotification(request.title, request.body, request.unreadCount);
            return;
        }

        if (HasMessageType(json, L"nova-mail:window-control")) {
            std::wstring action;
            if (!ExtractJsonStringValue(json, L"action", &action)) {
                return;
            }

            if (action == L"minimize") {
                ExecuteButton(ShellButton::Minimize);
            } else if (action == L"maximize") {
                ExecuteButton(ShellButton::Maximize);
            } else if (action == L"close") {
                ExecuteButton(ShellButton::Close);
            }
            return;
        }
    }

    void Show(int commandShow) {
        if (commandShow == SW_HIDE) {
            commandShow = SW_SHOWNORMAL;
        }

        WriteShellDebugLog(L"shell show: " + std::to_wstring(commandShow));
        ShowWindow(hwnd_, commandShow);
        UpdateWindow(hwnd_);
        Render();
    }

private:
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam,
                                                                          LPARAM lParam) {
        ShellWindow::Impl* window = nullptr;

        if (message == WM_NCCREATE) {
            auto* createStruct = reinterpret_cast<CREATESTRUCTW*>(lParam);
            window = static_cast<ShellWindow::Impl*>(createStruct->lpCreateParams);
            window->hwnd_ = hwnd;
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
        } else {
            window = reinterpret_cast<ShellWindow::Impl*>(
                    GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        }

        if (window) {
            return window->HandleMessage(message, wParam, lParam);
        }

        return DefWindowProcW(hwnd, message, wParam, lParam);
    }

    LRESULT HandleMessage(UINT message, WPARAM wParam, LPARAM lParam) {
        switch (message) {
            case WM_CREATE:
                dpi_ = static_cast<float>(GetDpiForWindow(hwnd_)) / 96.0f;
                Render();
                LayoutWebView();
                return 0;

            case WM_DPICHANGED:
                dpi_ = HIWORD(wParam) / 96.0f;
                if (auto* suggestedRect = reinterpret_cast<RECT*>(lParam)) {
                    SetWindowPos(hwnd_, nullptr, suggestedRect->left, suggestedRect->top,
                                              suggestedRect->right - suggestedRect->left,
                                              suggestedRect->bottom - suggestedRect->top,
                                              SWP_NOZORDER | SWP_NOACTIVATE);
                }
                Render();
                return 0;

            case WM_NCCALCSIZE:
                if (wParam) {
                    return 0;
                }
                break;

            case WM_NCHITTEST:
                return OnHitTest(lParam);

            case WM_GETMINMAXINFO:
                OnGetMinMaxInfo(reinterpret_cast<MINMAXINFO*>(lParam));
                return 0;

            case WM_SIZE:
                if (wParam != SIZE_MINIMIZED) {
                    hoverButton_ = ShellButton::None;
                    pressedButton_ = ShellButton::None;
                    Render();
                    LayoutWebView();
                }
                return 0;

            case WM_MOUSEMOVE:
                OnMouseMove(lParam);
                return 0;

            case WM_MOUSELEAVE:
                trackingMouse_ = false;
                if (hoverButton_ != ShellButton::None || pressedButton_ == ShellButton::None) {
                    hoverButton_ = ShellButton::None;
                    Render();
                }
                return 0;

            case WM_LBUTTONDOWN:
                OnLeftButtonDown(lParam);
                return 0;

            case WM_LBUTTONUP:
                OnLeftButtonUp(lParam);
                return 0;

            case WM_CAPTURECHANGED:
                if (reinterpret_cast<HWND>(lParam) != hwnd_) {
                    pressedButton_ = ShellButton::None;
                    Render();
                }
                return 0;

            case WM_DESTROY:
                KillTimer(hwnd_, kNotificationToastTimer);
                SetWindowLongPtrW(hwnd_, GWLP_USERDATA, 0);
                hwnd_ = nullptr;
                PostQuitMessage(0);
                return 0;

            case WM_TIMER:
                if (wParam == kNotificationToastTimer) {
                    KillTimer(hwnd_, kNotificationToastTimer);
                    notification_.toastVisible = false;
                    Render();
                    return 0;
                }
                break;

            case kShellMessageSetTitle:
                if (auto* title = reinterpret_cast<std::wstring*>(lParam)) {
                    SetShellTitle(*title);
                    delete title;
                }
                return 0;

            case kShellMessageNotify:
                if (auto* request = reinterpret_cast<ShellNotificationRequest*>(lParam)) {
                    ShowShellNotification(request->title, request->body,
                                                                request->unreadCount);
                    delete request;
                }
                return 0;
        }

        return DefWindowProcW(hwnd_, message, wParam, lParam);
    }

    int Scale(float value) const {
        return RoundToInt(value * dpi_);
    }

    float ScaleF(float value) const {
        return value * dpi_;
    }

    ShellLayout MakeLayout(int width, int height) const {
        ShellLayout layout;
        const bool maximized = IsZoomed(hwnd_) != FALSE;
        layout.shadowMargin = maximized ? 0.0f : ScaleF(16.0f);
        layout.cornerRadius = maximized ? 0.0f : ScaleF(10.0f);

        layout.frame = RectF(layout.shadowMargin, layout.shadowMargin,
                                                  static_cast<float>(width) - layout.shadowMargin * 2.0f,
                                                  static_cast<float>(height) - layout.shadowMargin * 2.0f);

        if (maximized) {
            layout.frame = RectF(0.0f, 0.0f, static_cast<float>(width),
                                                      static_cast<float>(height));
        }

        const float titlebarHeight = ScaleF(48.0f);
        layout.titlebar = RectF(layout.frame.X, layout.frame.Y, layout.frame.Width,
                                                        titlebarHeight);
        layout.content =
                RectF(layout.frame.X, layout.frame.Y + titlebarHeight, layout.frame.Width,
                            layout.frame.Height - titlebarHeight);

        const float iconSize = ScaleF(24.0f);
        layout.icon = RectF(layout.frame.X + ScaleF(16.0f),
                                                layout.frame.Y + (titlebarHeight - iconSize) / 2.0f,
                                                iconSize, iconSize);

        const float buttonWidth = ScaleF(42.0f);
        const float buttonHeight = ScaleF(32.0f);
        const float buttonTop = layout.frame.Y + (titlebarHeight - buttonHeight) / 2.0f;
        const float buttonRight = layout.frame.X + layout.frame.Width - ScaleF(8.0f);

        layout.closeButton =
                RectF(buttonRight - buttonWidth, buttonTop, buttonWidth, buttonHeight);
        layout.maximizeButton =
                RectF(layout.closeButton.X - ScaleF(2.0f) - buttonWidth, buttonTop,
                            buttonWidth, buttonHeight);
        layout.minimizeButton =
                RectF(layout.maximizeButton.X - ScaleF(2.0f) - buttonWidth, buttonTop,
                            buttonWidth, buttonHeight);

        if (notification_.unreadCount > 0) {
            const bool compactNotification = layout.frame.Width < ScaleF(760.0f);
            const float notificationWidth =
                    compactNotification ? ScaleF(30.0f) : ScaleF(92.0f);
            layout.notificationPill =
                    RectF(layout.minimizeButton.X - ScaleF(12.0f) - notificationWidth,
                                layout.frame.Y + (titlebarHeight - ScaleF(24.0f)) / 2.0f,
                                notificationWidth, ScaleF(24.0f));
        }

        const float titleLeft = layout.icon.X + layout.icon.Width + ScaleF(10.0f);
        const float titleRight =
                notification_.unreadCount > 0 ? layout.notificationPill.X
                                                                            : layout.minimizeButton.X;
        layout.title =
                RectF(titleLeft, layout.titlebar.Y,
                            std::max(0.0f, titleRight - ScaleF(16.0f) - titleLeft),
                            titlebarHeight);

        return layout;
    }

    ShellButton ButtonFromPoint(float x, float y) const {
        RECT clientRect = {};
        GetClientRect(hwnd_, &clientRect);
        const ShellLayout layout =
                MakeLayout(clientRect.right - clientRect.left, clientRect.bottom - clientRect.top);

        if (Contains(layout.minimizeButton, x, y)) {
            return ShellButton::Minimize;
        }

        if (Contains(layout.maximizeButton, x, y)) {
            return ShellButton::Maximize;
        }

        if (Contains(layout.closeButton, x, y)) {
            return ShellButton::Close;
        }

        return ShellButton::None;
    }

    LRESULT OnHitTest(LPARAM lParam) const {
        POINT screenPoint = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
        RECT windowRect = {};
        GetWindowRect(hwnd_, &windowRect);
        const float x = static_cast<float>(screenPoint.x - windowRect.left);
        const float y = static_cast<float>(screenPoint.y - windowRect.top);
        const int width = windowRect.right - windowRect.left;
        const int height = windowRect.bottom - windowRect.top;
        const ShellLayout layout = MakeLayout(width, height);

        if (ButtonFromPoint(x, y) != ShellButton::None) {
            return HTCLIENT;
        }

        if (!IsZoomed(hwnd_)) {
            const float resizeMargin = ScaleF(8.0f);
            const bool left = std::abs(x - layout.frame.X) <= resizeMargin;
            const bool right =
                    std::abs(x - (layout.frame.X + layout.frame.Width)) <= resizeMargin;
            const bool top = std::abs(y - layout.frame.Y) <= resizeMargin;
            const bool bottom =
                    std::abs(y - (layout.frame.Y + layout.frame.Height)) <= resizeMargin;

            if (top && left) {
                return HTTOPLEFT;
            }
            if (top && right) {
                return HTTOPRIGHT;
            }
            if (bottom && left) {
                return HTBOTTOMLEFT;
            }
            if (bottom && right) {
                return HTBOTTOMRIGHT;
            }
            if (left) {
                return HTLEFT;
            }
            if (right) {
                return HTRIGHT;
            }
            if (top) {
                return HTTOP;
            }
            if (bottom) {
                return HTBOTTOM;
            }
        }

        if (Contains(layout.titlebar, x, y)) {
            return HTCAPTION;
        }

        return HTCLIENT;
    }

    void OnGetMinMaxInfo(MINMAXINFO* minMaxInfo) const {
        if (!minMaxInfo) {
            return;
        }

        minMaxInfo->ptMinTrackSize.x = Scale(static_cast<float>(metrics_.minWidth));
        minMaxInfo->ptMinTrackSize.y = Scale(static_cast<float>(metrics_.minHeight));
    }

    void StartMouseTracking() {
        if (trackingMouse_) {
            return;
        }

        TRACKMOUSEEVENT event = {};
        event.cbSize = sizeof(event);
        event.dwFlags = TME_LEAVE;
        event.hwndTrack = hwnd_;
        if (TrackMouseEvent(&event)) {
            trackingMouse_ = true;
        }
    }

    void OnMouseMove(LPARAM lParam) {
        StartMouseTracking();
        const float x = static_cast<float>(GET_X_LPARAM(lParam));
        const float y = static_cast<float>(GET_Y_LPARAM(lParam));
        const ShellButton nextHover = ButtonFromPoint(x, y);

        if (nextHover != hoverButton_) {
            hoverButton_ = nextHover;
            Render();
        }
    }

    void OnLeftButtonDown(LPARAM lParam) {
        const float x = static_cast<float>(GET_X_LPARAM(lParam));
        const float y = static_cast<float>(GET_Y_LPARAM(lParam));
        pressedButton_ = ButtonFromPoint(x, y);

        if (pressedButton_ != ShellButton::None) {
            SetCapture(hwnd_);
            Render();
        }
    }

    void OnLeftButtonUp(LPARAM lParam) {
        const ShellButton pressed = pressedButton_;
        pressedButton_ = ShellButton::None;

        if (GetCapture() == hwnd_) {
            ReleaseCapture();
        }

        const float x = static_cast<float>(GET_X_LPARAM(lParam));
        const float y = static_cast<float>(GET_Y_LPARAM(lParam));
        const ShellButton releasedOver = ButtonFromPoint(x, y);
        hoverButton_ = releasedOver;
        Render();

        if (pressed != ShellButton::None && pressed == releasedOver) {
            ExecuteButton(pressed);
        }
    }

    void ExecuteButton(ShellButton button) {
        switch (button) {
            case ShellButton::Minimize:
                ShowWindow(hwnd_, SW_MINIMIZE);
                break;

            case ShellButton::Maximize:
                ShowWindow(hwnd_, IsZoomed(hwnd_) ? SW_RESTORE : SW_MAXIMIZE);
                Render();
                break;

            case ShellButton::Close:
                PostMessageW(hwnd_, WM_CLOSE, 0, 0);
                break;

            case ShellButton::None:
                break;
        }
    }

    void LayoutWebView() {
        if (!hwnd_) {
            return;
        }

        RECT clientRect = {};
        GetClientRect(hwnd_, &clientRect);
        const ShellLayout layout =
            MakeLayout(clientRect.right - clientRect.left,
                       clientRect.bottom - clientRect.top);
        const int leftBorderInset = std::max(1, Scale(1.0f));
        RECT contentRect = {
            RoundToInt(layout.content.X) + leftBorderInset,
            RoundToInt(layout.content.Y),
            RoundToInt(layout.content.X + layout.content.Width),
            RoundToInt(layout.content.Y + layout.content.Height),
        };
        webViewHost_.Resize(contentRect, RoundToInt(layout.cornerRadius));
    }

    void DrawShadow(Graphics& graphics, const ShellLayout& layout) const {
        if (layout.shadowMargin <= 0.0f) {
            return;
        }

        const int ambientLayers = Scale(24.0f);
        for (int i = ambientLayers; i >= 1; --i) {
            const float t = 1.0f - static_cast<float>(i) / ambientLayers;
            const BYTE alpha =
                    static_cast<BYTE>(ClampFloat(9.0f * t * t, 0.0f, 9.0f));
            const float spread = i * 0.58f;
            RectF rect = InflateRect(layout.frame, spread, spread);
            FillRoundedRect(graphics, rect, layout.cornerRadius + i * 0.28f,
                                            Rgba(22, 31, 49, alpha));
        }

        const int keyLayers = Scale(12.0f);
        for (int i = keyLayers; i >= 1; --i) {
            const float t = 1.0f - static_cast<float>(i) / keyLayers;
            const BYTE alpha =
                    static_cast<BYTE>(ClampFloat(12.0f * t * t, 0.0f, 12.0f));
            const float spread = i * 0.30f;
            RectF rect = InflateRect(layout.frame, spread, spread);
            FillRoundedRect(graphics, rect, layout.cornerRadius + i * 0.18f,
                                            Rgba(18, 28, 45, alpha));
        }
    }

    void DrawTitleIcon(Graphics& graphics, const RectF& iconRect) const {
        if (appIconBitmap_) {
            graphics.DrawImage(appIconBitmap_.get(), iconRect);
            return;
        }

        FillRoundedRect(graphics, iconRect, ScaleF(7.0f), Rgba(36, 87, 214));
        DrawIconText(graphics, kIconMail, iconRect, ScaleF(15.0f),
                                  Rgba(255, 255, 255));
    }

    void DrawNotificationPill(Graphics& graphics, const RectF& rect) const {
        if (notification_.unreadCount <= 0 || rect.Width <= 0.0f) {
            return;
        }

        FillRoundedRect(graphics, rect, rect.Height / 2.0f, Rgba(235, 244, 255));
        StrokeRoundedRect(graphics, rect, rect.Height / 2.0f, Rgba(36, 87, 214, 42),
                                            1.0f);

        FontFamily fontFamily(L"Segoe UI");
        Font font(&fontFamily, ScaleF(12.0f), FontStyleBold, Gdiplus::UnitPixel);
        SolidBrush textBrush(Rgba(36, 87, 214));
        StringFormat format;
        format.SetAlignment(StringAlignmentCenter);
        format.SetLineAlignment(StringAlignmentCenter);
        format.SetTrimming(StringTrimmingEllipsisCharacter);
        format.SetFormatFlags(StringFormatFlagsNoWrap);

        std::wstring label;
        if (rect.Width < ScaleF(48.0f)) {
            label = FormatUnreadCount(notification_.unreadCount);
        } else {
            label = kNewMailLabel;
            label += L" ";
            label += FormatUnreadCount(notification_.unreadCount);
        }

        graphics.DrawString(label.c_str(), -1, &font, rect, &format, &textBrush);
    }

    void DrawButtonIcon(Graphics& graphics, ShellButton button, const RectF& rect,
                                            const Color& color) const {
        const wchar_t* glyph = L"";
        float size = ScaleF(15.0f);

        switch (button) {
            case ShellButton::Minimize:
                glyph = kIconMinimize;
                size = ScaleF(17.0f);
                break;

            case ShellButton::Maximize:
                glyph = IsZoomed(hwnd_) ? kIconRestore : kIconMaximize;
                size = ScaleF(17.0f);
                break;

            case ShellButton::Close:
                glyph = kIconClose;
                size = ScaleF(19.0f);
                break;

            case ShellButton::None:
                break;
        }

        DrawIconText(graphics, glyph, rect, size, color);
    }

    void DrawButton(Graphics& graphics, ShellButton button, const RectF& rect) const {
        const bool hovered = hoverButton_ == button;
        const bool pressed = pressedButton_ == button;
        const bool closeButton = button == ShellButton::Close;

        if (hovered || pressed) {
            const Color background =
                    closeButton ? (pressed ? Rgba(207, 16, 32) : Rgba(232, 17, 35))
                                            : (pressed ? Rgba(228, 234, 242) : Rgba(238, 242, 247));
            FillRoundedRect(graphics, rect, ScaleF(6.0f), background);
        }

        DrawButtonIcon(graphics, button, rect,
                                      closeButton && hovered ? Rgba(255, 255, 255)
                                                                                      : Rgba(38, 53, 77));
    }

    void DrawNotificationToast(Graphics& graphics, const ShellLayout& layout) const {
        if (!notification_.toastVisible) {
            return;
        }

        const float width = std::min(ScaleF(360.0f), layout.content.Width - ScaleF(32.0f));
        if (width <= ScaleF(180.0f)) {
            return;
        }

        const RectF toast(layout.content.X + layout.content.Width - width - ScaleF(18.0f),
                                            layout.content.Y + ScaleF(16.0f), width, ScaleF(82.0f));
        const RectF shadow = InflateRect(toast, ScaleF(2.0f), ScaleF(2.0f));
        FillRoundedRect(graphics, shadow, ScaleF(12.0f), Rgba(18, 28, 45, 26));
        FillRoundedRect(graphics, toast, ScaleF(10.0f), Rgba(255, 255, 255, 246));
        StrokeRoundedRect(graphics, toast, ScaleF(10.0f), Rgba(24, 35, 55, 28), 1.0f);

        RectF iconRect(toast.X + ScaleF(14.0f), toast.Y + ScaleF(17.0f),
                                      ScaleF(32.0f), ScaleF(32.0f));
        DrawTitleIcon(graphics, iconRect);

        FontFamily fontFamily(L"Segoe UI");
        Font titleFont(&fontFamily, ScaleF(13.0f), FontStyleBold,
                                      Gdiplus::UnitPixel);
        Font bodyFont(&fontFamily, ScaleF(12.0f), FontStyleRegular,
                                    Gdiplus::UnitPixel);
        SolidBrush titleBrush(Rgba(23, 32, 51));
        SolidBrush bodyBrush(Rgba(92, 106, 130));

        StringFormat format;
        format.SetAlignment(Gdiplus::StringAlignmentNear);
        format.SetLineAlignment(StringAlignmentCenter);
        format.SetTrimming(StringTrimmingEllipsisCharacter);
        format.SetFormatFlags(StringFormatFlagsNoWrap);

        const float textX = iconRect.X + iconRect.Width + ScaleF(12.0f);
        const float textWidth = toast.X + toast.Width - ScaleF(16.0f) - textX;
        const std::wstring title =
                notification_.title.empty() ? kNewMailNotificationTitle : notification_.title;
        const std::wstring body =
                notification_.body.empty() ? kDefaultNewMailBody : notification_.body;

        graphics.DrawString(title.c_str(), -1, &titleFont,
                                                RectF(textX, toast.Y + ScaleF(15.0f), textWidth,
                                                            ScaleF(22.0f)),
                                                &format, &titleBrush);
        graphics.DrawString(body.c_str(), -1, &bodyFont,
                                                RectF(textX, toast.Y + ScaleF(42.0f), textWidth,
                                                            ScaleF(22.0f)),
                                                &format, &bodyBrush);
    }

    void Draw(Graphics& graphics, int width, int height) const {
        graphics.SetSmoothingMode(SmoothingModeAntiAlias);
        graphics.SetTextRenderingHint(TextRenderingHintClearTypeGridFit);
        graphics.Clear(Color(0, 0, 0, 0));

        const ShellLayout layout = MakeLayout(width, height);

        DrawShadow(graphics, layout);

        GraphicsPath framePath;
        AddRoundedRect(framePath, layout.frame, layout.cornerRadius);
        SolidBrush frameBrush(Rgba(255, 255, 255));
        graphics.FillPath(&frameBrush, &framePath);

        GraphicsState state = graphics.Save();
        graphics.SetClip(&framePath);

        SolidBrush titlebarBrush(Rgba(255, 255, 255));
        SolidBrush contentBrush(Rgba(255, 255, 255));

        graphics.FillRectangle(&titlebarBrush, layout.titlebar);
        graphics.FillRectangle(&contentBrush, layout.content);
        FallbackContentRenderer(dpi_).Draw(graphics, layout.content);

        DrawTitleIcon(graphics, layout.icon);
        DrawNotificationPill(graphics, layout.notificationPill);

        FontFamily fontFamily(L"Segoe UI");
        Font titleFont(&fontFamily, ScaleF(14.0f), FontStyleRegular,
                                      Gdiplus::UnitPixel);
        SolidBrush titleBrush(Rgba(23, 32, 51));
        StringFormat titleFormat;
        titleFormat.SetAlignment(Gdiplus::StringAlignmentNear);
        titleFormat.SetLineAlignment(StringAlignmentCenter);
        titleFormat.SetTrimming(StringTrimmingEllipsisCharacter);
        titleFormat.SetFormatFlags(StringFormatFlagsNoWrap);
        graphics.DrawString(shellTitle_.c_str(), -1, &titleFont, layout.title,
                                                &titleFormat, &titleBrush);

        DrawButton(graphics, ShellButton::Minimize, layout.minimizeButton);
        DrawButton(graphics, ShellButton::Maximize, layout.maximizeButton);
        DrawButton(graphics, ShellButton::Close, layout.closeButton);

        DrawNotificationToast(graphics, layout);

        graphics.Restore(state);
        StrokeRoundedRect(graphics, layout.frame, layout.cornerRadius,
                                            Rgba(24, 35, 55, 30), 1.0f);
    }

    bool CopyBitmapToLayeredWindow(Bitmap& bitmap, int width, int height,
                                                                  const RECT& windowRect) {
        BitmapData data = {};
        Rect rect(0, 0, width, height);
        if (bitmap.LockBits(&rect, ImageLockModeRead, PixelFormat32bppPARGB, &data) != Ok) {
            return false;
        }

        BITMAPINFO bitmapInfo = {};
        bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bitmapInfo.bmiHeader.biWidth = width;
        bitmapInfo.bmiHeader.biHeight = -height;
        bitmapInfo.bmiHeader.biPlanes = 1;
        bitmapInfo.bmiHeader.biBitCount = 32;
        bitmapInfo.bmiHeader.biCompression = BI_RGB;

        void* bits = nullptr;
        HDC screenDc = GetDC(nullptr);
        HBITMAP dib = CreateDIBSection(screenDc, &bitmapInfo, DIB_RGB_COLORS, &bits,
                                                                      nullptr, 0);

        if (!dib || !bits) {
            bitmap.UnlockBits(&data);
            if (dib) {
                DeleteObject(dib);
            }
            ReleaseDC(nullptr, screenDc);
            return false;
        }

        const auto* source = static_cast<const BYTE*>(data.Scan0);
        auto* destination = static_cast<BYTE*>(bits);
        const int destinationStride = width * 4;

        for (int y = 0; y < height; ++y) {
            const BYTE* sourceRow = source + y * data.Stride;
            BYTE* destinationRow = destination + y * destinationStride;
            CopyMemory(destinationRow, sourceRow, destinationStride);
        }

        bitmap.UnlockBits(&data);

        HDC memoryDc = CreateCompatibleDC(screenDc);
        HGDIOBJ oldBitmap = SelectObject(memoryDc, dib);

        POINT destinationPoint = {windowRect.left, windowRect.top};
        SIZE size = {width, height};
        POINT sourcePoint = {0, 0};
        BLENDFUNCTION blend = {AC_SRC_OVER, 0, 255, AC_SRC_ALPHA};

        const BOOL updated = UpdateLayeredWindow(hwnd_, screenDc, &destinationPoint,
                                                                                          &size, memoryDc, &sourcePoint, 0,
                                                                                          &blend, ULW_ALPHA);

        SelectObject(memoryDc, oldBitmap);
        DeleteDC(memoryDc);
        DeleteObject(dib);
        ReleaseDC(nullptr, screenDc);

        return updated != FALSE;
    }

    void Render() {
        if (!hwnd_ || IsIconic(hwnd_)) {
            return;
        }

        RECT windowRect = {};
        GetWindowRect(hwnd_, &windowRect);
        const int width = windowRect.right - windowRect.left;
        const int height = windowRect.bottom - windowRect.top;

        if (width <= 0 || height <= 0) {
            return;
        }

        Bitmap bitmap(width, height, PixelFormat32bppPARGB);
        Graphics graphics(&bitmap);
        Draw(graphics, width, height);
        CopyBitmapToLayeredWindow(bitmap, width, height, windowRect);
    }

    HINSTANCE instance_ = nullptr;
    HWND hwnd_ = nullptr;
    std::wstring shellTitle_ = kDefaultWindowTitle;
    PrivateFontCollection iconFontCollection_;
    std::vector<BYTE> iconFontBuffer_;
    std::vector<BYTE> appIconBuffer_;
    std::unique_ptr<Bitmap> appIconBitmap_;
    HICON appIcon_ = nullptr;
    ShellWindowMetrics metrics_;
    ShellNotification notification_;
    WebViewHost webViewHost_;
    float dpi_ = 1.0f;
    bool trackingMouse_ = false;
    ShellButton hoverButton_ = ShellButton::None;
    ShellButton pressedButton_ = ShellButton::None;
};

ShellWindow::ShellWindow() : impl_(std::make_unique<Impl>()) {}

ShellWindow::~ShellWindow() = default;

bool ShellWindow::Create(HINSTANCE instance) {
    return impl_->Create(instance);
}

void ShellWindow::Show(int commandShow) {
    impl_->Show(commandShow);
}
