#include "Protocol.h"
#include <QCryptographicHash>
#include <QDataStream>
#include <QtEndian>
#include <cstring>
#include <cmath>

namespace uav {

// ---- 编码/解码辅助 ----
// 物理值 → 存储值:  stored = (physical - offset) / resolution
template<typename T>
static T encode(double physical, double resolution, double offset = 0.0)
{
    return static_cast<T>(std::round((physical - offset) / resolution));
}

// 存储值 → 物理值:  physical = stored * resolution + offset
template<typename T>
static double decode(T stored, double resolution, double offset = 0.0)
{
    return static_cast<double>(stored) * resolution + offset;
}

// ---- 小端写入辅助 ----
static void writeLE(QByteArray& buf, quint8  v) { buf.append(static_cast<char>(v)); }
static void writeLE(QByteArray& buf, qint8   v) { buf.append(static_cast<char>(v)); }
static void writeLE(QByteArray& buf, quint16 v) { v = qToLittleEndian(v); buf.append(reinterpret_cast<const char*>(&v), 2); }
static void writeLE(QByteArray& buf, qint16  v) { v = qToLittleEndian(v); buf.append(reinterpret_cast<const char*>(&v), 2); }
static void writeLE(QByteArray& buf, quint32 v) { v = qToLittleEndian(v); buf.append(reinterpret_cast<const char*>(&v), 4); }
static void writeLE(QByteArray& buf, qint32  v) { v = qToLittleEndian(v); buf.append(reinterpret_cast<const char*>(&v), 4); }
static void writeLE(QByteArray& buf, quint64 v) { v = qToLittleEndian(v); buf.append(reinterpret_cast<const char*>(&v), 8); }

// ---- 小端读取辅助 ----
static int readPos = 0; // 线程不安全，仅在单次 unpack 内使用

static quint8  readU8 (const char* d) { quint8  v = static_cast<quint8>(d[readPos]); readPos += 1; return v; }
static qint8   readI8 (const char* d) { qint8   v = static_cast<qint8>(d[readPos]);  readPos += 1; return v; }
static quint16 readU16(const char* d) { quint16 v; memcpy(&v, d + readPos, 2); v = qFromLittleEndian(v); readPos += 2; return v; }
static qint16  readI16(const char* d) { qint16  v; memcpy(&v, d + readPos, 2); v = qFromLittleEndian(v); readPos += 2; return v; }
static quint32 readU32(const char* d) { quint32 v; memcpy(&v, d + readPos, 4); v = qFromLittleEndian(v); readPos += 4; return v; }
static qint32  readI32(const char* d) { qint32  v; memcpy(&v, d + readPos, 4); v = qFromLittleEndian(v); readPos += 4; return v; }
static quint64 readU64(const char* d) { quint64 v; memcpy(&v, d + readPos, 8); v = qFromLittleEndian(v); readPos += 8; return v; }

// ================================================================
//  packFrame — 按协议格式打包一帧
// ================================================================
QByteArray packFrame(const TelemetryFrame& tm,
                     const ProtoRunwayInfo& rw,
                     qint64 timestampMs)
{
    QByteArray buf;
    buf.reserve(TOTAL_FRAME_SIZE);

    // ---- 帧头 4B ----
    writeLE(buf, static_cast<quint8>(0xEB));
    writeLE(buf, static_cast<quint8>(0x90));
    writeLE(buf, static_cast<quint8>(0xEB));
    writeLE(buf, static_cast<quint8>(0x90));

    // ---- 帧长 2B (数据体 + MD5 的总长度) ----
    quint16 payloadLen = static_cast<quint16>(FIXED_META_SIZE + sizeof(TelemetryFrame) + MD5_SIZE);
    writeLE(buf, payloadLen);

    // ---- 时间戳 8B ----
    writeLE(buf, static_cast<quint64>(timestampMs));

    // ---- 跑道名称 16B (ASCII, 不足补 0) ----
    QByteArray nameBytes = rw.name.toLatin1().left(16);
    nameBytes.resize(16, '\0');
    buf.append(nameBytes);

    // ---- 跑道经度 4B (Int32, 1e-7 度) ----
    qint32 rwLon = static_cast<qint32>(std::round(rw.lonDeg * 1e7));
    writeLE(buf, rwLon);

    // ---- 跑道纬度 4B (Int32, 1e-7 度) ----
    qint32 rwLat = static_cast<qint32>(std::round(rw.latDeg * 1e7));
    writeLE(buf, rwLat);

    // ---- 遥测数据体 (直接写 POD 结构体，已是小端打包) ----
    buf.append(reinterpret_cast<const char*>(&tm), sizeof(TelemetryFrame));

    // ---- MD5 16B (对前面所有字节求 MD5) ----
    QByteArray md5 = QCryptographicHash::hash(buf, QCryptographicHash::Md5);
    buf.append(md5);

    return buf;
}

// ================================================================
//  unpackFrame — 解包一帧
// ================================================================
bool unpackFrame(const QByteArray& frame,
                 TelemetryFrame& tm,
                 ProtoRunwayInfo& rw,
                 qint64& timestampMs,
                 QString* errMsg)
{
    auto fail = [&](const QString& msg) -> bool {
        if (errMsg) *errMsg = msg;
        return false;
    };

    // 最小长度检查
    if (frame.size() < TOTAL_FRAME_SIZE) {
        return fail(QStringLiteral("Frame too short: %1 < %2")
                    .arg(frame.size()).arg(TOTAL_FRAME_SIZE));
    }

    const char* d = frame.constData();
    readPos = 0;

    // ---- 帧头校验 ----
    quint8 h0 = readU8(d), h1 = readU8(d), h2 = readU8(d), h3 = readU8(d);
    if (h0 != 0xEB || h1 != 0x90 || h2 != 0xEB || h3 != 0x90) {
        return fail(QStringLiteral("Invalid frame header"));
    }

    // ---- 帧长 ----
    quint16 payloadLen = readU16(d);
    int expectedPayload = FIXED_META_SIZE + static_cast<int>(sizeof(TelemetryFrame)) + MD5_SIZE;
    if (payloadLen != expectedPayload) {
        return fail(QStringLiteral("Payload length mismatch: %1 != %2")
                    .arg(payloadLen).arg(expectedPayload));
    }

    // 总长度再验
    if (frame.size() < 4 + 2 + payloadLen) {
        return fail(QStringLiteral("Frame truncated"));
    }

    // ---- MD5 校验 ----
    int dataEndPos = 4 + 2 + FIXED_META_SIZE + static_cast<int>(sizeof(TelemetryFrame));
    QByteArray dataForMd5 = frame.left(dataEndPos);
    QByteArray expectedMd5 = QCryptographicHash::hash(dataForMd5, QCryptographicHash::Md5);
    QByteArray actualMd5 = frame.mid(dataEndPos, MD5_SIZE);
    if (expectedMd5 != actualMd5) {
        return fail(QStringLiteral("MD5 checksum mismatch"));
    }

    // ---- 时间戳 ----
    timestampMs = static_cast<qint64>(readU64(d));

    // ---- 跑道名称 ----
    rw.name = QString::fromLatin1(d + readPos, 16).trimmed();
    // 去掉末尾的 \0
    rw.name = rw.name.left(rw.name.indexOf(QChar('\0')));
    if (rw.name.isEmpty()) {
        rw.name = QString::fromLatin1(d + readPos, 16);
        rw.name.remove(QChar('\0'));
    }
    readPos += 16;

    // ---- 跑道经纬度 ----
    qint32 rwLon = readI32(d);
    qint32 rwLat = readI32(d);
    rw.lonDeg = rwLon / 1e7;
    rw.latDeg = rwLat / 1e7;

    // ---- 遥测数据体 ----
    memcpy(&tm, d + readPos, sizeof(TelemetryFrame));

    return true;
}

} // namespace uav
