cmake_minimum_required(VERSION 3.16)
project(UAVMonitor LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

add_executable(UAVMonitor
    src/main.cpp
    src/MainWindow.cpp
    src/Simulator.cpp
    src/ProfileView.cpp
    src/TimeSeriesView.cpp
    src/DeviationView.cpp
    src/TrackView.cpp
    src/GDIWidget.cpp
    src/Protocol.cpp
    src/FrameTransmitter.cpp
    src/InfoBar.cpp
)

target_include_directories(UAVMonitor PRIVATE src)
target_compile_definitions(UAVMonitor PRIVATE NOMINMAX)

# MSVC: 用 /utf-8 支持源码中的中文字符串字面量
if(MSVC)
    target_compile_options(UAVMonitor PRIVATE /utf-8)
    target_link_libraries(UAVMonitor
        Qt6::Widgets
        "gdiplus.lib"
        "gdi32.lib"
        "winmm.lib"
    )
else()
    target_link_libraries(UAVMonitor
        Qt6::Widgets
        gdiplus
        gdi32
        winmm
    )
endif()

if(WIN32)
    set_target_properties(UAVMonitor PROPERTIES WIN32_EXECUTABLE $<CONFIG:Release>)
endif()
