# UAVMonitor — 固定翼无人机起降监控仿真系统

基于 **Qt6 + GDI+** 的 Windows 桌面应用，用于仿真和监控固定翼无人机的起飞/巡航/进近/着陆全流程参数。

## 功能列表

- **实时仿真引擎** — 内置 5 阶段飞行仿真器（起飞→爬升→巡航→进近→着陆），生成姿态、高度、速度、舵面、发动机等 80+ 遥测参数
- **遥测协议帧** — 按自定义二进制协议打包/解包，包含帧头同步、字段分辨率编码、MD5 校验
- **双模式数据源**
  - 模式 A：内部仿真 → 打包 → 写入 `.bin` 文件（可用于离线分析）
  - 模式 B：读取外部 `.bin` 文件 → 帧头同步 → 解包 → 实时回放
- **跑道/时间信息条** — 顶部 InfoBar 实时显示跑道名称、经纬度、UTC→本地时间
- **多视图可视化**（GDI+ 绘制）
  - 剖面视图（高度 vs 距离）
  - 时间序列视图（速度/攻角/高度/垂直速度）
  - 偏差视图（横向偏差、下滑道偏差）
  - 航迹俯视图

## 协议帧格式

| 偏移 | 长度 | 字段 | 说明 |
|------|------|------|------|
| 0 | 4B | 帧头 | 固定 `0xEB 0x90 0xEB 0x90` |
| 4 | 2B | 帧长 | 后续"数据体+MD5"的总字节数 |
| 6 | 8B | 时间戳 | UTC 毫秒 |
| 14 | 16B | 跑道名称 | ASCII，不足补 `0x00` |
| 30 | 4B | 跑道经度 | Int32，单位 1e-7 度 |
| 34 | 4B | 跑道纬度 | Int32，单位 1e-7 度 |
| 38 | 136B | 遥测数据体 | 紧凑打包的 `TelemetryFrame` 结构体 |
| 174 | 16B | MD5 校验 | 对偏移 0~173 的全部字节求 MD5 |

**总帧长：190 字节**

## 编译步骤

### 环境要求

- Windows 10/11
- Visual Studio 2022 Build Tools（MSVC）
- Qt 6.x（需要 QtWidgets 模块）
- CMake 3.16+

### 编译

```bat
build.bat
```

详细部署说明参见 [DEPLOY.md](DEPLOY.md)。

## 使用说明

1. 启动 `build\UAVMonitor.exe`
2. **仿真模式**：右侧面板选择飞行阶段，点击"开始仿真"观察四个视图实时刷新
3. **协议帧传输**（底部控制栏）：
   - 选择模式："内部仿真→文件"或"读取外部文件"
   - 设置传输间隔（默认 200ms）
   - 填写跑道名称和经纬度
   - 点击"选择文件"指定输入/输出路径
   - 点击"开始传输"，顶部 InfoBar 将每个间隔刷新一次时间戳
4. 模式 A 会自动启动仿真 timer，停止仿真时自动停止传输

## 目录结构

```
UAVMonitor/
├── CMakeLists.txt          # CMake 构建配置
├── build.bat               # 一键编译脚本
├── DEPLOY.md               # 部署说明
├── README.md               # 本文件
└── src/
    ├── main.cpp             # 入口 + debug 自测
    ├── MainWindow.h/cpp     # 主窗口（仿真控制 + 传输控制）
    ├── Simulator.h/cpp      # 飞行仿真器
    ├── TelemetryFrame.h     # 遥测帧 POD 结构体（136 字节）
    ├── Protocol.h/cpp       # 协议帧打包/解包 + MD5
    ├── FrameTransmitter.h/cpp # 双模式帧发送器
    ├── InfoBar.h/cpp        # 顶部跑道/时间信息条
    ├── ProfileView.h/cpp    # 剖面视图
    ├── TimeSeriesView.h/cpp # 时间序列视图
    ├── DeviationView.h/cpp  # 偏差视图
    ├── TrackView.h/cpp      # 航迹视图
    ├── GDIWidget.h/cpp      # GDI+ Qt 绘图桥接
    ├── GDIPlusManager.h     # GDI+ 初始化管理
    └── UAVData.h            # 基础数据结构
```
