#include "MainWindow.h"
#include "GDIPlusManager.h"
#include <QFileDialog>
#include <QFrame>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QSplitter>
#include <QVBoxLayout>
#include <timeapi.h>
#pragma comment(lib, "winmm.lib")

namespace uav {

static const char* phaseStr(Phase p) {
    switch (p) {
        case Phase::Stopped:  return "停止";
        case Phase::Takeoff:  return "起飞滑跑";
        case Phase::Climb:    return "爬升";
        case Phase::Cruise:   return "巡航";
        case Phase::Approach: return "进近";
        case Phase::Landing:  return "着陆减速";
    }
    return "未知";
}

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    timeBeginPeriod(1);
    GDIPlusManager::instance();
    setupUI();
    timer_ = new QTimer(this);
    connect(timer_, &QTimer::timeout, this, &MainWindow::onTick);
    transmitter_ = new FrameTransmitter(this);
    transmitter_->setSimulator(&sim_);
    connect(transmitter_, &FrameTransmitter::frameReady, this, &MainWindow::onFrameReady);
    connect(transmitter_, &FrameTransmitter::errorOccurred, this, &MainWindow::onTransmitError);
    connect(transmitter_, &FrameTransmitter::finished, this, [this](){
        transmitBtn_->setText(QStringLiteral("开始传输"));
    });
}

MainWindow::~MainWindow() { timeEndPeriod(1); }

