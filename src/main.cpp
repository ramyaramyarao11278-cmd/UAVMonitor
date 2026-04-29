#include <QApplication>
#include <QDebug>
#include <cassert>
#include <cstring>
#include "MainWindow.h"
#include "Protocol.h"
#include "Simulator.h"

// Debug 模式下的 pack→unpack 往返自测
static void selfTest() {
#ifdef QT_DEBUG
    qDebug() << "[SelfTest] Running pack/unpack roundtrip...";

    uav::Simulator sim;
    sim.setPhase(uav::Phase::Takeoff);
    sim.update(0.2);
    uav::TelemetryFrame origFrame = sim.generateFrame();

    uav::ProtoRunwayInfo rw;
    rw.name = "RW01L";
    rw.lonDeg = 116.5874;
    rw.latDeg = 40.0725;
    qint64 ts = 1714400000000LL;

    QByteArray packed = uav::packFrame(origFrame, rw, ts);
    assert(packed.size() == uav::TOTAL_FRAME_SIZE);

    uav::TelemetryFrame decoded{};
    uav::ProtoRunwayInfo decodedRw;
    qint64 decodedTs = 0;
    QString err;
    bool ok = uav::unpackFrame(packed, decoded, decodedRw, decodedTs, &err);
    assert(ok && "unpackFrame failed");
    assert(decodedTs == ts);
    assert(decodedRw.name == rw.name);
    assert(std::memcmp(&origFrame, &decoded, sizeof(uav::TelemetryFrame)) == 0);

    qDebug() << "[SelfTest] PASSED - frame size:" << packed.size()
             << "runway:" << decodedRw.name
             << "timestamp:" << decodedTs;
#endif
}

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    selfTest();
    uav::MainWindow win;
    win.show();
    return app.exec();
}
