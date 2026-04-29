#pragma once
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QMainWindow>
#include <QPushButton>
#include <QTableWidget>
#include <QTimer>
#include <chrono>

#include "DeviationView.h"
#include "GDIWidget.h"
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
};

} // namespace uav
