#include "DeviationView.h"
#include <algorithm>
#include <cmath>
#include <vector>

namespace uav {
using namespace Gdiplus;

namespace {

constexpr float kLeftMargin = 72.0f;
constexpr float kRightMargin = 22.0f;
constexpr float kTopMargin = 42.0f;
constexpr float kBottomMargin = 54.0f;

constexpr double kXMinDefault = -500.0;
constexpr double kXMaxDefault = 4500.0;
constexpr double kXGridStep = 500.0;
constexpr double kXPadding = 500.0;
constexpr double kXWindow = kXMaxDefault - kXMinDefault;

struct AxisRange {
    double min = kXMinDefault;
    double max = kXMaxDefault;
};

AxisRange visibleXRange(double currentX) {
    double xMax = kXMaxDefault;
    if (currentX + kXPadding > kXMaxDefault) {
        xMax = std::ceil((currentX + kXPadding) / kXGridStep) * kXGridStep;
    }

    return {xMax - kXWindow, xMax};
}

void drawUavIcon(Graphics& g, const PointF& center, float size) {
    PointF points[] = {
        {center.X, center.Y - size},
        {center.X - size * 0.16f, center.Y - size * 0.28f},
        {center.X - size * 0.82f, center.Y},
        {center.X - size * 0.22f, center.Y + size * 0.10f},
        {center.X - size * 0.10f, center.Y + size * 0.88f},
        {center.X, center.Y + size * 0.52f},
        {center.X + size * 0.10f, center.Y + size * 0.88f},
        {center.X + size * 0.22f, center.Y + size * 0.10f},
        {center.X + size * 0.82f, center.Y},
        {center.X + size * 0.16f, center.Y - size * 0.28f}
    };

    SolidBrush fill(Color(255, 255, 150, 80));
    Pen outline(Color(245, 255, 255, 255), 1.1f);
    g.FillPolygon(&fill, points, 10);
    g.DrawPolygon(&outline, points, 10);
}

} // namespace

double DeviationView::displayX(const FrameData& frame,
                               const RunwayInfo&,
                               bool) const {
    return frame.x;
}

void DeviationView::paint(Graphics& g, int w, int h,
                          const std::vector<FrameData>& history,
                          const FrameData& current,
                          const RunwayInfo& runway,
                          bool approachMode) {
    (void)approachMode;

    const float plotW = w - kLeftMargin - kRightMargin;
    const float plotH = h - kTopMargin - kBottomMargin;
    if (plotW <= 0 || plotH <= 0) {
        return;
    }

    const AxisRange axisRange = visibleXRange(current.x);
    const double xMin = axisRange.min;
    const double xMax = axisRange.max;

    double maxAbsDeviation = std::abs(current.y);
    for (const auto& frame : history) {
        const double x = displayX(frame, runway, approachMode);
        if (x >= xMin && x <= xMax) {
            maxAbsDeviation = std::max(maxAbsDeviation, std::abs(frame.y));
        }
    }

    double yRange = std::max(20.0, runway.width * 0.55);
    yRange = std::max(yRange, std::ceil(maxAbsDeviation / 5.0) * 5.0 + 5.0);

    auto toScreen = [&](double px, double py) -> PointF {
        const float sx = kLeftMargin + static_cast<float>((px - xMin) / (xMax - xMin)) * plotW;
        const float sy = kTopMargin + plotH * 0.5f -
                         static_cast<float>(py / yRange) * (plotH * 0.5f);
        return {sx, sy};
    };

    SolidBrush plotBg(Color(255, 25, 28, 36));
    g.FillRectangle(&plotBg, kLeftMargin, kTopMargin, plotW, plotH);

    Pen borderPen(Color(180, 82, 92, 112), 1.0f);
    g.DrawRectangle(&borderPen, kLeftMargin, kTopMargin, plotW, plotH);

    FontFamily ff(L"Microsoft YaHei");
    Font tickFont(&ff, 12, FontStyleRegular, UnitPixel);
    Font labelFont(&ff, 13, FontStyleBold, UnitPixel);
    Font valueFont(&ff, 12, FontStyleRegular, UnitPixel);
    SolidBrush textBrush(Color(230, 220, 228, 242));
    Pen gridPen(Color(55, 112, 126, 152), 1.0f);
    gridPen.SetDashStyle(DashStyleDash);

    StringFormat sfRight;
    sfRight.SetAlignment(StringAlignmentFar);
    sfRight.SetLineAlignment(StringAlignmentCenter);

    StringFormat sfCenter;
    sfCenter.SetAlignment(StringAlignmentCenter);

    for (double deviation = -yRange; deviation <= yRange + 0.1; deviation += 5.0) {
        const PointF p1 = toScreen(xMin, deviation);
        const PointF p2 = toScreen(xMax, deviation);
        g.DrawLine(&gridPen, p1, p2);

        wchar_t label[32];
        swprintf(label, 32, L"%.0f", deviation);
        g.DrawString(label, -1, &tickFont, PointF(kLeftMargin - 8.0f, p1.Y), &sfRight, &textBrush);
    }

    for (int distance = static_cast<int>(xMin); distance <= static_cast<int>(xMax); distance += 500) {
        const PointF p = toScreen(static_cast<double>(distance), 0.0);
        g.DrawLine(&gridPen, PointF(p.X, kTopMargin), PointF(p.X, kTopMargin + plotH));

        wchar_t label[32];
        swprintf(label, 32, L"%d", distance);
        g.DrawString(label, -1, &tickFont, PointF(p.X, kTopMargin + plotH + 10.0f), &sfCenter, &textBrush);
    }

    Pen centerPen(Color(185, 130, 188, 255), 1.3f);
    centerPen.SetDashStyle(DashStyleDash);
    g.DrawLine(&centerPen, toScreen(xMin, 0.0), toScreen(xMax, 0.0));

    Pen safePen(Color(180, 255, 206, 96), 1.2f);
    safePen.SetDashStyle(DashStyleDashDot);
    g.DrawLine(&safePen, toScreen(xMin, 5.0), toScreen(xMax, 5.0));
    g.DrawLine(&safePen, toScreen(xMin, -5.0), toScreen(xMax, -5.0));

    if (history.size() >= 2) {
        std::vector<PointF> points;
        points.reserve(history.size());

        for (const auto& frame : history) {
            const double x = displayX(frame, runway, approachMode);
            if (x < xMin - 50.0 || x > xMax + 50.0) {
                continue;
            }
            points.push_back(toScreen(x, frame.y));
        }

        if (points.size() >= 2) {
            Pen trackPen(Color(230, 32, 190, 245), 3.0f);
            trackPen.SetLineJoin(LineJoinRound);
            g.DrawLines(&trackPen, points.data(), static_cast<INT>(points.size()));
        }
    }

    const double currentX = displayX(current, runway, approachMode);
    if (currentX >= xMin && currentX <= xMax) {
        const PointF currentPoint = toScreen(currentX, current.y);
        drawUavIcon(g, currentPoint, 8.5f);

        wchar_t label[64];
        swprintf(label, 64, L"\x4FA7\x504F %.1f m", current.y);
        g.DrawString(label, -1, &valueFont,
                     PointF(currentPoint.X + 12.0f, currentPoint.Y - 20.0f), &textBrush);
    }

    g.DrawString(L"\x6A2A\x5411\x504F\x5DEE (m)", -1, &labelFont,
                 PointF(6.0f, kTopMargin + plotH * 0.5f - 18.0f), &textBrush);
    g.DrawString(L"\x8DD1\x9053\x65B9\x5411\x8DDD\x79BB (m)", -1, &labelFont,
                 PointF(kLeftMargin + plotW * 0.5f - 78.0f, kTopMargin + plotH + 28.0f), &textBrush);

    Font titleFont(&ff, 16, FontStyleBold, UnitPixel);
    SolidBrush titleBrush(Color(255, 236, 240, 248));
    g.DrawString(L"\x4FA7\x504F\x56FE", -1, &titleFont, PointF(kLeftMargin, 8.0f), &titleBrush);
}

} // namespace uav
