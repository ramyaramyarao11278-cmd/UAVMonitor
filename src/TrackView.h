#pragma once
#include <windows.h>
#include <gdiplus.h>
#include "UAVData.h"
#include <vector>

namespace uav {

class TrackView {
public:
    void paint(Gdiplus::Graphics& g, int w, int h,
               const std::vector<FrameData>& history,
               const FrameData& current,
               const RunwayInfo& runway);
};

} // namespace uav
