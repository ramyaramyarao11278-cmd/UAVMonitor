#pragma once
#include <windows.h>
#include <gdiplus.h>
#include <vector>

#include "UAVData.h"

namespace uav {

class TimeSeriesView {
public:
    void paint(Gdiplus::Graphics& g, int w, int h,
               const std::vector<FrameData>& history,
               const FrameData& current);

private:
    using ValueFn = double (*)(const FrameData&);

    static constexpr float ML = 76.0f;
    static constexpr float MR = 84.0f;
    static constexpr float MT = 48.0f;
    static constexpr float MB = 52.0f;
    static constexpr float BandGap = 14.0f;

    void drawBand(Gdiplus::Graphics& g,
                  const Gdiplus::RectF& rect,
                  const std::vector<FrameData>& history,
                  const FrameData& current,
                  double timeMin,
                  double timeMax,
                  double yMin,
                  double yMax,
                  ValueFn valueFn,
                  const wchar_t* label,
                  const wchar_t* unit,
                  const Gdiplus::Color& lineColor,
                  bool drawTimeLabels) const;
};

} // namespace uav
