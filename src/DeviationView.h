#pragma once
#include <windows.h>
#include <gdiplus.h>
#include "UAVData.h"
#include <vector>

namespace uav {

class DeviationView {
public:
    void paint(Gdiplus::Graphics& g, int w, int h,
               const std::vector<FrameData>& history,
               const FrameData& current,
               const RunwayInfo& runway,
               bool approachMode);

private:
    double displayX(const FrameData& frame,
                    const RunwayInfo& runway,
                    bool approachMode) const;
};

} // namespace uav
