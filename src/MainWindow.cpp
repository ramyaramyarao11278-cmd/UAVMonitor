#include "MainWindow.h"

#include "GDIPlusManager.h"

#include <QFrame>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QSplitter>
#include <QVBoxLayout>
#include <timeapi.h>

#pragma comment(lib, "winmm.lib")

namespace uav {

static const char* phaseStr(Phase p) {
    switch (p) {
        case Phase::Stopped:  return "\xE5\x81\x9C\xE6\xAD\xA2";
        case Phase::Takeoff:  return "\xE8\xB5\xB7\xE9\xA3\x9E\xE6\xBB\x91\xE8\xB7\x91";
        case Phase::Climb:    return "\xE7\x88\xAC\xE5\x8D\x87";
        case Phase::Cruise:   return "\xE5\xB7\xA1\xE8\x88\xAA";
        case Phase::Approach: return "\xE8\xBF\x9B\xE8\xBF\x91";
        case Phase::Landing:  return "\xE7\x9D\x80\xE9\x99\x86\xE5\x87\x8F\xE9\x80\x9F";
    }

    return "\xE6\x9C\xAA\xE7\x9F\xA5";
}

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    timeBeginPeriod(1);

    GDIPlusManager::instance();
    setupUI();

    timer_ = new QTimer(this);
    connect(timer_, &QTimer::timeout, this, &MainWindow::onTick);
}

MainWindow::~MainWindow() {
    timeEndPeriod(1);
}

