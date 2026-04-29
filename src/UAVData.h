#pragma once
#include <cmath>
#include <vector>

namespace uav {

struct FrameData {
    double time = 0.0;
    double altitude = 0.0;
    double speed = 0.0;
    double verticalSpd = 0.0;
    double attackAngle = 0.0;
    double roll = 0.0;
    double x = 0.0;
    double y = 0.0;
};

struct ParamPanel {
    double windSpeed = 10.0;
    double temperature = 25.0;
    double altitude = 0.0;
    double hSpeed = 0.0;
    double vSpeed = 0.0;
    double batteryVolt = 3.9;
    double signalStrength = 80.0;
    double distToReturn = 500.0;
};

struct RunwayInfo {
    double length = 2000.0;
    double width = 45.0;
    double touchdownX = 300.0;
};

} // namespace uav
