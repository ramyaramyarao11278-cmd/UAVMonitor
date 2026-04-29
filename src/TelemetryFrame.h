#pragma once
// 总长度 135 字节，由各字段累加得出（#pragma pack(1) 紧凑打包）
#include <cstdint>

namespace uav {

// 遥测数据帧 ── 按协议"遥测字段表"顺序紧凑打包
// 物理值 = 存储值 × 分辨率 + offset
#pragma pack(push, 1)
struct TelemetryFrame {
    // --- 姿态 ---
    int16_t  angleOfAttack;      // 攻角           0.005°
    int16_t  yaw;                // 偏航角          0.001°
    int16_t  pitch;              // 俯仰角          0.005°
    int16_t  roll;               // 滚转角          0.001°
    int16_t  trackAngle;         // 航迹角          0.001°

    // --- 高度/速度 ---
    uint16_t altitude;           // 海拔高度 0.5m  offset=-500
    uint16_t trueAirspeed;       // 真空速   0.01 m/s
    int16_t  groundSpeed;        // 地速     0.01 m/s
    uint16_t relFieldHeight;     // 相对场高 0.5m
    uint16_t pressureAlt;        // 气压高度 0.5m  offset=-500

    // --- 遥控源码 ---
    uint8_t  rcSource1;          // 遥控源码1区
    uint8_t  rcSource2;          // 遥控源码2区

    // --- 位置 ---
    int32_t  longitude;          // 经度  0.005°
    int32_t  latitude;           // 纬度  0.001°

    // --- 速度分量 ---
    int16_t  velNorth;           // 北向速度   0.01 m/s
    int16_t  velEast;            // 东向速度   0.01 m/s
    int16_t  velUp;              // 天向速度   0.005 m/s

    // --- 角速度 ---
    int8_t   pitchRate;          // 俯仰角速度  0.5 °/s
    int8_t   rollRate;           // 滚转角速度  0.5 °/s
    int8_t   yawRate;            // 偏航角速度  0.5 °/s

    // --- 加速度 ---
    int16_t  accNorth;           // 北向加速度  0.005 m/s²
    int16_t  accEast;            // 东向加速度  0.005 m/s²
    int8_t   accUp;              // 天向加速度  0.005 m/s²

    // --- 舵面 ---
    int16_t  elevatorL;          // 左升降舵  0.005°
    int16_t  elevatorR;          // 右升降舵  0.005°
    int16_t  rudderL;            // 方向舵左  0.001°
    int16_t  rudderR;            // 方向舵右  0.001°
    int8_t   flapL;              // 襟翼左    0.5°
    int8_t   flapR;              // 襟翼右    0.5°
    int16_t  aileronL;           // 副翼左    0.005°
    int16_t  aileronR;           // 副翼右    0.005°
    int16_t  waterRudder;        // 水舵      0.005°
    int16_t  engineTilt;         // 发动机倾转 0.005°

    // --- 起落架 / 发动机状态 ---
    uint8_t  landingGearStatus;
    uint8_t  engineRunStatus;

    // --- 液压 ---
    int16_t  oilPump1Pressure;   // 油泵1压力  0.02 Bar
    int16_t  oilPump2Pressure;   // 油泵2压力  0.02 Bar

    // --- 发动机 ---
    uint16_t engineRpm;          // 转速 rpm
    uint8_t  engineThrottle;     // 风门 %
    uint8_t  localThrottle;      // 本地风门 %
    int16_t  exhaust1Temp;       // 排气1温度 0.01℃
    int16_t  exhaust2Temp;       // 排气2温度 0.01℃
    int16_t  cylinderTempL;      // 缸温左   0.01℃
    int16_t  cylinderTempR;      // 缸温右   0.01℃
    uint16_t tcuRpm;             // TCU 转速 rpm
    uint16_t engineBattVolt1;    // 发动机电池1 0.01V
    uint16_t engineBattVolt2;    // 发动机电池2 0.01V
    int16_t  oilTemp;            // 滑油温度 0.01℃
    uint8_t  engineStatus;
    uint8_t  engineFaultStatus;

    // --- 导航传感器状态 ---
    uint8_t  gps1Status;
    uint8_t  gps2Status;
    uint8_t  beidouStatus;
    uint8_t  radioAltStatus;
    uint8_t  groundDiffStatus;
    uint8_t  insStatus;
    uint8_t  diffSatCount;       // 差分卫星数

    // --- 高度计 ---
    uint16_t altimeterHeight;    // 无线电高度 0.5m
    uint16_t diffHeight;         // 差分高度   0.5m

    // --- 位置源 ---
    uint8_t  posSourceId;
    int32_t  sourceLongitude;    // 位置源经度 0.005°
    int32_t  sourceLatitude;     // 位置源纬度 0.001°

    // --- 任务时间 ---
    uint32_t missionTime;        // ms

    // --- 传感器故障 ---
    uint8_t  sensorFaultStatus;

    // --- 飞控供电 ---
    uint16_t fcuVoltage;         // 飞控机电压 0.01V
    uint16_t battery1Voltage;    // 电池1 0.01V
    uint16_t battery2Voltage;    // 电池2 0.01V

    // --- 控制状态 ---
    uint8_t  controlStatus;
    uint8_t  flightPhase;
    uint8_t  ctrlLawVersion;
    uint8_t  fcuLogicVersion;

    // --- 扩展字 ---
    uint8_t  engineCtrlStatus[2];   // 发动机控制状态
    uint8_t  fcuSysWord[4];         // 飞控系统状态字
    uint8_t  fcuFaultWord[4];       // 飞控系统故障字
};
#pragma pack(pop)

// 编译期校验 TelemetryFrame 紧凑大小 = 135 字节
static_assert(sizeof(TelemetryFrame) == 135,
              "TelemetryFrame size mismatch -- check field types and packing");

} // namespace uav
