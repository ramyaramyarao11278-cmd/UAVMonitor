#include "Simulator.h"
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace uav {

Simulator::Simulator() { reset(); }

void Simulator::reset() {
    cur_ = {};
    cur_.x = 0;
    cur_.y = 0;
    cur_.speed = 0;
    history_.clear();
    phase_ = Phase::Stopped;
    elapsed_ = 0;
}

void Simulator::update(double dt) {
    elapsed_ += dt;
    cur_.time = elapsed_;

    switch (phase_) {
        case Phase::Takeoff:  updateTakeoff(dt);  break;
        case Phase::Climb:    updateClimb(dt);    break;
        case Phase::Cruise:   updateCruise(dt);   break;
        case Phase::Approach: updateApproach(dt); break;
        case Phase::Landing:  updateLanding(dt);  break;
        case Phase::Stopped:  break;
    }

    // 风带来的横向漂移
    std::normal_distribution<double> noise(0, 0.15);
    double windEffect = windSpeed_ / 3.6 * 0.02 * dt;
    cur_.y += windEffect + noise(rng_) * dt;

    // 沿跑道推进
    cur_.x += (cur_.speed / 3.6) * dt;

    // 记录历史
    history_.push_back(cur_);
    if (history_.size() > 5000) {
        history_.erase(history_.begin());
    }
}

void Simulator::updateTakeoff(double dt) {
    cur_.speed += 40.0 * dt;
    cur_.attackAngle = 0;

    if (cur_.speed > 60) {
        cur_.attackAngle = std::min(8.0, (cur_.speed - 60) * 0.5);
        cur_.verticalSpd = cur_.attackAngle * 0.4;
        cur_.altitude += cur_.verticalSpd * dt;
    }

    if (cur_.altitude > 10) {
        phase_ = Phase::Climb;
    }
}

void Simulator::updateClimb(double dt) {
    double targetSpeed = 120;
    double targetAlt   = 200;
    double targetAoA   = 6.0;

    cur_.speed += (targetSpeed - cur_.speed) * 0.3 * dt;
    cur_.attackAngle += (targetAoA - cur_.attackAngle) * 2.0 * dt;

    cur_.verticalSpd = 2.5 + (targetAlt - cur_.altitude) * 0.01;
    cur_.verticalSpd = std::max(0.5, std::min(4.0, cur_.verticalSpd));
    cur_.altitude += cur_.verticalSpd * dt;

    if (cur_.altitude >= targetAlt * 0.95) {
        phase_ = Phase::Cruise;
    }
}

void Simulator::updateCruise(double dt) {
    double targetSpeed = 120;

    cur_.speed += (targetSpeed - cur_.speed) * 0.5 * dt;
    cur_.attackAngle += (2.0 - cur_.attackAngle) * 1.0 * dt;
    cur_.verticalSpd *= 0.9;
    cur_.altitude += cur_.verticalSpd * dt;

    if (cur_.x > 3000) {
        phase_ = Phase::Approach;
    }
}

void Simulator::updateApproach(double dt) {
    double targetSpeed = 60;
    double glideAngle  = 3.0;

    cur_.speed += (targetSpeed - cur_.speed) * 0.3 * dt;
    cur_.attackAngle += (4.0 - cur_.attackAngle) * 1.0 * dt;

    double descentRate = (cur_.speed / 3.6) * std::tan(glideAngle * M_PI / 180.0);
    cur_.verticalSpd = -descentRate;
    cur_.altitude += cur_.verticalSpd * dt;
    cur_.altitude = std::max(0.0, cur_.altitude);

    if (cur_.altitude < 5.0) {
        phase_ = Phase::Landing;
    }
}

void Simulator::updateLanding(double dt) {
    cur_.altitude = std::max(0.0, cur_.altitude - 1.0 * dt);
    if (cur_.altitude <= 0) cur_.altitude = 0;

    cur_.speed -= 30.0 * dt;
    cur_.speed = std::max(0.0, cur_.speed);
    cur_.attackAngle *= 0.95;
    cur_.verticalSpd = 0;

    if (cur_.speed < 1.0) {
        cur_.speed = 0;
        phase_ = Phase::Stopped;
    }
}

void Simulator::setWind(double speed) {
    windSpeed_ = speed;
}

void Simulator::setPhase(Phase p) {
    phase_ = p;
}

ParamPanel Simulator::getParams() const {
    ParamPanel p;
    p.windSpeed      = windSpeed_;
    p.temperature    = 25.0;
    p.altitude       = cur_.altitude;
    p.hSpeed         = cur_.speed;
    p.vSpeed         = cur_.verticalSpd;
    p.batteryVolt    = 3.9 - elapsed_ * 0.001;
    p.signalStrength = std::max(10.0, 95.0 - cur_.altitude * 0.05);
    p.distToReturn   = std::sqrt(cur_.x * cur_.x + cur_.altitude * cur_.altitude);
    return p;
}

} // namespace uav
