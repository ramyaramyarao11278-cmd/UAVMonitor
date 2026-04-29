#include "FrameTransmitter.h"
#include "Simulator.h"
#include <QDateTime>
#include <QDebug>

namespace uav {

FrameTransmitter::FrameTransmitter(QObject* parent)
    : QObject(parent)
{
}

FrameTransmitter::~FrameTransmitter()
{
    stop();
}

void FrameTransmitter::start()
{
    if (timer_ && timer_->isActive())
        return;

    if (mode_ == ReadExternalFile) {
        // 模式 B: 打开输入文件
        if (inputPath_.isEmpty()) {
            emit errorOccurred(QStringLiteral("Input file path is empty"));
            return;
        }
        inputFile_ = new QFile(inputPath_, this);
        if (!inputFile_->open(QIODevice::ReadOnly)) {
            emit errorOccurred(QStringLiteral("Cannot open input file: %1").arg(inputPath_));
            delete inputFile_;
            inputFile_ = nullptr;
            return;
        }
        readBuf_.clear();
    } else {
        // 模式 A: 检查 Simulator
        if (!sim_) {
            emit errorOccurred(QStringLiteral("Simulator not set"));
            return;
        }
    }

    timer_ = new QTimer(this);
    timer_->setTimerType(Qt::PreciseTimer);
    connect(timer_, &QTimer::timeout, this, &FrameTransmitter::onTick);
    timer_->start(intervalMs_);
}

void FrameTransmitter::stop()
{
    if (timer_) {
        timer_->stop();
        timer_->deleteLater();
        timer_ = nullptr;
    }
    if (inputFile_) {
        inputFile_->close();
        inputFile_->deleteLater();
        inputFile_ = nullptr;
    }
    readBuf_.clear();
}

void FrameTransmitter::onTick()
{
    if (mode_ == SimulateThenFile) {
        // ---- 模式 A: 仿真 → 打包 → 写文件 → 发信号 ----
        if (!sim_) return;

        // 让仿真器前进一步
        double dt = intervalMs_ / 1000.0;
        sim_->update(dt);

        TelemetryFrame frame = sim_->generateFrame();
        qint64 ts = QDateTime::currentMSecsSinceEpoch();

        QByteArray packed = packFrame(frame, runway_, ts);

        // 追加写入输出文件
        if (!outputPath_.isEmpty()) {
            QFile outFile(outputPath_);
            if (outFile.open(QIODevice::Append)) {
                outFile.write(packed);
                outFile.close();
            } else {
                emit errorOccurred(QStringLiteral("Cannot write to output file: %1").arg(outputPath_));
            }
        }

        emit frameReady(frame, runway_, ts);

    } else {
        // ---- 模式 B: 从文件读帧 → 解包 → 发信号 ----
        if (!inputFile_ || !inputFile_->isOpen()) {
            emit finished();
            stop();
            return;
        }

        // 读取数据到缓冲区
        QByteArray chunk = inputFile_->read(4096);
        if (!chunk.isEmpty()) {
            readBuf_.append(chunk);
        }

        // 尝试在缓冲区中查找帧头同步
        while (readBuf_.size() >= TOTAL_FRAME_SIZE) {
            // 查找帧头 0xEB 0x90 0xEB 0x90
            int headerIdx = -1;
            for (int i = 0; i <= readBuf_.size() - 4; ++i) {
                if (static_cast<quint8>(readBuf_[i])   == 0xEB &&
                    static_cast<quint8>(readBuf_[i+1]) == 0x90 &&
                    static_cast<quint8>(readBuf_[i+2]) == 0xEB &&
                    static_cast<quint8>(readBuf_[i+3]) == 0x90) {
                    headerIdx = i;
                    break;
                }
            }

            if (headerIdx < 0) {
                // 没找到帧头，保留最后 3 字节（可能是不完整帧头）
                readBuf_ = readBuf_.right(3);
                break;
            }

            // 跳过帧头之前的垃圾数据
            if (headerIdx > 0) {
                readBuf_.remove(0, headerIdx);
            }

            // 检查是否有完整帧
            if (readBuf_.size() < TOTAL_FRAME_SIZE) {
                break; // 等更多数据
            }

            // 提取一帧
            QByteArray oneFrame = readBuf_.left(TOTAL_FRAME_SIZE);
            readBuf_.remove(0, TOTAL_FRAME_SIZE);

            TelemetryFrame tm{};
            ProtoRunwayInfo rw;
            qint64 ts = 0;
            QString err;

            if (unpackFrame(oneFrame, tm, rw, ts, &err)) {
                emit frameReady(tm, rw, ts);
            } else {
                emit errorOccurred(QStringLiteral("Unpack error: %1").arg(err));
            }

            // 每个 tick 只处理一帧，模拟实时回放
            return;
        }

        // 缓冲区数据不够且文件读完
        if (chunk.isEmpty() && readBuf_.size() < TOTAL_FRAME_SIZE) {
            emit finished();
            stop();
        }
    }
}

} // namespace uav
