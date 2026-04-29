#pragma once
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QPushButton>
#include <QSpinBox>
#include <QTableWidget>
#include <QTimer>
#include <chrono>

#include "DeviationView.h"
#include "FrameTransmitter.h"
#include "GDIWidget.h"
#include "InfoBar.h"
#include "ProfileView.h"
#include "Simulator.h"
#include "TimeSeriesView.h"
#include "TrackView.h"

namespace uav {

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private slots:
    void onTick();
    void onStartStop();
    void onReset();

    // 协议帧相关
    void onTransmitStartStop();
    void onSelectFile();
    void onFrameReady(uav::TelemetryFrame tm, uav::ProtoRunwayInfo rw, qint64 timestampMs);
    void onTransmitError(QString msg);

private:
    void setupUI();
    void updateParamTable();

    Simulator      sim_;
    ProfileView    profileView_;
    TimeSeriesView timeSeriesView_;
    DeviationView  deviationView_;
    TrackView      trackView_;
    RunwayInfo     runway_;

    GDIWidget*     profileWidget_    = nullptr;
    GDIWidget*     timeSeriesWidget_ = nullptr;
    GDIWidget*     deviationWidget_  = nullptr;
    GDIWidget*     trackWidget_      = nullptr;
    QTableWidget*  paramTable_       = nullptr;
    QPushButton*   startStopBtn_     = nullptr;
    QPushButton*   resetBtn_         = nullptr;
    QComboBox*     phaseCombo_       = nullptr;
    QDoubleSpinBox* windSpin_        = nullptr;
    QLabel*        phaseLabel_       = nullptr;
    QLabel*        timeLabel_        = nullptr;
    QTimer*        timer_            = nullptr;

    bool running_ = false;
    std::chrono::steady_clock::time_point lastTick_;

    // ---- 新增：协议帧相关控件 ----
    InfoBar*          infoBar_           = nullptr;
    FrameTransmitter* transmitter_      = nullptr;
    QComboBox*        modeCombo_        = nullptr;
    QSpinBox*         intervalSpin_     = nullptr;
    QLineEdit*        runwayNameEdit_   = nullptr;
    QDoubleSpinBox*   rwLonSpin_        = nullptr;
    QDoubleSpinBox*   rwLatSpin_        = nullptr;
    QPushButton*      fileBtn_          = nullptr;
    QPushButton*      transmitBtn_      = nullptr;
    QLabel*           filePathLabel_    = nullptr;
    QString           selectedFilePath_;
};

} // namespace uav
