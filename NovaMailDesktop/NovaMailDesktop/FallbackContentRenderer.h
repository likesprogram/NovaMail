#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>
#include <objidl.h>
#include <olectl.h>
#include <gdiplus.h>

class FallbackContentRenderer {
public:
    explicit FallbackContentRenderer(float dpi);

    void Draw(Gdiplus::Graphics& graphics, const Gdiplus::RectF& content) const;

private:
    float ScaleF(float value) const;
    void DrawSkeletonBar(Gdiplus::Graphics& graphics, const Gdiplus::RectF& rect,
                         float radius = 4.0f) const;

    static void AddRoundedRect(Gdiplus::GraphicsPath& path,
                               const Gdiplus::RectF& rect, float radius);
    static void FillRoundedRect(Gdiplus::Graphics& graphics,
                                const Gdiplus::RectF& rect, float radius,
                                const Gdiplus::Color& color);

    float dpi_ = 1.0f;
};
