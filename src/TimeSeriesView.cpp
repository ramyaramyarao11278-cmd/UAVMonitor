#include "TimeSeriesView.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace uav {
using namespace Gdiplus;

namespace {

double altitudeValue(const FrameData& frame) {
    return frame.altitude;
}

double speedValue(const FrameData& frame) {
    return frame.speed;
}

double attackAngleValue(const FrameData& frame) {
    return frame.attackAngle;
}

float toScreenX(double time, double timeMin, double timeMax, const RectF& rect) {
    return rect.X + static_cast<float>((time - timeMin) / (timeMax - timeMin)) * rect.Width;
}

float toScreenY(double value, double yMin, double yMax, const RectF& rect) {
    const double clamped = std::clamp(value, yMin, yMax);
    return rect.Y + rect.Height -
           static_cast<float>((clamped - yMin) / (yMax - yMin)) * rect.Height;
}

double roundUp(double value, double step, double minimumValue) {
    return std::max(minimumValue, std::ceil(value / step) * step);
}

double roundDown(double value, double step, double maximumValue) {
    return std::min(maximumValue, std::floor(value / step) * step);
}

} // namespace

void TimeSeriesView::drawBand(Graphics& g,
                              const RectF& rect,
                              const std::vector<FrameData>& history,
                              const FrameData& current,
                              double timeMin,
                              double timeMax,
                              double yMin,
                              double yMax,
                              ValueFn valueFn,
                              const wchar_t* label,
                              const wchar_t* unit,
                              const Color& lineColor,
                              bool drawTimeLabels) const {
    if (yMax <= yMin) {
        yMax = yMin + 1.0;
    }

    Pen borderPen(Color(150, 82, 92, 112), 1.0f);
    g.DrawRectangle(&borderPen, rect);

    Pen gridPen(Color(45, 112, 126, 152), 1.0f);
    gridPen.SetDashStyle(DashStyleDash);

    FontFamily ff(L"Microsoft YaHei");
    Font tickFont(&ff, 11, FontStyleRegular, UnitPixel);
    Font labelFont(&ff, 12, FontStyleBold, UnitPixel);
    Font valueFont(&ff, 11, FontStyleRegular, UnitPixel);
    SolidBrush textBrush(Color(230, 220, 228, 242));
    SolidBrush valueBrush(Color(240, lineColor.GetR(), lineColor.GetG(), lineColor.GetB()));

    StringFormat sfRight;
    sfRight.SetAlignment(StringAlignmentFar);
    sfRight.SetLineAlignment(StringAlignmentCenter);

    StringFormat sfCenter;
    sfCenter.SetAlignment(StringAlignmentCenter);

    for (int i = 0; i <= 4; ++i) {
        const float y = rect.Y + rect.Height * static_cast<float>(i) / 4.0f;
        g.DrawLine(&gridPen, PointF(rect.X, y), PointF(rect.X + rect.Width, y));

        const double tickValue = yMax - (yMax - yMin) * static_cast<double>(i) / 4.0;
        const int precision = (yMax - yMin <= 20.0) ? 1 : 0;
        wchar_t tickLabel[48];
        swprintf(tickLabel, 48, precision > 0 ? L"%.1f" : L"%.0f", tickValue);
        g.DrawString(tickLabel, -1, &tickFont, PointF(ML - 8.0f, y), &sfRight, &textBrush);
    }

    for (int i = 0; i <= 6; ++i) {
        const double tickTime = timeMin + (timeMax - timeMin) * static_cast<double>(i) / 6.0;
        const float x = toScreenX(tickTime, timeMin, timeMax, rect);
        g.DrawLine(&gridPen, PointF(x, rect.Y), PointF(x, rect.Y + rect.Height));

        if (drawTimeLabels) {
            wchar_t timeLabel[32];
            swprintf(timeLabel, 32, L"%.0f", tickTime);
            g.DrawString(timeLabel, -1, &tickFont,
                         PointF(x, rect.Y + rect.Height + 8.0f), &sfCenter, &textBrush);
        }
    }

    std::vector<PointF> points;
    points.reserve(history.size() + 1);
    for (const auto& frame : history) {
        if (frame.time < timeMin || frame.time > timeMax) {
            continue;
        }

        points.push_back(PointF(
            toScreenX(frame.time, timeMin, timeMax, rect),
            toScreenY(valueFn(frame), yMin, yMax, rect)
        ));
    }

    if (points.empty()) {
        points.push_back(PointF(
            toScreenX(current.time, timeMin, timeMax, rect),
            toScreenY(valueFn(current), yMin, yMax, rect)
        ));
    }

    if (points.size() >= 2) {
        Pen linePen(lineColor, 2.4f);
        linePen.SetLineJoin(LineJoinRound);
        g.DrawLines(&linePen, points.data(), static_cast<INT>(points.size()));
    }

    const double currentValue = valueFn(current);
    const PointF currentPoint(
        toScreenX(std::clamp(current.time, timeMin, timeMax), timeMin, timeMax, rect),
        toScreenY(currentValue, yMin, yMax, rect)
    );

    Pen markerPen(Color(245, 255, 255, 255), 1.0f);
    SolidBrush markerBrush(lineColor);
    g.FillEllipse(&markerBrush, currentPoint.X - 4.0f, currentPoint.Y - 4.0f, 8.0f, 8.0f);
    g.DrawEllipse(&markerPen, currentPoint.X - 4.0f, currentPoint.Y - 4.0f, 8.0f, 8.0f);

    g.DrawString(label, -1, &labelFont, PointF(rect.X + 8.0f, rect.Y + 6.0f), &textBrush);

    const int precision = (yMax - yMin <= 20.0) ? 1 : 0;
    wchar_t currentLabel[96];
    swprintf(currentLabel, 96,
             precision > 0 ? L"\x5F53\x524D %.1f %ls" : L"\x5F53\x524D %.0f %ls",
             currentValue, unit);
    g.DrawString(currentLabel, -1, &valueFont,
                 PointF(rect.X + rect.Width + 10.0f, rect.Y + rect.Height * 0.5f - 8.0f),
                 &valueBrush);
}