void MainWindow::setupUI() {
    setWindowTitle(QString::fromUtf8("\xE5\x9B\xBA\xE5\xAE\x9A\xE7\xBF\xBC\xE6\x97\xA0\xE4\xBA\xBA\xE6\x9C\xBA\xE8\xB5\xB7\xE9\x99\x8D\xE7\x9B\x91\xE6\x8E\xA7\xE7\xB3\xBB\xE7\xBB\x9F"));
    resize(1560, 940);
    setMinimumSize(1240, 760);

    QWidget* central = new QWidget();
    setCentralWidget(central);

    QHBoxLayout* mainLayout = new QHBoxLayout(central);
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

    QGroupBox* infoGroup = new QGroupBox(
        QString::fromUtf8("\xE9\xA3\x9E\xE8\xA1\x8C\xE5\x8F\x82\xE6\x95\xB0\xE4\xB8\x8E\xE4\xBB\xBF\xE7\x9C\x9F\xE6\x8E\xA7\xE5\x88\xB6"));
    QVBoxLayout* infoLayout = new QVBoxLayout(infoGroup);
    infoLayout->setSpacing(10);

    QLabel* paramTitle = new QLabel(QString::fromUtf8("\xE5\xAE\x9E\xE6\x97\xB6\xE5\x8F\x82\xE6\x95\xB0"));
    paramTitle->setObjectName("sectionLabel");
    infoLayout->addWidget(paramTitle);

    paramTable_ = new QTableWidget(8, 2);
    paramTable_->setHorizontalHeaderLabels({
        QString::fromUtf8("\xE5\x8F\x82\xE6\x95\xB0\xE9\xA1\xB9"),
        QString::fromUtf8("\xE6\x95\xB0\xE5\x80\xBC")
    });
    paramTable_->horizontalHeader()->setStretchLastSection(true);
    paramTable_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    paramTable_->verticalHeader()->hide();
    paramTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    paramTable_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    paramTable_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    paramTable_->setShowGrid(true);

    QStringList names = {
        QString::fromUtf8("\xE9\xA3\x8E\xE9\x80\x9F"),
        QString::fromUtf8("\xE6\xB8\xA9\xE5\xBA\xA6"),
        QString::fromUtf8("\xE7\x9B\xB8\xE5\xAF\xB9\xE9\xAB\x98\xE5\xBA\xA6"),
        QString::fromUtf8("\xE6\xB0\xB4\xE5\xB9\xB3\xE9\xA3\x9E\xE8\xA1\x8C\xE9\x80\x9F\xE5\xBA\xA6"),
        QString::fromUtf8("\xE5\x9E\x82\xE7\x9B\xB4\xE5\x8D\x87\xE9\x99\x8D\xE9\x80\x9F\xE5\xBA\xA6"),
        QString::fromUtf8("\xE7\x94\xB5\xE6\xB1\xA0\xE5\x8D\x95\xE4\xBD\x93\xE7\x94\xB5\xE5\x8E\x8B"),
        QString::fromUtf8("\xE9\x81\xA5\xE6\x8E\xA7\xE4\xBF\xA1\xE5\x8F\xB7\xE5\xBC\xBA\xE5\xBA\xA6"),
        QString::fromUtf8("\xE7\x9B\xB8\xE5\xAF\xB9\xE8\xBF\x94\xE8\x88\xAA\xE7\x82\xB9\xE8\xB7\x9D\xE7\xA6\xBB")
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

    QLabel* ctrlTitle = new QLabel(QString::fromUtf8("\xE4\xBB\xBF\xE7\x9C\x9F\xE6\x8E\xA7\xE5\x88\xB6"));
    ctrlTitle->setObjectName("sectionLabel");
    infoLayout->addWidget(ctrlTitle);

    QHBoxLayout* infoRow = new QHBoxLayout();
    phaseLabel_ = new QLabel(QString::fromUtf8("\xE9\x98\xB6\xE6\xAE\xB5: \xE5\x81\x9C\xE6\xAD\xA2"));
    timeLabel_ = new QLabel(QString::fromUtf8("\xE6\x97\xB6\xE9\x97\xB4: 0.0s"));
    infoRow->addWidget(phaseLabel_);
    infoRow->addWidget(timeLabel_);
    infoLayout->addLayout(infoRow);

    QHBoxLayout* windRow = new QHBoxLayout();
    windRow->addWidget(new QLabel(QString::fromUtf8("\xE9\xA3\x8E\xE9\x80\x9F(km/h):")));
    windSpin_ = new QDoubleSpinBox();
    windSpin_->setRange(0, 50);
    windSpin_->setValue(10);
    windSpin_->setSingleStep(1);
    connect(windSpin_, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            [this](double v) { sim_.setWind(v); });
    windRow->addWidget(windSpin_);
    infoLayout->addLayout(windRow);

    QHBoxLayout* phaseRow = new QHBoxLayout();
    phaseRow->addWidget(new QLabel(QString::fromUtf8("\xE5\x88\x87\xE6\x8D\xA2\xE9\x98\xB6\xE6\xAE\xB5:")));
    phaseCombo_ = new QComboBox();
    phaseCombo_->addItem(QString::fromUtf8("\xE8\xB5\xB7\xE9\xA3\x9E"), (int)Phase::Takeoff);
    phaseCombo_->addItem(QString::fromUtf8("\xE7\x88\xAC\xE5\x8D\x87"), (int)Phase::Climb);
    phaseCombo_->addItem(QString::fromUtf8("\xE5\xB7\xA1\xE8\x88\xAA"), (int)Phase::Cruise);
    phaseCombo_->addItem(QString::fromUtf8("\xE8\xBF\x9B\xE8\xBF\x91"), (int)Phase::Approach);
    phaseCombo_->addItem(QString::fromUtf8("\xE7\x9D\x80\xE9\x99\x86"), (int)Phase::Landing);
    connect(phaseCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            [this](int idx) {
                Phase p = static_cast<Phase>(phaseCombo_->itemData(idx).toInt());
                sim_.setPhase(p);
            });
    phaseRow->addWidget(phaseCombo_);
    infoLayout->addLayout(phaseRow);

    QHBoxLayout* btnRow = new QHBoxLayout();
    startStopBtn_ = new QPushButton(QString::fromUtf8("\xE5\xBC\x80\xE5\xA7\x8B\xE4\xBB\xBF\xE7\x9C\x9F"));
    connect(startStopBtn_, &QPushButton::clicked, this, &MainWindow::onStartStop);
    btnRow->addWidget(startStopBtn_);

    resetBtn_ = new QPushButton(QString::fromUtf8("\xE9\x87\x8D\xE7\xBD\xAE"));
    connect(resetBtn_, &QPushButton::clicked, this, &MainWindow::onReset);
    btnRow->addWidget(resetBtn_);
    infoLayout->addLayout(btnRow);

    rightSplit->addWidget(infoGroup);
    rightSplit->setStretchFactor(0, 7);
    rightSplit->setStretchFactor(1, 5);

    mainLayout->addWidget(rightSplit, 13);

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
            gridline-color: #313244; border: none;
            font-size: 13px;
        }
        QHeaderView::section {
            background: #1e1e2e; color: #a6adc8;
            border: 1px solid #313244; padding: 6px;
            font-size: 13px;
        }
        QLabel { color: #cdd6f4; font-size: 13px; }
        QLabel#sectionLabel {
            color: #f9e2af;
            font-size: 13px;
            font-weight: bold;
            padding-top: 2px;
        }
        QComboBox, QDoubleSpinBox {
            background: #181825; color: #cdd6f4;
            border: 1px solid #45475a; border-radius: 3px; padding: 5px;
            font-size: 13px;
        }
        QFrame#panelSeparator {
            color: #45475a;
            background: #45475a;
            min-height: 1px;
            max-height: 1px;
            border: none;
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
        startStopBtn_->setText(QString::fromUtf8("\xE7\xBB\xA7\xE7\xBB\xAD\xE4\xBB\xBF\xE7\x9C\x9F"));
        return;
    }

    if (sim_.phase() == Phase::Stopped && sim_.elapsed() == 0) {
        sim_.setPhase(Phase::Takeoff);
    }

    lastTick_ = std::chrono::steady_clock::now();
    timer_->start(33);
    running_ = true;
    startStopBtn_->setText(QString::fromUtf8("\xE6\x9A\x82\xE5\x81\x9C"));
}

