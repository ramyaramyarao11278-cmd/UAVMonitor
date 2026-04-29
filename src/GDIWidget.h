#pragma once
#include <QWidget>
#include <QEvent>
#include <QResizeEvent>
#include <QScreen>
#include <QWindow>
#include <functional>
#include <windows.h>
#include <gdiplus.h>

namespace uav {

class GDIWidget : public QWidget {
    Q_OBJECT
public:
    using PaintFn = std::function<void(Gdiplus::Graphics& g, int w, int h)>;

    explicit GDIWidget(QWidget* parent = nullptr) : QWidget(parent) {
        QWidget::paintEngine();
        setAttribute(Qt::WA_NativeWindow);
        setAttribute(Qt::WA_PaintOnScreen);
        setAttribute(Qt::WA_NoSystemBackground);
        setAttribute(Qt::WA_OpaquePaintEvent);
    }

    void setPaintFn(PaintFn fn) { paintFn_ = std::move(fn); }

    void requestRepaint() {
        HWND hwnd = (HWND)winId();
        InvalidateRect(hwnd, nullptr, FALSE);
    }

protected:
    QPaintEngine* paintEngine() const override { return nullptr; }

    bool event(QEvent* e) override {
        if (e->type() == QEvent::Paint ||
            e->type() == QEvent::UpdateRequest) {
            return true;
        }
        return QWidget::event(e);
    }

    bool nativeEvent(const QByteArray& eventType, void* message, qintptr* result) override {
        MSG* msg = static_cast<MSG*>(message);

        if (msg->message == WM_ERASEBKGND) {
            *result = 1;
            return true;
        }

        if (msg->message == WM_PAINT) {
            doPaint(msg->hwnd);
            *result = 0;
            return true;
        }

        return QWidget::nativeEvent(eventType, message, result);
    }

    void resizeEvent(QResizeEvent* e) override {
        QWidget::resizeEvent(e);
        requestRepaint();
    }

private:
    void doPaint(HWND hwnd) {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        // 用Win32 API获取窗口实际物理像素大小，不依赖Qt的逻辑像素
        RECT rc;
        GetClientRect(hwnd, &rc);
        int pw = rc.right - rc.left;
        int ph = rc.bottom - rc.top;

        if (pw <= 0 || ph <= 0) {
            EndPaint(hwnd, &ps);
            return;
        }

        // 双缓冲 - 用物理像素大小
        HDC mem = CreateCompatibleDC(hdc);
        HBITMAP bmp = CreateCompatibleBitmap(hdc, pw, ph);
        HBITMAP old = (HBITMAP)SelectObject(mem, bmp);

        {
            Gdiplus::Graphics g(mem);
            g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
            g.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAlias);
            g.Clear(Gdiplus::Color(255, 30, 30, 35));

            // 传物理像素尺寸给绘制函数
            if (paintFn_) paintFn_(g, pw, ph);
        }

        BitBlt(hdc, 0, 0, pw, ph, mem, 0, 0, SRCCOPY);
        SelectObject(mem, old);
        DeleteObject(bmp);
        DeleteDC(mem);

        EndPaint(hwnd, &ps);
    }

    PaintFn paintFn_;
};

} // namespace uav
