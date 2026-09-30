#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <objbase.h>

#include "WebViewHost.h"
#include "ShellDebugLog.h"

#include <WebView2.h>
#include <wrl.h>

#include <algorithm>
#include <cwctype>
#include <string>
#include <utility>
#include <vector>

using Microsoft::WRL::Callback;
using Microsoft::WRL::ComPtr;

class WebViewHost::Impl {
public:
    void Initialize(HWND parent, std::wstring distRoot, std::wstring entryFile,
                    MessageCallback onMessage) {
        parent_ = parent;
        distRoot_ = std::move(distRoot);
        entryFile_ = std::move(entryFile);
        onMessage_ = std::move(onMessage);

        WriteShellDebugLog(L"webview initialize: " + distRoot_ + L"\\" + entryFile_);
        CreateCoreWebView2EnvironmentWithOptions(
            nullptr, nullptr, nullptr,
            Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
                [this](HRESULT result, ICoreWebView2Environment* environment) -> HRESULT {
                    WriteShellDebugLog(L"webview environment completed: " +
                                       std::to_wstring(result));
                    if (FAILED(result) || !environment || !parent_) {
                        return S_OK;
                    }

                    environment_ = environment;
                    return environment_->CreateCoreWebView2Controller(
                        parent_,
                        Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
                            [this](HRESULT controllerResult,
                                   ICoreWebView2Controller* controller) -> HRESULT {
                                WriteShellDebugLog(L"webview controller completed: " +
                                                   std::to_wstring(controllerResult));
                                if (FAILED(controllerResult) || !controller) {
                                    return S_OK;
                                }

                                controller_ = controller;
                                if (SUCCEEDED(controller_.As(&controller2_)) &&
                                    controller2_) {
                                    COREWEBVIEW2_COLOR transparent = {0, 255, 255, 255};
                                    controller2_->put_DefaultBackgroundColor(transparent);
                                }

                                controller_->get_CoreWebView2(&webview_);
                                if (!webview_) {
                                    return S_OK;
                                }

                                ConfigureWebViewBehavior();

                                webview_->add_WebMessageReceived(
                                    Callback<ICoreWebView2WebMessageReceivedEventHandler>(
                                        [this](ICoreWebView2*,
                                               ICoreWebView2WebMessageReceivedEventArgs* args)
                                            -> HRESULT {
                                            return OnWebMessage(args);
                                        })
                                        .Get(),
                                    &webMessageToken_);

                                RegisterSpaFallback();
                                InstallDocumentClipStyle();
                                ApplyBounds();
                                controller_->put_IsVisible(TRUE);
                                const std::wstring entryUri = BuildEntryUri();
                                WriteShellDebugLog(L"webview navigate: " + entryUri);
                                webview_->Navigate(entryUri.c_str());
                                ready_ = true;
                                return S_OK;
                            })
                            .Get());
                })
                .Get());
    }

    void Resize(const RECT& bounds, int bottomCornerRadius) {
        bounds_ = bounds;
        bottomCornerRadius_ = std::max(0, bottomCornerRadius);
        ApplyClipRadius();
        if (!controller_) {
            return;
        }

        ApplyBounds();
    }

    bool IsReady() const {
        return ready_;
    }