void MainWindow::onReset() {
    timer_->stop();
    running_ = false;
    sim_.reset();

    startStopBtn_->setText(QString::fromUtf8("\xE5\xBC\x80\xE5\xA7\x8B\xE4\xBB\xBF\xE7\x9C\x9F"));
    phaseLabel_->setText(QString::fromUtf8("\xE9\x98\xB6\xE6\xAE\xB5: \xE5\x81\x9C\xE6\xAD\xA2"));
    timeLabel_->setText(QString::fromUtf8("\xE6\x97\xB6\xE9\x97\xB4: 0.0s"));
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
        QString::fromUtf8("\xE9\x98\xB6\xE6\xAE\xB5: %1").arg(QString::fromUtf8(phaseStr(sim_.phase()))));
    timeLabel_->setText(
        QString::fromUtf8("\xE6\x97\xB6\xE9\x97\xB4: %1s").arg(sim_.elapsed(), 0, 'f', 1));

    if (sim_.phase() == Phase::Stopped && running_) {
        timer_->stop();
        running_ = false;
        startStopBtn_->setText(QString::fromUtf8("\xE4\xBB\xBF\xE7\x9C\x9F\xE5\xAE\x8C\xE6\x88\x90"));
    }
}

void MainWindow::updateParamTable() {
    ParamPanel p = sim_.getParams();

    auto set = [this](int row, const QString& val) {
        paramTable_->item(row, 1)->setText(val);
    };

    set(0, QString("%1 km/h").arg(p.windSpeed, 0, 'f', 1));
    set(1, QString::fromUtf8("%1 \xC2\xB0" "C").arg(p.temperature, 0, 'f', 1));
    set(2, QString("%1 m").arg(p.altitude, 0, 'f', 1));
    set(3, QString("%1 km/h").arg(p.hSpeed, 0, 'f', 1));
    set(4, QString("%1 m/s").arg(p.vSpeed, 0, 'f', 1));
    set(5, QString("%1 V").arg(p.batteryVolt, 0, 'f', 2));
    set(6, QString("%1 %").arg(p.signalStrength, 0, 'f', 0));
    set(7, QString("%1 m").arg(p.distToReturn, 0, 'f', 0));
}

} // namespace uav
