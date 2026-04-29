@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x64

cd /d "%~dp0"
if not exist build mkdir build
cd build

cmake .. -G "NMake Makefiles" -DCMAKE_PREFIX_PATH=C:/Qt/6.10.2/msvc2022_64 -DCMAKE_BUILD_TYPE=Release
if %errorlevel% neq 0 (
    echo === CMAKE CONFIGURE FAILED ===
    exit /b 1
)

nmake
if %errorlevel% neq 0 (
    echo === BUILD FAILED ===
    exit /b 1
)

echo === BUILD SUCCESS ===
dir /b *.exe
