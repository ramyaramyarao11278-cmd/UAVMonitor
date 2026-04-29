#include "ProfileView.h"
#include <algorithm>
#include <cmath>
#include <vector>

namespace uav {
using namespace Gdiplus;

namespace {

constexpr double kXMinDefault = -500.0;
constexpr double kXMaxDefault = 4500.0;
constexpr double kXGridStep = 500.0;
constexpr double kXPadding = 500.0;
constexpr double kXWindow = kXMaxDefault - kXMinDefault;
constexpr double kAltitudeMax = 500.0;

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
        {center.X - size * 0.18f, center.Y - size * 0.28f},
        {center.X - size * 0.90f, center.Y - size * 0.05f},
        {center.X - size * 0.24f, center.Y + size * 0.10f},
        {center.X - size * 0.12f, center.Y + size * 0.95f},
        {center.X, center.Y + size * 0.56f},
        {center.X + size * 0.12f, center.Y + size * 0.95f},
        {center.X + size * 0.24f, center.Y + size * 0.10f},
        {center.X + size * 0.90f, center.Y - size * 0.05f},
        {center.X + size * 0.18f, center.Y - size * 0.28f}
    };

    SolidBrush fill(Color(255, 255, 150, 80));
    Pen outline(Color(245, 255, 255, 255), 1.2f);
    g.FillPolygon(&fill, points, 10);
    g.DrawPolygon(&outline, points, 10);
}

} // namespace

double ProfileView::displayX(const FrameData& frame,
                             const RunwayInfo&,
                             bool) const {
    return frame.x;
}

PointF ProfileView::toScreen(double x,
                             double altitude,
                             double xMin,
                             double xMax,
                             float plotW,
                             float plotH) const {
    const float sx = ML + static_cast<float>((x - xMin) / (xMax - xMin)) * plotW;
    const float sy = MT + plotH -
                     static_cast<float>(std::clamp(altitude, 0.0, kAltitudeMax) / kAltitudeMax) * plotH;
    return {sx, sy};
}

void ProfileView::drawAxes(Graphics& g,
                           float plotW,
                           float plotH,
                           double xMin,
                           double xMax) const {
    SolidBrush plotBg(Color(255, 25, 28, 36));
    g.FillRectangle(&plotBg, ML, MT, plotW, plotH);

    Pen borderPen(Color(180, 82, 92, 112), 1.0f);
    g.DrawRectangle(&borderPen, ML, MT, plotW, plotH);

    FontFamily ff(L"Microsoft YaHei");
    Font tickFont(&ff, 12, FontStyleRegular, UnitPixel);
    Font labelFont(&ff, 13, FontStyleBold, UnitPixel);
    SolidBrush textBrush(Color(230, 220, 228, 242));
    Pen gridPen(Color(55, 112, 126, 152), 1.0f);
    gridPen.SetDashStyle(DashStyleDash);

    StringFormat sfRight;
    sfRight.SetAlignment(StringAlignmentFar);
    sfRight.SetLineAlignment(StringAlignmentCenter);

    StringFormat sfCenter;
    sfCenter.SetAlignment(StringAlignmentCenter);

    for (int altitude = 0; altitude <= static_cast<int>(kAltitudeMax); altitude += 100) {
        const float y = MT + plotH -
                        static_cast<float>(altitude / kAltitudeMax) * plotH;
        g.DrawLine(&gridPen, PointF(ML, y), PointF(ML + plotW, y));

        wchar_t label[32];
        swprintf(label, 32, L"%d", altitude);
        g.DrawString(label, -1, &tickFont, PointF(ML - 8, y), &sfRight, &textBrush);
    }

    for (int distance = static_cast<int>(xMin); distance <= static_cast<int>(xMax); distance += 500) {
        const float x = ML + static_cast<float>((distance - xMin) / (xMax - xMin)) * plotW;
        g.DrawLine(&gridPen, PointF(x, MT), PointF(x, MT + plotH));

        wchar_t label[32];
        swprintf(label, 32, L"%d", distance);
        g.DrawString(label, -1, &tickFont, PointF(x, MT + plotH + 10), &sfCenter, &textBrush);
    }

    SolidBrush runwayBrush(Color(200, 136, 148, 168));
    const float groundY = MT + plotH - 4.0f;
    g.FillRectangle(&runwayBrush, ML, groundY, plotW, 4.0f);

    g.DrawString(L"\x9AD8\x5EA6 (m)", -1, &labelFont, PointF(8.0f, MT + plotH * 0.5f - 18.0f), &textBrush);
    g.DrawString(L"\x8DD1\x9053\x65B9\x5411\x8DDD\x79BB (m)", -1, &labelFont,
                 PointF(ML + plotW * 0.5f - 78.0f, MT + plotH + 28.0f), &textBrush);
}

