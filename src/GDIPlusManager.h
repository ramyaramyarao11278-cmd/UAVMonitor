#pragma once
#include <windows.h>
#include <gdiplus.h>
#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "gdi32.lib")

namespace uav {

class GDIPlusManager {
public:
    static GDIPlusManager& instance() {
        static GDIPlusManager m;
        return m;
    }
    bool ok() const { return ok_; }
private:
    GDIPlusManager() {
        Gdiplus::GdiplusStartupInput input;
        ok_ = (Gdiplus::GdiplusStartup(&token_, &input, nullptr) == Gdiplus::Ok);
    }
    ~GDIPlusManager() { if (ok_) Gdiplus::GdiplusShutdown(token_); }
    ULONG_PTR token_ = 0;
    bool ok_ = false;
};

} // namespace uav
