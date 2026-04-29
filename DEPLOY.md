# UAVMonitor 部署文档

## 环境要求

| 组件 | 版本 | 备注 |
|------|------|------|
| 操作系统 | Windows 10/11 x64 | |
| Qt | 6.10.2 | msvc2022_64 |
| Visual Studio Build Tools | 2022 | 需含 MSVC x64 工具链 |
| CMake | 3.16+ | 需在 PATH 中 |

## 编译构建

### 方式一：使用脚本（推荐）

双击或在命令行运行：

```bat
build.bat
```

构建成功后，exe 位于：

```
build\UAVMonitor.exe
```

### 方式二：手动构建

```bat
:: 初始化 MSVC 环境
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x64

:: 配置
cd C:\Users\da983\UAVMonitor
mkdir build
cd build
cmake .. -G "NMake Makefiles" -DCMAKE_PREFIX_PATH=C:/Qt/6.10.2/msvc2022_64 -DCMAKE_BUILD_TYPE=Release

:: 编译
nmake
```

## 运行前部署（windeployqt）

构建完成后，需要将 Qt 运行时 DLL 拷贝到 exe 同目录，否则无法在无 Qt 环境的机器上运行：

```bat
cd build
C:\Qt\6.10.2\msvc2022_64\bin\windeployqt.exe UAVMonitor.exe
```

执行后目录中会自动生成所需的 Qt DLL、插件等文件。

## 依赖的系统库

以下为 Windows 系统自带库，无需额外安装：

- `gdiplus.dll` — GDI+ 图形
- `gdi32.dll` — GDI 图形
- `winmm.dll` — 多媒体计时

## 发布打包

将 `build\` 目录下以下内容打包分发：

```
UAVMonitor.exe
Qt6Core.dll
Qt6Gui.dll
Qt6Widgets.dll
platforms\
styles\
（windeployqt 生成的其他文件）
```

## 常见问题

**Q: 提示找不到 Qt6Core.dll**
A: 未执行 windeployqt，按上方步骤补充运行时文件。

**Q: cmake 报错找不到 Qt6**
A: 检查 `-DCMAKE_PREFIX_PATH` 路径是否与本机 Qt 安装路径一致，默认为 `C:/Qt/6.10.2/msvc2022_64`。

**Q: nmake 不是内部命令**
A: 未初始化 MSVC 环境，需先执行 `vcvarsall.bat x64`。
