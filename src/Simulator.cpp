#include "Simulator.h"
#include <algorithm>
#include <cstring>
#include <cmath>
#include <limits>

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

    // 非巡航/进近阶段：roll 缓慢回 0
    if (phase_ != Phase::Cruise && phase_ != Phase::Approach) {
        cur_.roll *= 0.95;
    }

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

    // 巡航阶段 roll 模拟阵风晃动 ±5°
    cur_.roll = 5.0 * std::sin(elapsed_ * 0.3);

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

    // 进近阶段 roll 模拟修正 ±3°
    cur_.roll = 3.0 * std::sin(elapsed_ * 0.5);

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

// 编码辅助：物理值 → 存储值（带 clamp 保护，防止溢出）
template<typename T>
static T enc(double phys, double res, double offset = 0.0) {
    double v = std::round((phys - offset) / res);
    constexpr double maxV = static_cast<double>(std::numeric_limits<T>::max());
    constexpr double minV = static_cast<double>(std::numeric_limits<T>::min());
    if (v > maxV) v = maxV;
    if (v < minV) v = minV;
    return static_cast<T>(v);
}

TelemetryFrame Simulator::generateFrame()
{
    TelemetryFrame f{};
    std::normal_distribution<double> noise01(0, 0.1);
    double t = elapsed_;

    // --- 姿态 ---
    f.angleOfAttack = enc<int16_t>(cur_.attackAngle + noise01(rng_), 0.005);
    double yawDeg   = std::fmod(std::abs(cur_.x) * 0.01, 360.0);
    f.yaw           = enc<int16_t>(yawDeg, 0.001);
    f.pitch         = enc<int16_t>(cur_.attackAngle * 0.5 + noise01(rng_), 0.005);
    f.roll          = enc<int16_t>(cur_.roll + noise01(rng_), 0.001);
    double trackDeg = std::fmod(yawDeg + 2.0 * std::sin(t * 0.3), 360.0);
    if (trackDeg < 0) trackDeg += 360.0;
    f.trackAngle    = enc<int16_t>(trackDeg, 0.001);

    // --- 高度 / 速度 ---
    f.altitude      = enc<uint16_t>(cur_.altitude, 0.5, -500.0);
    f.trueAirspeed  = enc<uint16_t>(cur_.speed / 3.6, 0.01);
    f.groundSpeed   = enc<int16_t>((cur_.speed / 3.6) + noise01(rng_), 0.01);
    f.relFieldHeight = enc<uint16_t>(cur_.altitude, 0.5);
    f.pressureAlt   = enc<uint16_t>(cur_.altitude + 5.0, 0.5, -500.0);

    // --- 遥控源码 ---
    f.rcSource1 = 0x01;
    f.rcSource2 = 0x00;

    // --- 位置 (以北京首都机场附近为参考) ---
    double baseLon = 116.5874 + cur_.x * 0.00001;
    double baseLat = 40.0725  + cur_.y * 0.00001;
    f.longitude = enc<int32_t>(baseLon, 0.005);
    f.latitude  = enc<int32_t>(baseLat, 0.001);

    // --- 速度分量 ---
    double speedMs = cur_.speed / 3.6;
    f.velNorth = enc<int16_t>(speedMs * std::cos(yawDeg * M_PI / 180.0), 0.01);
    f.velEast  = enc<int16_t>(speedMs * std::sin(yawDeg * M_PI / 180.0), 0.01);
    f.velUp    = enc<int16_t>(cur_.verticalSpd, 0.005);

    // --- 角速度 (正弦扰动) ---
    f.pitchRate = enc<int8_t>(2.0 * std::sin(t * 1.1) + noise01(rng_), 0.5);
    f.rollRate  = enc<int8_t>(1.5 * std::sin(t * 0.7) + noise01(rng_), 0.5);
    f.yawRate   = enc<int8_t>(1.0 * std::sin(t * 0.5) + noise01(rng_), 0.5);

    // --- 加速度 ---
    f.accNorth = enc<int16_t>(0.3 * std::sin(t * 0.8) + noise01(rng_), 0.005);
    f.accEast  = enc<int16_t>(0.2 * std::cos(t * 0.6) + noise01(rng_), 0.005);
    f.accUp    = enc<int16_t>(cur_.verticalSpd * 0.1, 0.005);

    // --- 舵面 ---
    f.elevatorL  = enc<int16_t>(-cur_.attackAngle * 0.3 + noise01(rng_), 0.005);
    f.elevatorR  = enc<int16_t>(-cur_.attackAngle * 0.3 + noise01(rng_), 0.005);
    f.rudderL    = enc<int16_t>(1.0 * std::sin(t * 0.4), 0.001);
    f.rudderR    = enc<int16_t>(-1.0 * std::sin(t * 0.4), 0.001);
    f.flapL      = enc<int8_t>(phase_ == Phase::Landing ? 15.0 : 0.0, 0.5);
    f.flapR      = enc<int8_t>(phase_ == Phase::Landing ? 15.0 : 0.0, 0.5);
    f.aileronL   = enc<int16_t>(cur_.roll * 0.2, 0.005);
    f.aileronR   = enc<int16_t>(-cur_.roll * 0.2, 0.005);
    f.waterRudder = enc<int16_t>(0.0, 0.005);
    f.engineTilt  = enc<int16_t>(0.0, 0.005);

    // --- 起落架 / 发动机运行状态 ---
    f.landingGearStatus = (phase_ == Phase::Landing || phase_ == Phase::Takeoff) ? 1 : 0;
    f.engineRunStatus   = (phase_ != Phase::Stopped) ? 1 : 0;

    // --- 液压 ---
    std::normal_distribution<double> oilNoise(0, 0.5);
    f.oilPump1Pressure = enc<int16_t>(150.0 + oilNoise(rng_), 0.02);
    f.oilPump2Pressure = enc<int16_t>(148.0 + oilNoise(rng_), 0.02);

    // --- 发动机 ---
    std::normal_distribution<double> rpmNoise(0, 20.0);
    f.engineRpm       = static_cast<uint16_t>(std::clamp(3000.0 + rpmNoise(rng_), 0.0, 6500.0));
    f.engineThrottle  = (phase_ == Phase::Cruise) ? 75 : (phase_ == Phase::Takeoff ? 100 : 40);
    f.localThrottle   = f.engineThrottle;
    f.exhaust1Temp    = enc<int16_t>(180.0 + 5.0 * std::sin(t * 0.2), 0.01);
    f.exhaust2Temp    = enc<int16_t>(178.0 + 5.0 * std::cos(t * 0.2), 0.01);
    f.cylinderTempL   = enc<int16_t>(200.0 + 3.0 * std::sin(t * 0.3), 0.01);
    f.cylinderTempR   = enc<int16_t>(198.0 + 3.0 * std::cos(t * 0.3), 0.01);
    f.tcuRpm          = f.engineRpm;

    std::normal_distribution<double> voltNoise(0, 0.02);
    f.engineBattVolt1 = enc<uint16_t>(24.0 + voltNoise(rng_), 0.01);
    f.engineBattVolt2 = enc<uint16_t>(24.0 + voltNoise(rng_), 0.01);
    f.oilTemp         = enc<int16_t>(85.0 + 2.0 * std::sin(t * 0.1), 0.01);
    f.engineStatus      = (phase_ != Phase::Stopped) ? 0x01 : 0x00;
    f.engineFaultStatus = 0x00;

    // --- 导航传感器状态 ---
    f.gps1Status       = 0x03; // 正常
    f.gps2Status       = 0x03;
    f.beidouStatus     = 0x03;
    f.radioAltStatus   = 0x01;
    f.groundDiffStatus = 0x01;
    f.insStatus        = 0x01;
    f.diffSatCount     = 12;

    // --- 高度计 ---
    f.altimeterHeight = enc<uint16_t>(cur_.altitude, 0.5);
    f.diffHeight      = enc<uint16_t>(cur_.altitude + 1.0, 0.5);

    // --- 位置源 ---
    f.posSourceId     = 1;
    f.sourceLongitude = f.longitude;
    f.sourceLatitude  = f.latitude;

    // --- 任务时间 ---
    f.missionTime = static_cast<uint32_t>(elapsed_ * 1000);

    // --- 传感器故障 ---
    f.sensorFaultStatus = 0x00;

    // --- 飞控供电 ---
    f.fcuVoltage      = enc<uint16_t>(24.0 + voltNoise(rng_), 0.01);
    f.battery1Voltage = enc<uint16_t>(24.2 + voltNoise(rng_), 0.01);
    f.battery2Voltage = enc<uint16_t>(24.1 + voltNoise(rng_), 0.01);

    // --- 控制状态 ---
    f.controlStatus   = 0x01;
    f.flightPhase     = static_cast<uint8_t>(phase_);
    f.ctrlLawVersion  = 3;
    f.fcuLogicVersion = 7;

    // --- 扩展字 ---
    f.engineCtrlStatus[0] = 0x00;
    f.engineCtrlStatus[1] = 0x00;
    memset(f.fcuSysWord, 0, 4);
    memset(f.fcuFaultWord, 0, 4);

    return f;
}

} // namespace uav
