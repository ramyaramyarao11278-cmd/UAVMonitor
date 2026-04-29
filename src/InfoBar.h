#pragma once
#include <QWidget>
#include <QLabel>
#include "Protocol.h"

namespace uav {

/// 顶部信息条 —— 显示跑道名称、跑道经纬度、任务时间
class InfoBar : public QWidget {
    Q_OBJECT
public:
    explicit InfoBar(QWidget* parent = nullptr);

public slots:
    /// 更新跑道和时间信息
    void updateInfo(const uav::ProtoRunwayInfo& rw, qint64 timestampMs);

private:
    QLabel* runwayNameLabel_ = nullptr;
    QLabel* runwayPosLabel_  = nullptr;
    QLabel* missionTimeLabel_ = nullptr;
};

} // namespace uav