private:
    void ApplyBounds() {
        if (!controller_ || bounds_.right <= bounds_.left || bounds_.bottom <= bounds_.top) {
            return;
        }

        controller_->put_Bounds(bounds_);
    }

    void ConfigureWebViewBehavior() {
        if (!webview_) {
            return;
        }

        ComPtr<ICoreWebView2Settings> settings;
        if (FAILED(webview_->get_Settings(&settings)) || !settings) {
            return;
        }

        settings->put_IsStatusBarEnabled(FALSE);
        settings->put_AreDevToolsEnabled(FALSE);
        settings->put_AreDefaultContextMenusEnabled(FALSE);
        settings->put_IsZoomControlEnabled(FALSE);
        settings->put_IsBuiltInErrorPageEnabled(FALSE);

        ComPtr<ICoreWebView2Settings3> settings3;
        if (SUCCEEDED(settings.As(&settings3)) && settings3) {
            settings3->put_AreBrowserAcceleratorKeysEnabled(FALSE);
        }
    }

    void RegisterSpaFallback() {
        if (!webview_) {
            return;
        }

        const std::wstring filter = BuildOrigin() + L"/*";
        const HRESULT filterResult = webview_->AddWebResourceRequestedFilter(
            filter.c_str(), COREWEBVIEW2_WEB_RESOURCE_CONTEXT_ALL);
        const std::wstring rootFilter = BuildOrigin() + L"/";
        const HRESULT rootFilterResult = webview_->AddWebResourceRequestedFilter(
            rootFilter.c_str(), COREWEBVIEW2_WEB_RESOURCE_CONTEXT_ALL);
        WriteShellDebugLog(L"webview spa fallback filter result: " +
                           std::to_wstring(filterResult));
        WriteShellDebugLog(L"webview spa root fallback filter result: " +
                           std::to_wstring(rootFilterResult));

        webview_->add_WebResourceRequested(
            Callback<ICoreWebView2WebResourceRequestedEventHandler>(
                [this](ICoreWebView2*, ICoreWebView2WebResourceRequestedEventArgs* args)
                    -> HRESULT {
                    return OnWebResourceRequested(args);
                })
                .Get(),
            &webResourceRequestedToken_);
    }

    void InstallDocumentClipStyle() {
        if (!webview_) {
            return;
        }

        const std::wstring script =
            L"(() => {"
            L"  Object.defineProperty(window, '__NOVA_MAIL_SHELL__', {"
            L"    configurable: false,"
            L"    enumerable: false,"
            L"    writable: false,"
            L"    value: Object.freeze({"
            L"      name: 'NovaMailDesktop',"
            L"      version: '1.0.0',"
            L"      bridge: 'webview2'"
            L"    })"
            L"  });"
            L"  const block = event => {"
            L"    event.preventDefault();"
            L"    event.stopPropagation();"
            L"  };"
            L"  const blockKeyboard = event => {"
            L"    const key = String(event.key || '').toLowerCase();"
            L"    const code = String(event.code || '').toLowerCase();"
            L"    const isRefresh = key === 'f5' || code === 'f5' ||"
            L"      ((event.ctrlKey || event.metaKey) && key === 'r');"
            L"    const isDevTools = key === 'f12' || code === 'f12' ||"
            L"      ((event.ctrlKey || event.metaKey) && event.shiftKey &&"
            L"        (key === 'i' || key === 'j' || key === 'c'));"
            L"    if (isRefresh || isDevTools) {"
            L"      block(event);"
            L"    }"
            L"  };"
            L"  const disableDraggable = root => {"
            L"    if (!root || !root.querySelectorAll) {"
            L"      return;"
            L"    }"
            L"    root.querySelectorAll('a,img').forEach(element => {"
            L"      element.draggable = false;"
            L"      element.setAttribute('draggable', 'false');"
            L"    });"
            L"  };"
            L"  const apply = () => {"
            L"    if (location.pathname === '/index.html') {"
            L"      history.replaceState(null, document.title, '/' + location.search + location.hash);"
            L"    }"
            L"    document.documentElement.style.background = 'transparent';"
            L"    document.documentElement.style.setProperty('--nova-shell-content-radius', '" +
            std::to_wstring(bottomCornerRadius_) +
            L"px');"
            L"    let style = document.getElementById('nova-shell-window-clip-style');"
            L"    if (!style) {"
            L"      style = document.createElement('style');"
            L"      style.id = 'nova-shell-window-clip-style';"
            L"      style.textContent = 'html,body{margin:0!important;width:100%;height:100%;background:transparent!important;overflow:hidden!important;}#app{width:100%;height:100vh;min-height:100vh;overflow:auto;border-bottom-left-radius:var(--nova-shell-content-radius);border-bottom-right-radius:var(--nova-shell-content-radius);}a,img{-webkit-user-drag:none!important;user-drag:none!important;}';"
            L"      (document.head || document.documentElement).appendChild(style);"
            L"    }"
            L"    disableDraggable(document);"
            L"  };"
            L"  document.addEventListener('contextmenu', block, true);"
            L"  document.addEventListener('dragstart', block, true);"
            L"  document.addEventListener('dragover', block, true);"
            L"  document.addEventListener('drop', block, true);"
            L"  document.addEventListener('keydown', blockKeyboard, true);"
            L"  if (document.readyState === 'loading') {"
            L"    document.addEventListener('DOMContentLoaded', apply, { once: true });"
            L"  }"
            L"  apply();"
            L"  new MutationObserver(mutations => {"
            L"    for (const mutation of mutations) {"
            L"      for (const node of mutation.addedNodes) {"
            L"        if (node.nodeType === 1) {"
            L"          disableDraggable(node);"
            L"        }"
            L"      }"
            L"    }"
            L"  }).observe(document.documentElement, { childList: true, subtree: true });"
            L"})();";

        webview_->AddScriptToExecuteOnDocumentCreated(
            script.c_str(),
            Callback<ICoreWebView2AddScriptToExecuteOnDocumentCreatedCompletedHandler>(
                [](HRESULT, LPCWSTR) -> HRESULT {
                    return S_OK;
                })
                .Get());
    }

    void ApplyClipRadius() {
        if (!webview_) {
            return;
        }

        const std::wstring script =
            L"document.documentElement.style.setProperty('--nova-shell-content-radius', '" +
            std::to_wstring(bottomCornerRadius_) + L"px');";
        webview_->ExecuteScript(script.c_str(), nullptr);
    }

    HRESULT OnWebResourceRequested(ICoreWebView2WebResourceRequestedEventArgs* args) {
        if (!args || !environment_) {
            return S_OK;
        }

        ComPtr<ICoreWebView2WebResourceRequest> request;
        if (FAILED(args->get_Request(&request)) || !request) {
            return S_OK;
        }

        LPWSTR rawUri = nullptr;
        if (FAILED(request->get_Uri(&rawUri)) || !rawUri) {
            return S_OK;
        }

        std::wstring uri(rawUri);
        CoTaskMemFree(rawUri);

        COREWEBVIEW2_WEB_RESOURCE_CONTEXT resourceContext =
            COREWEBVIEW2_WEB_RESOURCE_CONTEXT_ALL;
        args->get_ResourceContext(&resourceContext);

        std::wstring filePath;
        std::wstring contentType;
        if (!TryResolveResourceFile(uri, resourceContext, &filePath, &contentType)) {
            return S_OK;
        }

        ComPtr<IStream> stream = CreateFileStream(filePath);
        if (!stream) {
            WriteShellDebugLog(L"webview resource stream failed: " + filePath);
            return S_OK;
        }

        ComPtr<ICoreWebView2WebResourceResponse> response;
        const std::wstring headers =
            L"Content-Type: " + contentType + L"\r\nCache-Control: no-store";
        const HRESULT responseResult = environment_->CreateWebResourceResponse(
            stream.Get(), 200, L"OK", headers.c_str(), &response);
        if (SUCCEEDED(responseResult) && response) {
            args->put_Response(response.Get());
            WriteShellDebugLog(L"webview resource served: " + uri + L" -> " + filePath);
        }

        return S_OK;
    }

    HRESULT OnWebMessage(ICoreWebView2WebMessageReceivedEventArgs* args) {
        if (!args || !onMessage_) {
            return S_OK;
        }

        LPWSTR rawMessage = nullptr;
        if (SUCCEEDED(args->TryGetWebMessageAsString(&rawMessage)) && rawMessage) {
            std::wstring message(rawMessage);
            CoTaskMemFree(rawMessage);
            WriteShellDebugLog(L"webview message: " + message);
            onMessage_(message);
        }

        return S_OK;
    }

    std::wstring BuildEntryUri() const {
        return BuildOrigin() + L"/";
    }

    std::wstring BuildOrigin() const {
        std::wstring origin = L"https://";
        origin += kVirtualHostName;
        return origin;
    }

    bool TryResolveResourceFile(const std::wstring& uri,
                                COREWEBVIEW2_WEB_RESOURCE_CONTEXT resourceContext,
                                std::wstring* filePath,
                                std::wstring* contentType) const {
        if (!filePath || !contentType) {
            return false;
        }

        std::wstring uriPath;
        if (!TryGetAppUriPath(uri, &uriPath)) {
            return false;
        }

        std::wstring candidatePath;
        if (uriPath.empty() || uriPath == L"/") {
            candidatePath = BuildDistFilePath(entryFile_);
        } else if (TryBuildExistingResourcePath(uriPath, &candidatePath)) {
        } else if (resourceContext == COREWEBVIEW2_WEB_RESOURCE_CONTEXT_DOCUMENT) {
            candidatePath = BuildDistFilePath(entryFile_);
        } else {
            return false;
        }

        *filePath = candidatePath;
        *contentType = GetContentType(candidatePath);
        return true;
    }

    bool TryGetAppUriPath(const std::wstring& uri, std::wstring* uriPath) const {
        const std::wstring origin = BuildOrigin();
        if (uri.size() < origin.size() ||
            CompareStringOrdinal(uri.c_str(), static_cast<int>(origin.size()),
                                 origin.c_str(), static_cast<int>(origin.size()),
                                 TRUE) != CSTR_EQUAL) {
            return false;
        }

        size_t pathStart = origin.size();
        if (pathStart >= uri.size()) {
            if (uriPath) {
                *uriPath = L"/";
            }
            return true;
        }

        if (uri[pathStart] != L'/') {
            return false;
        }

        const size_t pathEnd = uri.find_first_of(L"?#", pathStart);
        if (uriPath) {
            *uriPath =
            uri.substr(pathStart, pathEnd == std::wstring::npos
                                      ? std::wstring::npos
                                      : pathEnd - pathStart);
        }

        return true;
    }

    bool TryBuildExistingResourcePath(std::wstring uriPath,
                                      std::wstring* filePath) const {
        if (!filePath) {
            return false;
        }

        while (!uriPath.empty() && uriPath.front() == L'/') {
            uriPath.erase(uriPath.begin());
        }

        if (uriPath.empty() || uriPath.find(L"..") != std::wstring::npos ||
            uriPath.find(L':') != std::wstring::npos) {
            return false;
        }

        std::replace(uriPath.begin(), uriPath.end(), L'/', L'\\');
        std::wstring path = BuildDistFilePath(uriPath);

        const DWORD attributes = GetFileAttributesW(path.c_str());
        if (attributes == INVALID_FILE_ATTRIBUTES ||
            (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
            return false;
        }

        *filePath = path;
        return true;
    }

    std::wstring BuildDistFilePath(const std::wstring& relativePath) const {
        std::wstring path = distRoot_;
        if (!path.empty() && path.back() != L'\\' && path.back() != L'/') {
            path += L"\\";
        }
        path += relativePath;
        return path;
    }

    std::wstring GetContentType(std::wstring path) const {
        const size_t extensionStart = path.find_last_of(L'.');
        std::wstring extension =
            extensionStart == std::wstring::npos ? L"" : path.substr(extensionStart);
        std::transform(extension.begin(), extension.end(), extension.begin(),
                       [](wchar_t value) {
                           return static_cast<wchar_t>(std::towlower(value));
                       });

        if (extension == L".html" || extension == L".htm") {
            return L"text/html; charset=utf-8";
        }
        if (extension == L".js" || extension == L".mjs") {
            return L"text/javascript; charset=utf-8";
        }
        if (extension == L".css") {
            return L"text/css; charset=utf-8";
        }
        if (extension == L".json") {
            return L"application/json; charset=utf-8";
        }
        if (extension == L".svg") {
            return L"image/svg+xml";
        }
        if (extension == L".png") {
            return L"image/png";
        }
        if (extension == L".jpg" || extension == L".jpeg") {
            return L"image/jpeg";
        }
        if (extension == L".gif") {
            return L"image/gif";
        }
        if (extension == L".ico") {
            return L"image/x-icon";
        }
        if (extension == L".webp") {
            return L"image/webp";
        }
        if (extension == L".wasm") {
            return L"application/wasm";
        }
        if (extension == L".ttf") {
            return L"font/ttf";
        }
        if (extension == L".otf") {
            return L"font/otf";
        }
        if (extension == L".woff") {
            return L"font/woff";
        }
        if (extension == L".woff2") {
            return L"font/woff2";
        }

        return L"application/octet-stream";
    }

    ComPtr<IStream> CreateFileStream(const std::wstring& filePath) const {
        ComPtr<IStream> stream;

        HANDLE file = CreateFileW(filePath.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
                                  OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (file == INVALID_HANDLE_VALUE) {
            return stream;
        }

        LARGE_INTEGER size = {};
        if (!GetFileSizeEx(file, &size) || size.HighPart != 0) {
            CloseHandle(file);
            return stream;
        }

        const DWORD fileSize = static_cast<DWORD>(size.LowPart);
        HGLOBAL memory = GlobalAlloc(GMEM_MOVEABLE, std::max<DWORD>(1, fileSize));
        if (!memory) {
            CloseHandle(file);
            return stream;
        }

        void* buffer = GlobalLock(memory);
        if (!buffer) {
            GlobalFree(memory);
            CloseHandle(file);
            return stream;
        }

        DWORD bytesRead = 0;
        const BOOL readOk =
            ReadFile(file, buffer, fileSize, &bytesRead, nullptr);
        GlobalUnlock(memory);
        CloseHandle(file);

        if (!readOk || bytesRead != fileSize) {
            GlobalFree(memory);
            return stream;
        }

        IStream* rawStream = nullptr;
        if (FAILED(CreateStreamOnHGlobal(memory, TRUE, &rawStream))) {
            GlobalFree(memory);
            return stream;
        }

        stream.Attach(rawStream);
        return stream;
    }

    static constexpr wchar_t kVirtualHostName[] = L"appassets.novamail.local";

    HWND parent_ = nullptr;
    std::wstring distRoot_;
    std::wstring entryFile_;
    MessageCallback onMessage_;
    ComPtr<ICoreWebView2Environment> environment_;
    ComPtr<ICoreWebView2Controller> controller_;
    ComPtr<ICoreWebView2Controller2> controller2_;
    ComPtr<ICoreWebView2> webview_;
    RECT bounds_ = {};
    int bottomCornerRadius_ = 0;
    EventRegistrationToken webMessageToken_ = {};
    EventRegistrationToken webResourceRequestedToken_ = {};
    bool ready_ = false;
};

WebViewHost::WebViewHost() : impl_(new Impl()) {}

WebViewHost::~WebViewHost() {
    delete impl_;
}

void WebViewHost::Initialize(HWND parent, const std::wstring& distRoot,
                             const std::wstring& entryFile,
                             MessageCallback onMessage) {
    impl_->Initialize(parent, distRoot, entryFile, std::move(onMessage));
}

void WebViewHost::Resize(const RECT& bounds, int bottomCornerRadius) {
    impl_->Resize(bounds, bottomCornerRadius);
}

bool WebViewHost::IsReady() const {
    return impl_->IsReady();
}