void ProfileView::paint(Graphics& g, int w, int h,
                        const std::vector<FrameData>& history,
                        const FrameData& current,
                        const RunwayInfo& runway,
                        bool approachMode) {
    (void)runway;
    (void)approachMode;

    const float plotW = w - ML - MR;
    const float plotH = h - MT - MB;
    if (plotW <= 0 || plotH <= 0) {
        return;
    }

    const AxisRange axisRange = visibleXRange(current.x);
    const double xMin = axisRange.min;
    const double xMax = axisRange.max;

    drawAxes(g, plotW, plotH, xMin, xMax);

    std::vector<PointF> curvePoints;
    curvePoints.reserve(history.size());

    for (const auto& frame : history) {
        const double x = displayX(frame, runway, approachMode);
        if (x < xMin - 50.0 || x > xMax + 50.0) {
            continue;
        }
        curvePoints.push_back(toScreen(x, frame.altitude, xMin, xMax, plotW, plotH));
    }

    if (curvePoints.size() >= 2) {
        std::vector<PointF> fillPoints;
        fillPoints.reserve(curvePoints.size() + 2);
        fillPoints.push_back(PointF(curvePoints.front().X, MT + plotH));
        fillPoints.insert(fillPoints.end(), curvePoints.begin(), curvePoints.end());
        fillPoints.push_back(PointF(curvePoints.back().X, MT + plotH));

        SolidBrush fill(Color(105, 96, 148, 220));
        g.FillPolygon(&fill, fillPoints.data(), static_cast<INT>(fillPoints.size()));

        Pen curvePen(Color(255, 44, 146, 228), 4.0f);
        curvePen.SetLineJoin(LineJoinRound);
        g.DrawLines(&curvePen, curvePoints.data(), static_cast<INT>(curvePoints.size()));
    }

    const double currentX = displayX(current, runway, approachMode);
    if (currentX >= xMin && currentX <= xMax) {
        const PointF currentPoint = toScreen(currentX, current.altitude, xMin, xMax, plotW, plotH);
        drawUavIcon(g, currentPoint, 10.0f);

        FontFamily ff(L"Microsoft YaHei");
        Font valueFont(&ff, 12, FontStyleRegular, UnitPixel);
        SolidBrush textBrush(Color(235, 240, 240, 245));
        wchar_t currentLabel[64];
        swprintf(currentLabel, 64, L"%.0f m", current.altitude);
        g.DrawString(currentLabel, -1, &valueFont,
                     PointF(currentPoint.X + 14.0f, currentPoint.Y - 20.0f), &textBrush);
    }

    FontFamily ff(L"Microsoft YaHei");
    Font titleFont(&ff, 16, FontStyleBold, UnitPixel);
    SolidBrush titleBrush(Color(255, 236, 240, 248));
    g.DrawString(L"\x8D77\x964D\x5256\x9762\x56FE", -1, &titleFont, PointF(ML, 8.0f), &titleBrush);
}

} // namespace uav
