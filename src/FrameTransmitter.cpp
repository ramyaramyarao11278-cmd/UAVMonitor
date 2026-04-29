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
        // 模式 A: 检查 Simulator，并持久化打开输出文件
        if (!sim_) {
            emit errorOccurred(QStringLiteral("Simulator not set"));
            return;
        }
        if (!outputPath_.isEmpty()) {
            outputFile_ = new QFile(outputPath_, this);
            if (!outputFile_->open(QIODevice::Append | QIODevice::WriteOnly)) {
                emit errorOccurred(QStringLiteral("Cannot open output file: %1").arg(outputPath_));
                delete outputFile_;
                outputFile_ = nullptr;
                return;
            }
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
    if (outputFile_) {
        outputFile_->close();
        outputFile_->deleteLater();
        outputFile_ = nullptr;
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
        // ---- 模式 A: 仅 generateFrame → 打包 → 写文件 → 发信号 ----
        // 注意：不调用 sim_->update()，Simulator 由 MainWindow 的主 timer 驱动
        if (!sim_) return;

        TelemetryFrame frame = sim_->generateFrame();
        qint64 ts = QDateTime::currentMSecsSinceEpoch();

        QByteArray packed = packFrame(frame, runway_, ts);

        // 写入已持久化打开的输出文件
        if (outputFile_ && outputFile_->isOpen()) {
            outputFile_->write(packed);
            outputFile_->flush();
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

        // 第一步：帧头同步（独立于是否够一帧）
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
            if (readBuf_.size() > 3)
                readBuf_ = readBuf_.right(3);
            // 文件已读完且找不到帧头
            if (chunk.isEmpty()) {
                emit finished();
                stop();
            }
            return;
        }

        // 跳过帧头之前的垃圾数据
        if (headerIdx > 0) {
            readBuf_.remove(0, headerIdx);
        }

        // 第二步：检查是否有完整帧
        if (readBuf_.size() < TOTAL_FRAME_SIZE) {
            // 不够一帧，等下个 tick 继续读
            if (chunk.isEmpty()) {
                emit finished();
                stop();
            }
            return;
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
    }
}

} // namespace uav