void TimeSeriesView::paint(Graphics& g, int w, int h,
                           const std::vector<FrameData>& history,
                           const FrameData& current) {
    const float plotW = w - ML - MR;
    const float plotH = h - MT - MB;
    if (plotW <= 0 || plotH <= 0) {
        return;
    }

    SolidBrush plotBg(Color(255, 25, 28, 36));
    g.FillRectangle(&plotBg, ML, MT, plotW, plotH);

    Pen borderPen(Color(180, 82, 92, 112), 1.0f);
    g.DrawRectangle(&borderPen, ML, MT, plotW, plotH);

    double timeMin = history.empty() ? 0.0 : history.front().time;
    double timeMax = std::max(timeMin + 30.0, current.time);

    double altitudeMax = std::max(20.0, current.altitude);
    double speedMax = std::max(20.0, current.speed);
    double attackAngleMin = std::min(0.0, current.attackAngle);
    double attackAngleMax = std::max(6.0, current.attackAngle);

    for (const auto& frame : history) {
        altitudeMax = std::max(altitudeMax, frame.altitude);
        speedMax = std::max(speedMax, frame.speed);
        attackAngleMin = std::min(attackAngleMin, frame.attackAngle);
        attackAngleMax = std::max(attackAngleMax, frame.attackAngle);
    }

    altitudeMax = roundUp(altitudeMax * 1.1, 50.0, 100.0);
    speedMax = roundUp(speedMax * 1.1, 20.0, 80.0);
    attackAngleMin = roundDown(attackAngleMin - 0.5, 2.0, 0.0);
    attackAngleMax = roundUp(attackAngleMax + 0.5, 2.0, 8.0);
    if (attackAngleMax - attackAngleMin < 6.0) {
        attackAngleMax = attackAngleMin + 6.0;
    }

    const float bandH = (plotH - BandGap * 2.0f) / 3.0f;
    const RectF altitudeRect(ML, MT, plotW, bandH);
    const RectF speedRect(ML, MT + bandH + BandGap, plotW, bandH);
    const RectF angleRect(ML, MT + (bandH + BandGap) * 2.0f, plotW, bandH);

    drawBand(g, altitudeRect, history, current, timeMin, timeMax, 0.0, altitudeMax,
             altitudeValue, L"\x9AD8\x5EA6", L"m", Color(255, 72, 190, 255), false);
    drawBand(g, speedRect, history, current, timeMin, timeMax, 0.0, speedMax,
             speedValue, L"\x901F\x5EA6", L"km/h", Color(255, 116, 223, 146), false);
    drawBand(g, angleRect, history, current, timeMin, timeMax, attackAngleMin, attackAngleMax,
             attackAngleValue, L"\x653B\x89D2", L"\x00B0", Color(255, 255, 166, 87), true);

    FontFamily ff(L"Microsoft YaHei");
    Font titleFont(&ff, 16, FontStyleBold, UnitPixel);
    Font labelFont(&ff, 13, FontStyleBold, UnitPixel);
    SolidBrush titleBrush(Color(255, 236, 240, 248));

    g.DrawString(L"\x9AD8\x5EA6 / \x653B\x89D2 / \x901F\x5EA6 \x65F6\x95F4\x5386\x7A0B", -1,
                 &titleFont, PointF(ML, 8.0f), &titleBrush);
    g.DrawString(L"\x65F6\x95F4 (s)", -1, &labelFont,
                 PointF(ML + plotW * 0.5f - 28.0f, MT + plotH + 26.0f), &titleBrush);
}

} // namespace uav
