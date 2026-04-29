#include "TrackView.h"
#include <algorithm>
#include <cmath>

namespace uav {
using namespace Gdiplus;

namespace {

bool isInsideRunway(const FrameData& frame, const RunwayInfo& runway) {
    const double halfWidth = runway.width * 0.5;
    return frame.x >= 0.0 && frame.x <= runway.length &&
           std::abs(frame.y) <= halfWidth;
}

void drawUavIcon(Graphics& g, const PointF& center, float size) {
    PointF points[] = {
        {center.X, center.Y - size},
        {center.X - size * 0.18f, center.Y - size * 0.30f},
        {center.X - size * 0.92f, center.Y - size * 0.02f},
        {center.X - size * 0.24f, center.Y + size * 0.10f},
        {center.X - size * 0.12f, center.Y + size * 0.96f},
        {center.X, center.Y + size * 0.56f},
        {center.X + size * 0.12f, center.Y + size * 0.96f},
        {center.X + size * 0.24f, center.Y + size * 0.10f},
        {center.X + size * 0.92f, center.Y - size * 0.02f},
        {center.X + size * 0.18f, center.Y - size * 0.30f}
    };

    SolidBrush fill(Color(255, 255, 150, 80));
    Pen outline(Color(245, 255, 255, 255), 1.2f);
    g.FillPolygon(&fill, points, 10);
    g.DrawPolygon(&outline, points, 10);
}

} // namespace

void TrackView::paint(Graphics& g, int w, int h,
                      const std::vector<FrameData>& history,
                      const FrameData& current,
                      const RunwayInfo& runway) {
    const float leftMargin = 26.0f;
    const float rightMargin = 18.0f;
    const float topMargin = 42.0f;
    const float bottomMargin = 20.0f;
    const float plotW = w - leftMargin - rightMargin;
    const float plotH = h - topMargin - bottomMargin;
    if (plotW <= 0 || plotH <= 0) {
        return;
    }

    double lateralExtent = std::max(runway.width * 0.8, 55.0);
    for (const auto& frame : history) {
        if (isInsideRunway(frame, runway)) {
            lateralExtent = std::max(lateralExtent, std::abs(frame.y) * 1.3);
        }
    }
    lateralExtent = std::max(lateralExtent, std::abs(current.y) * 1.2);

    const double worldXMin = -60.0;
    const double worldXMax = std::max(runway.length + 180.0, current.x + 160.0);

    auto toScreen = [&](double wx, double wy) -> PointF {
        const float sx = leftMargin +
                         static_cast<float>((wy + lateralExtent) / (2.0 * lateralExtent)) * plotW;
        const float sy = topMargin + plotH -
                         static_cast<float>((wx - worldXMin) / (worldXMax - worldXMin)) * plotH;
        return {sx, sy};
    };

    SolidBrush plotBg(Color(255, 25, 28, 36));
    g.FillRectangle(&plotBg, leftMargin, topMargin, plotW, plotH);

    Pen borderPen(Color(180, 82, 92, 112), 1.0f);
    g.DrawRectangle(&borderPen, leftMargin, topMargin, plotW, plotH);

    const double halfWidth = runway.width * 0.5;
    PointF runwayShape[4] = {
        toScreen(0.0, -halfWidth),
        toScreen(0.0, halfWidth),
        toScreen(runway.length, halfWidth),
        toScreen(runway.length, -halfWidth)
    };

    SolidBrush runwayBrush(Color(195, 85, 92, 108));
    g.FillPolygon(&runwayBrush, runwayShape, 4);

    Pen runwayPen(Color(225, 208, 214, 226), 1.6f);
    g.DrawPolygon(&runwayPen, runwayShape, 4);

    Pen centerPen(Color(180, 224, 234, 248), 1.2f);
    centerPen.SetDashStyle(DashStyleDash);
    g.DrawLine(&centerPen, toScreen(0.0, 0.0), toScreen(runway.length, 0.0));

    const double touchdownStart = std::max(0.0, runway.touchdownX - 30.0);
    const double touchdownEnd = std::min(runway.length, runway.touchdownX + 30.0);
    PointF touchdownZone[4] = {
        toScreen(touchdownStart, -halfWidth * 0.62),
        toScreen(touchdownStart, halfWidth * 0.62),
        toScreen(touchdownEnd, halfWidth * 0.62),
        toScreen(touchdownEnd, -halfWidth * 0.62)
    };
    SolidBrush touchdownBrush(Color(85, 255, 226, 92));
    g.FillPolygon(&touchdownBrush, touchdownZone, 4);

    if (isInsideRunway(current, runway)) {
        PointF usedShape[4] = {
            toScreen(0.0, -halfWidth),
            toScreen(0.0, halfWidth),
            toScreen(current.x, halfWidth),
            toScreen(current.x, -halfWidth)
        };

        SolidBrush usedBrush(Color(65, 255, 120, 120));
        g.FillPolygon(&usedBrush, usedShape, 4);
    }

    if (history.size() >= 2) {
        Pen trackPen(Color(235, 26, 186, 248), 3.0f);
        trackPen.SetLineJoin(LineJoinRound);

        for (size_t i = 1; i < history.size(); ++i) {
            if (!isInsideRunway(history[i - 1], runway) || !isInsideRunway(history[i], runway)) {
                continue;
            }
            g.DrawLine(&trackPen,
                       toScreen(history[i - 1].x, history[i - 1].y),
                       toScreen(history[i].x, history[i].y));
        }
    }

    const PointF currentPoint = toScreen(current.x, current.y);
    drawUavIcon(g, currentPoint, 10.0f);

    FontFamily ff(L"Microsoft YaHei");
    Font titleFont(&ff, 16, FontStyleBold, UnitPixel);
    Font infoFont(&ff, 12, FontStyleRegular, UnitPixel);
    Font scaleFont(&ff, 11, FontStyleRegular, UnitPixel);
    SolidBrush titleBrush(Color(255, 236, 240, 248));
    SolidBrush infoBrush(Color(230, 224, 230, 240));

    if (isInsideRunway(current, runway)) {
        const double remain = runway.length - current.x;
        wchar_t runwayInfo[96];
        swprintf(runwayInfo, 96, L"\x5DF2\x7528 %.0f m   \x4F59\x91CF %.0f m", current.x, remain);
        g.DrawString(runwayInfo, -1, &infoFont,
                     PointF(leftMargin + 8.0f, topMargin + 8.0f), &infoBrush);
    }

    const PointF scaleStart = toScreen(80.0, -lateralExtent * 0.82);
    const PointF scaleEnd = toScreen(280.0, -lateralExtent * 0.82);
    Pen scalePen(Color(215, 214, 220, 228), 1.8f);
    g.DrawLine(&scalePen, scaleStart, scaleEnd);
    g.DrawString(L"200 m", -1, &scaleFont,
                 PointF(scaleStart.X + 4.0f, scaleStart.Y + 4.0f), &infoBrush);

    g.DrawString(L"\x5168\x5C40\x8F68\x8FF9\x56FE (\x4FEF\x89C6)", -1, &titleFont,
                 PointF(leftMargin, 8.0f), &titleBrush);
}

} // namespace uav
