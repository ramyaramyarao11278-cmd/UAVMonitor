#pragma once
#include <QObject>
#include <QTimer>
#include <QFile>
#include "TelemetryFrame.h"
#include "Protocol.h"

namespace uav {

class Simulator; // 前向声明

/// 帧发送器 —— 支持两种模式：
///   A) SimulateThenFile: Simulator 生成仿真数据 → 打包写文件 → 发信号
///   B) ReadExternalFile: 从外部二进制文件按帧读取 → 解包 → 发信号
class FrameTransmitter : public QObject {
    Q_OBJECT
public:
    enum Mode { SimulateThenFile, ReadExternalFile };

    explicit FrameTransmitter(QObject* parent = nullptr);
    ~FrameTransmitter();

    void setMode(Mode m)              { mode_ = m; }
    void setIntervalMs(int ms)        { intervalMs_ = ms; }
    void setOutputFile(const QString& path) { outputPath_ = path; }
    void setInputFile(const QString& path)  { inputPath_ = path; }
    void setRunway(const ProtoRunwayInfo& rw) { runway_ = rw; }
    void setSimulator(Simulator* sim)  { sim_ = sim; }

    bool isRunning() const { return timer_ && timer_->isActive(); }

public slots:
    void start();
    void stop();

signals:
    /// 每成功处理一帧后发出
    void frameReady(uav::TelemetryFrame tm, uav::ProtoRunwayInfo rw, qint64 timestampMs);
    void errorOccurred(QString msg);
    void finished();

private slots:
    void onTick();

private:
    Mode mode_ = SimulateThenFile;
    int  intervalMs_ = 200;

    QString outputPath_;
    QString inputPath_;
    ProtoRunwayInfo runway_;

    QTimer*     timer_ = nullptr;
    Simulator*  sim_   = nullptr;

    // 模式 B 用
    QFile*   inputFile_ = nullptr;
    QByteArray readBuf_;
};

} // namespace uav
