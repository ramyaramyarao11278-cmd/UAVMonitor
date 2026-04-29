#include "InfoBar.h"
#include <QHBoxLayout>
#include <QDateTime>
#include <QTimeZone>

namespace uav {

InfoBar::InfoBar(QWidget* parent)
    : QWidget(parent)
{
    QHBoxLayout* lay = new QHBoxLayout(this);
    lay->setContentsMargins(12, 6, 12, 6);
    lay->setSpacing(24);

    runwayNameLabel_ = new QLabel(QStringLiteral("Runway: --"));
    runwayPosLabel_  = new QLabel(QStringLiteral("Pos: --"));
    missionTimeLabel_ = new QLabel(QStringLiteral("Time: --"));

    // 统一样式：与现有 UI 配色协调（Catppuccin 风）
    const QString labelStyle = QStringLiteral(
        "color: #89b4fa; font-size: 14px; font-weight: bold; padding: 2px 8px;"
    );

    runwayNameLabel_->setStyleSheet(labelStyle);
    runwayPosLabel_->setStyleSheet(labelStyle);
    missionTimeLabel_->setStyleSheet(labelStyle);

    lay->addWidget(runwayNameLabel_);
    lay->addWidget(runwayPosLabel_);
    lay->addStretch();
    lay->addWidget(missionTimeLabel_);

    // 信息条底部加一条微妙的分隔线效果
    setStyleSheet(QStringLiteral(
        "InfoBar { background: #181825; border-bottom: 1px solid #45475a; }"
    ));
    setFixedHeight(36);
}

void InfoBar::updateInfo(const ProtoRunwayInfo& rw, qint64 timestampMs)
{
    // 跑道名称
    runwayNameLabel_->setText(
        QStringLiteral("Runway: %1").arg(rw.name.isEmpty() ? "--" : rw.name));

    // 跑道经纬度
    runwayPosLabel_->setText(
        QStringLiteral("Lon: %1  Lat: %2")
            .arg(rw.lonDeg, 0, 'f', 7)
            .arg(rw.latDeg, 0, 'f', 7));

    // UTC 毫秒 → 本地时间
    QDateTime dt = QDateTime::fromMSecsSinceEpoch(timestampMs, QTimeZone::LocalTime);
    missionTimeLabel_->setText(
        QStringLiteral("Time: %1").arg(dt.toString("yyyy-MM-dd HH:mm:ss.zzz")));
}

} // namespace uav
