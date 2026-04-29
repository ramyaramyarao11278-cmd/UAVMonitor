@echo off
setlocal

call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x64 >nul 2>&1

echo === STEP 1: CMAKE CONFIGURE ===
cd /d "C:\Users\da983\UAVMonitor"
if not exist build mkdir build
cd build

cmake .. -G "NMake Makefiles" -DCMAKE_PREFIX_PATH=C:/Qt/6.10.2/msvc2022_64 -DCMAKE_BUILD_TYPE=Release 2>&1
echo CMAKE_EXIT_CODE=%errorlevel%

echo === STEP 2: BUILD ===
nmake 2>&1
echo NMAKE_EXIT_CODE=%errorlevel%

echo === DONE ===
if exist UAVMonitor.exe (
    echo SUCCESS: UAVMonitor.exe built
) else (
    echo FAILED: no exe found
)
