#pragma once
#include <windows.h>
#include <gdiplus.h>
#include "UAVData.h"
#include <vector>

namespace uav {

class ProfileView {
public:
    void paint(Gdiplus::Graphics& g, int w, int h,
               const std::vector<FrameData>& history,
               const FrameData& current,
               const RunwayInfo& runway,
               bool approachMode);

private:
    static constexpr float ML = 72, MR = 22, MT = 42, MB = 54;

    double displayX(const FrameData& frame,
                    const RunwayInfo& runway,
                    bool approachMode) const;

    Gdiplus::PointF toScreen(double x,
                             double altitude,
                             double xMin,
                             double xMax,
                             float plotW,
                             float plotH) const;

    void drawAxes(Gdiplus::Graphics& g,
                  float plotW,
                  float plotH,
                  double xMin,
                  double xMax) const;
};

} // namespace uav