void MainWindow::setupUI() {
    setWindowTitle(QStringLiteral("固定翼无人机起降监控系统"));
    resize(1560, 1000);
    setMinimumSize(1240, 800);

    QWidget* central = new QWidget();
    setCentralWidget(central);
    QVBoxLayout* rootLayout = new QVBoxLayout(central);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    // ---- 顶部 InfoBar ----
    infoBar_ = new InfoBar();
    rootLayout->addWidget(infoBar_);

    // ---- 主体区域 ----
    QWidget* bodyWidget = new QWidget();
    QHBoxLayout* mainLayout = new QHBoxLayout(bodyWidget);
    mainLayout->setContentsMargins(6, 6, 6, 6);
    mainLayout->setSpacing(6);

    QSplitter* leftSplit = new QSplitter(Qt::Vertical);
    profileWidget_ = new GDIWidget();
    profileWidget_->setPaintFn([this](Gdiplus::Graphics& g, int w, int h) {
        const bool approachMode = sim_.phase() == Phase::Approach || sim_.phase() == Phase::Landing;
        profileView_.paint(g, w, h, sim_.history(), sim_.current(), runway_, approachMode);
    });
    profileWidget_->setMinimumHeight(230);

    timeSeriesWidget_ = new GDIWidget();
    timeSeriesWidget_->setPaintFn([this](Gdiplus::Graphics& g, int w, int h) {
        timeSeriesView_.paint(g, w, h, sim_.history(), sim_.current());
    });
    timeSeriesWidget_->setMinimumHeight(230);

    deviationWidget_ = new GDIWidget();
    deviationWidget_->setPaintFn([this](Gdiplus::Graphics& g, int w, int h) {
        const bool approachMode = sim_.phase() == Phase::Approach || sim_.phase() == Phase::Landing;
        deviationView_.paint(g, w, h, sim_.history(), sim_.current(), runway_, approachMode);
    });
    deviationWidget_->setMinimumHeight(210);

    leftSplit->addWidget(profileWidget_);
    leftSplit->addWidget(timeSeriesWidget_);
    leftSplit->addWidget(deviationWidget_);
    leftSplit->setStretchFactor(0, 3);
    leftSplit->setStretchFactor(1, 3);
    leftSplit->setStretchFactor(2, 2);
    mainLayout->addWidget(leftSplit, 11);

    QSplitter* rightSplit = new QSplitter(Qt::Vertical);
    trackWidget_ = new GDIWidget();
    trackWidget_->setPaintFn([this](Gdiplus::Graphics& g, int w, int h) {
        trackView_.paint(g, w, h, sim_.history(), sim_.current(), runway_);
    });
    trackWidget_->setMinimumHeight(420);
    rightSplit->addWidget(trackWidget_);

    // ---- 右侧参数面板 ----
    QGroupBox* infoGroup = new QGroupBox(QStringLiteral("飞行参数与仿真控制"));
    QVBoxLayout* infoLayout = new QVBoxLayout(infoGroup);
    infoLayout->setSpacing(10);

    QLabel* paramTitle = new QLabel(QStringLiteral("实时参数"));
    paramTitle->setObjectName("sectionLabel");
    infoLayout->addWidget(paramTitle);

    paramTable_ = new QTableWidget(8, 2);
    paramTable_->setHorizontalHeaderLabels({
        QStringLiteral("参数项"), QStringLiteral("数值")
    });
    paramTable_->horizontalHeader()->setStretchLastSection(true);
    paramTable_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    paramTable_->verticalHeader()->hide();
    paramTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    paramTable_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    paramTable_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    paramTable_->setShowGrid(true);

    QStringList names = {
        QStringLiteral("风速"),
        QStringLiteral("温度"),
        QStringLiteral("相对高度"),
        QStringLiteral("水平飞行速度"),
        QStringLiteral("垂直升降速度"),
        QStringLiteral("电池单体电压"),
        QStringLiteral("遥控信号强度"),
        QStringLiteral("相对返航点距离")
    };
    for (int i = 0; i < 8; ++i) {
        paramTable_->setItem(i, 0, new QTableWidgetItem(names[i]));
        paramTable_->setItem(i, 1, new QTableWidgetItem("-"));
        paramTable_->setRowHeight(i, 30);
    }
    infoLayout->addWidget(paramTable_);

    QFrame* separator = new QFrame();
    separator->setObjectName("panelSeparator");
    separator->setFrameShape(QFrame::HLine);
    separator->setFrameShadow(QFrame::Plain);
    infoLayout->addWidget(separator);

    // 仿真控制
    QLabel* ctrlTitle = new QLabel(QStringLiteral("仿真控制"));
    ctrlTitle->setObjectName("sectionLabel");
    infoLayout->addWidget(ctrlTitle);

    QHBoxLayout* infoRow = new QHBoxLayout();
    phaseLabel_ = new QLabel(QStringLiteral("阶段: 停止"));
    timeLabel_ = new QLabel(QStringLiteral("时间: 0.0s"));
    infoRow->addWidget(phaseLabel_);
    infoRow->addWidget(timeLabel_);
    infoLayout->addLayout(infoRow);

    QHBoxLayout* windRow = new QHBoxLayout();
    windRow->addWidget(new QLabel(QStringLiteral("风速(km/h):")));
    windSpin_ = new QDoubleSpinBox();
    windSpin_->setRange(0, 50);
    windSpin_->setValue(10);
    windSpin_->setSingleStep(1);
    connect(windSpin_, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            [this](double v) { sim_.setWind(v); });
    windRow->addWidget(windSpin_);
    infoLayout->addLayout(windRow);

    QHBoxLayout* phaseRow = new QHBoxLayout();
    phaseRow->addWidget(new QLabel(QStringLiteral("切换阶段:")));
    phaseCombo_ = new QComboBox();
    phaseCombo_->addItem(QStringLiteral("起飞"), (int)Phase::Takeoff);
    phaseCombo_->addItem(QStringLiteral("爬升"), (int)Phase::Climb);
    phaseCombo_->addItem(QStringLiteral("巡航"), (int)Phase::Cruise);
    phaseCombo_->addItem(QStringLiteral("进近"), (int)Phase::Approach);
    phaseCombo_->addItem(QStringLiteral("着陆"), (int)Phase::Landing);
    connect(phaseCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            [this](int idx) {
                Phase p = static_cast<Phase>(phaseCombo_->itemData(idx).toInt());
                sim_.setPhase(p);
            });
    phaseRow->addWidget(phaseCombo_);
    infoLayout->addLayout(phaseRow);

    QHBoxLayout* btnRow = new QHBoxLayout();
    startStopBtn_ = new QPushButton(QStringLiteral("开始仿真"));
    connect(startStopBtn_, &QPushButton::clicked, this, &MainWindow::onStartStop);
    btnRow->addWidget(startStopBtn_);
    resetBtn_ = new QPushButton(QStringLiteral("重置"));
    connect(resetBtn_, &QPushButton::clicked, this, &MainWindow::onReset);
    btnRow->addWidget(resetBtn_);
    infoLayout->addLayout(btnRow);

    rightSplit->addWidget(infoGroup);
    rightSplit->setStretchFactor(0, 7);
    rightSplit->setStretchFactor(1, 5);
    mainLayout->addWidget(rightSplit, 13);

    rootLayout->addWidget(bodyWidget, 1);

    // ---- 底部控制栏：协议帧传输 ----
    QGroupBox* txGroup = new QGroupBox(QStringLiteral("协议帧传输控制"));
    QHBoxLayout* txLayout = new QHBoxLayout(txGroup);
    txLayout->setSpacing(8);

    txLayout->addWidget(new QLabel(QStringLiteral("模式:")));
    modeCombo_ = new QComboBox();
    modeCombo_->addItem(QStringLiteral("内部仿真→文件"));
    modeCombo_->addItem(QStringLiteral("读取外部文件"));
    txLayout->addWidget(modeCombo_);

    txLayout->addWidget(new QLabel(QStringLiteral("间隔(ms):")));
    intervalSpin_ = new QSpinBox();
    intervalSpin_->setRange(20, 2000);
    intervalSpin_->setValue(200);
    intervalSpin_->setSingleStep(10);
    txLayout->addWidget(intervalSpin_);

    txLayout->addWidget(new QLabel(QStringLiteral("跑道:")));
    runwayNameEdit_ = new QLineEdit(QStringLiteral("RW01"));
    runwayNameEdit_->setMaximumWidth(80);
    txLayout->addWidget(runwayNameEdit_);

    txLayout->addWidget(new QLabel(QStringLiteral("Lon:")));
    rwLonSpin_ = new QDoubleSpinBox();
    rwLonSpin_->setRange(-180, 180);
    rwLonSpin_->setDecimals(7);
    rwLonSpin_->setValue(116.5874000);
    txLayout->addWidget(rwLonSpin_);

    txLayout->addWidget(new QLabel(QStringLiteral("Lat:")));
    rwLatSpin_ = new QDoubleSpinBox();
    rwLatSpin_->setRange(-90, 90);
    rwLatSpin_->setDecimals(7);
    rwLatSpin_->setValue(40.0725000);
    txLayout->addWidget(rwLatSpin_);

    fileBtn_ = new QPushButton(QStringLiteral("选择文件"));
    connect(fileBtn_, &QPushButton::clicked, this, &MainWindow::onSelectFile);
    txLayout->addWidget(fileBtn_);

    filePathLabel_ = new QLabel(QStringLiteral("--"));
    filePathLabel_->setMaximumWidth(200);
    txLayout->addWidget(filePathLabel_);

    transmitBtn_ = new QPushButton(QStringLiteral("开始传输"));
    connect(transmitBtn_, &QPushButton::clicked, this, &MainWindow::onTransmitStartStop);
    txLayout->addWidget(transmitBtn_);

    rootLayout->addWidget(txGroup);

    // ---- 样式表 ----
    setStyleSheet(R"(
        QMainWindow, QWidget { background: #1e1e2e; }
        QGroupBox {
            color: #89b4fa; font-weight: bold; font-size: 14px;
            border: 1px solid #45475a; border-radius: 4px;
            margin-top: 12px; padding-top: 16px;
        }
        QGroupBox::title { subcontrol-origin: margin; left: 10px; }
        QTableWidget {
            background: #181825; color: #cdd6f4;
            gridline-color: #313244; border: none; font-size: 13px;
        }
        QHeaderView::section {
            background: #1e1e2e; color: #a6adc8;
            border: 1px solid #313244; padding: 6px; font-size: 13px;
        }
        QLabel { color: #cdd6f4; font-size: 13px; }
        QLabel#sectionLabel {
            color: #f9e2af; font-size: 13px; font-weight: bold; padding-top: 2px;
        }
        QComboBox, QDoubleSpinBox, QSpinBox, QLineEdit {
            background: #181825; color: #cdd6f4;
            border: 1px solid #45475a; border-radius: 3px; padding: 5px; font-size: 13px;
        }
        QFrame#panelSeparator {
            color: #45475a; background: #45475a;
            min-height: 1px; max-height: 1px; border: none;
        }
        QPushButton {
            background: #313244; color: #a6e3a1;
            border: 1px solid #a6e3a1; border-radius: 3px;
            padding: 8px 18px; font-weight: bold; font-size: 13px;
        }
        QPushButton:hover { background: #45475a; }
        QPushButton:pressed { background: #181825; }
    )");

    updateParamTable();
}

void MainWindow::onStartStop() {
    if (running_) {
        timer_->stop();
        running_ = false;
        // 暂停主 timer 时，如果模式 A 传输在跑，自动停止传输
        if (transmitter_->isRunning() &&
            transmitter_->mode() == FrameTransmitter::SimulateThenFile) {
            transmitter_->stop();
            transmitBtn_->setText(QStringLiteral("开始传输"));
        }
        startStopBtn_->setText(QStringLiteral("继续仿真"));
        return;
    }
    if (sim_.phase() == Phase::Stopped && sim_.elapsed() == 0) {
        sim_.setPhase(Phase::Takeoff);
    }
    lastTick_ = std::chrono::steady_clock::now();
    timer_->start(33);
    running_ = true;
    startStopBtn_->setText(QStringLiteral("暂停"));
}

void MainWindow::onReset() {
    timer_->stop();
    running_ = false;
    if (transmitter_->isRunning()) {
        transmitter_->stop();
        transmitBtn_->setText(QStringLiteral("开始传输"));
    }
    sim_.reset();
    startStopBtn_->setText(QStringLiteral("开始仿真"));
    phaseLabel_->setText(QStringLiteral("阶段: 停止"));
    timeLabel_->setText(QStringLiteral("时间: 0.0s"));
    updateParamTable();
    profileWidget_->requestRepaint();
    timeSeriesWidget_->requestRepaint();
    deviationWidget_->requestRepaint();
    trackWidget_->requestRepaint();
}

void MainWindow::onTick() {
    const auto now = std::chrono::steady_clock::now();
    double dt = std::chrono::duration<double>(now - lastTick_).count();
    lastTick_ = now;
    dt = std::min(dt, 0.1);
    sim_.update(dt);

    profileWidget_->requestRepaint();
    timeSeriesWidget_->requestRepaint();
    deviationWidget_->requestRepaint();
    trackWidget_->requestRepaint();
    updateParamTable();

    phaseLabel_->setText(
        QStringLiteral("阶段: %1").arg(QString::fromUtf8(phaseStr(sim_.phase()))));
    timeLabel_->setText(
        QStringLiteral("时间: %1s").arg(sim_.elapsed(), 0, 'f', 1));

    if (sim_.phase() == Phase::Stopped && running_) {
        timer_->stop();
        running_ = false;
        startStopBtn_->setText(QStringLiteral("仿真完成"));
    }
}

void MainWindow::updateParamTable() {
    ParamPanel p = sim_.getParams();
    auto set = [this](int row, const QString& val) {
        paramTable_->item(row, 1)->setText(val);
    };
    set(0, QStringLiteral("%1 km/h").arg(p.windSpeed, 0, 'f', 1));
    set(1, QStringLiteral("%1 °C").arg(p.temperature, 0, 'f', 1));
    set(2, QStringLiteral("%1 m").arg(p.altitude, 0, 'f', 1));
    set(3, QStringLiteral("%1 km/h").arg(p.hSpeed, 0, 'f', 1));
    set(4, QStringLiteral("%1 m/s").arg(p.vSpeed, 0, 'f', 1));
    set(5, QStringLiteral("%1 V").arg(p.batteryVolt, 0, 'f', 2));
    set(6, QStringLiteral("%1 %").arg(p.signalStrength, 0, 'f', 0));
    set(7, QStringLiteral("%1 m").arg(p.distToReturn, 0, 'f', 0));
}

// ---- 协议帧传输控制 ----
void MainWindow::onTransmitStartStop() {
    if (transmitter_->isRunning()) {
        transmitter_->stop();
        transmitBtn_->setText(QStringLiteral("开始传输"));
        return;
    }

    ProtoRunwayInfo rw;
    rw.name = runwayNameEdit_->text();
    rw.lonDeg = rwLonSpin_->value();
    rw.latDeg = rwLatSpin_->value();
    transmitter_->setRunway(rw);
    transmitter_->setIntervalMs(intervalSpin_->value());

    // 同步协议跑道信息到成员，供 InfoBar 等使用
    protoRunway_ = rw;

    int modeIdx = modeCombo_->currentIndex();
    if (modeIdx == 0) {
        transmitter_->setMode(FrameTransmitter::SimulateThenFile);
        if (selectedFilePath_.isEmpty()) {
            selectedFilePath_ = QStringLiteral("telemetry_output.bin");
        }
        transmitter_->setOutputFile(selectedFilePath_);
        if (sim_.phase() == Phase::Stopped && sim_.elapsed() == 0) {
            sim_.setPhase(Phase::Takeoff);
        }
        // 确保主仿真 timer 在跑（Simulator 只由主 timer 推进）
        if (!running_) {
            onStartStop();
        }
    } else {
        transmitter_->setMode(FrameTransmitter::ReadExternalFile);
        if (selectedFilePath_.isEmpty()) {
            QMessageBox::warning(this, QStringLiteral("错误"),
                QStringLiteral("请先选择输入文件"));
            return;
        }
        transmitter_->setInputFile(selectedFilePath_);
    }

    transmitter_->start();
    transmitBtn_->setText(QStringLiteral("停止传输"));
}

void MainWindow::onSelectFile() {
    int modeIdx = modeCombo_->currentIndex();
    QString path;
    if (modeIdx == 0) {
        path = QFileDialog::getSaveFileName(this,
            QStringLiteral("选择输出文件"),
            QString(), QStringLiteral("Binary (*.bin);;All (*)"));
    } else {
        path = QFileDialog::getOpenFileName(this,
            QStringLiteral("选择输入文件"),
            QString(), QStringLiteral("Binary (*.bin);;All (*)"));
    }
    if (!path.isEmpty()) {
        selectedFilePath_ = path;
        filePathLabel_->setText(QFileInfo(path).fileName());
    }
}

void MainWindow::onFrameReady(TelemetryFrame tm, ProtoRunwayInfo rw, qint64 timestampMs) {
    // 同步协议跑道信息（模式 B 回放时更新）
    protoRunway_ = rw;
    infoBar_->updateInfo(rw, timestampMs);
    // 刷新视图
    profileWidget_->requestRepaint();
    timeSeriesWidget_->requestRepaint();
    deviationWidget_->requestRepaint();
    trackWidget_->requestRepaint();
}

void MainWindow::onTransmitError(QString msg) {
    QMessageBox::warning(this, QStringLiteral("传输错误"), msg);
}

} // namespace uav
