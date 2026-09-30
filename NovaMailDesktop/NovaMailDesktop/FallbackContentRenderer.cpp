#include "FallbackContentRenderer.h"

#include <algorithm>

using Gdiplus::Color;
using Gdiplus::Graphics;
using Gdiplus::GraphicsPath;
using Gdiplus::Pen;
using Gdiplus::RectF;
using Gdiplus::SolidBrush;

namespace {

Color Rgba(BYTE red, BYTE green, BYTE blue, BYTE alpha = 255) {
    return Color(alpha, red, green, blue);
}

}  // namespace

FallbackContentRenderer::FallbackContentRenderer(float dpi) : dpi_(dpi) {}

void FallbackContentRenderer::Draw(Graphics& graphics, const RectF& content) const {
    SolidBrush background(Rgba(255, 255, 255));
    graphics.FillRectangle(&background, content);

    const float sidebarWidth = std::min(ScaleF(255.0f), content.Width * 0.32f);
    const float detailX = content.X + sidebarWidth;
    Pen divider(Rgba(30, 41, 59, 18), ScaleF(1.0f));
    graphics.DrawLine(&divider, detailX, content.Y, detailX,
                      content.Y + content.Height);

    const float leftX = content.X + ScaleF(13.0f);
    const float leftTop = content.Y + ScaleF(10.0f);
    const float leftLong = std::min(ScaleF(164.0f), sidebarWidth - ScaleF(38.0f));
    const float leftShort =
        std::min(ScaleF(118.0f), sidebarWidth - ScaleF(72.0f));
    for (int i = 0; i < 6; ++i) {
        const float y = leftTop + i * ScaleF(80.0f);
        if (y + ScaleF(32.0f) > content.Y + content.Height) {
            break;
        }

        DrawSkeletonBar(graphics, RectF(leftX, y, leftLong, ScaleF(7.0f)));
        DrawSkeletonBar(graphics,
                        RectF(leftX, y + ScaleF(23.0f), leftShort,
                              ScaleF(7.0f)));
    }

    const float detailPad = ScaleF(38.0f);
    const float detailWidth =
        content.X + content.Width - detailX - detailPad * 2.0f;
    if (detailWidth <= ScaleF(120.0f)) {
        return;
    }

    DrawSkeletonBar(
        graphics,
        RectF(detailX + detailPad, content.Y + ScaleF(7.0f),
              std::min(ScaleF(345.0f), detailWidth * 0.64f), ScaleF(18.0f)),
        9.0f);
    DrawSkeletonBar(
        graphics,
        RectF(detailX + detailPad, content.Y + ScaleF(50.0f),
              std::min(ScaleF(528.0f), detailWidth * 0.98f), ScaleF(9.0f)),
        5.0f);
    DrawSkeletonBar(
        graphics,
        RectF(detailX + detailPad, content.Y + ScaleF(75.0f),
              std::min(ScaleF(456.0f), detailWidth * 0.84f), ScaleF(9.0f)),
        5.0f);
    FillRoundedRect(
        graphics,
        RectF(detailX + detailPad, content.Y + ScaleF(119.0f), detailWidth,
              std::min(ScaleF(180.0f),
                       std::max(ScaleF(96.0f),
                                content.Height - ScaleF(180.0f)))),
        ScaleF(10.0f), Rgba(240, 243, 248));
}

float FallbackContentRenderer::ScaleF(float value) const {
    return value * dpi_;
}

void FallbackContentRenderer::DrawSkeletonBar(Graphics& graphics,
                                              const RectF& rect,
                                              float radius) const {
    FillRoundedRect(graphics, rect, ScaleF(radius), Rgba(218, 226, 238));
}

void FallbackContentRenderer::AddRoundedRect(GraphicsPath& path,
                                             const RectF& rect, float radius) {
    const float diameter = radius * 2.0f;

    if (diameter <= 0.0f) {
        path.AddRectangle(rect);
        path.CloseFigure();
        return;
    }

    path.AddArc(rect.X, rect.Y, diameter, diameter, 180.0f, 90.0f);
    path.AddArc(rect.X + rect.Width - diameter, rect.Y, diameter, diameter,
                270.0f, 90.0f);
    path.AddArc(rect.X + rect.Width - diameter,
                rect.Y + rect.Height - diameter, diameter, diameter, 0.0f,
                90.0f);
    path.AddArc(rect.X, rect.Y + rect.Height - diameter, diameter, diameter,
                90.0f, 90.0f);
    path.CloseFigure();
}

void FallbackContentRenderer::FillRoundedRect(Graphics& graphics,
                                              const RectF& rect, float radius,
                                              const Color& color) {
    SolidBrush brush(color);
    GraphicsPath path;
    AddRoundedRect(path, rect, radius);
    graphics.FillPath(&brush, &path);
}
