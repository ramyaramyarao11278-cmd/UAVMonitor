#pragma once
#include "UAVData.h"
#include "TelemetryFrame.h"
#include <random>
#include <vector>

namespace uav {

enum class Phase { Takeoff, Climb, Cruise, Approach, Landing, Stopped };

class Simulator {
public:
    Simulator();

    void reset();
    void update(double dt);
    void setWind(double speed);
    void setPhase(Phase p);

    const std::vector<FrameData>& history() const { return history_; }
    const FrameData& current() const { return cur_; }
    ParamPanel getParams() const;
    Phase phase() const { return phase_; }
    double elapsed() const { return elapsed_; }

    /// 根据当前仿真状态生成一帧完整遥测数据
    TelemetryFrame generateFrame();

private:
    void updateTakeoff(double dt);
    void updateClimb(double dt);
    void updateCruise(double dt);
    void updateApproach(double dt);
    void updateLanding(double dt);

    FrameData cur_;
    std::vector<FrameData> history_;
    Phase phase_ = Phase::Stopped;
    double elapsed_ = 0.0;
    double windSpeed_ = 10.0;
    RunwayInfo runway_;
    std::mt19937 rng_{std::random_device{}()};
};

} // namespace uav
