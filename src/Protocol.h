#pragma once
#include <QString>
#include <QByteArray>
#include "TelemetryFrame.h"

namespace uav {

// 跑道信息（协议帧中的跑道名称、经纬度）
struct ProtoRunwayInfo {
    QString name;       // 最长 16 字节 ASCII
    double  lonDeg = 0; // 经度（度）
    double  latDeg = 0; // 纬度（度）
};

// 帧头常量
static constexpr quint32 FRAME_HEADER = 0xEB90EB90;

/// 打包一帧完整二进制数据（含帧头、长度、时间戳、跑道、遥测数据体、MD5）
QByteArray packFrame(const TelemetryFrame& tm,
                     const ProtoRunwayInfo& rw,
                     qint64 timestampMs);

/// 解包一帧二进制数据，校验帧头/长度/MD5
/// 返回 false 表示解包失败，errMsg 指出原因
bool unpackFrame(const QByteArray& frame,
                 TelemetryFrame& tm,
                 ProtoRunwayInfo& rw,
                 qint64& timestampMs,
                 QString* errMsg = nullptr);

/// 帧头大小（4 字节帧头 + 2 字节长度）
static constexpr int HEADER_SIZE = 6;

/// 固定帧头部分（时间戳 + 跑道名 + 跑道经纬度）
static constexpr int FIXED_META_SIZE = 8 + 16 + 4 + 4; // = 32

/// MD5 校验长度
static constexpr int MD5_SIZE = 16;

/// 完整帧总长度
static constexpr int TOTAL_FRAME_SIZE =
    4 + 2 + FIXED_META_SIZE + static_cast<int>(sizeof(TelemetryFrame)) + MD5_SIZE;

} // namespace uav
