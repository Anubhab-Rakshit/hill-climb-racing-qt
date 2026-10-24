@echo off
setlocal
cd /d "%~dp0"
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat"
if not exist "build\Desktop_Qt_6_11_1_MSVC2022_64bit_Debug" mkdir "build\Desktop_Qt_6_11_1_MSVC2022_64bit_Debug"
cd build\Desktop_Qt_6_11_1_MSVC2022_64bit_Debug
E:\qt\6.11.1\msvc2022_64\bin\qmake.exe ..\..\hill-climbing-qt.pro -spec win32-msvc "CONFIG+=debug" "CONFIG+=qml_debug"
if %ERRORLEVEL% neq 0 (
    echo QMake failed!
    exit /b %ERRORLEVEL%
)
E:\qt\Tools\QtCreator\bin\jom\jom.exe -j4
if %ERRORLEVEL% neq 0 (
    echo Build failed!
    exit /b %ERRORLEVEL%
)
echo Build completed successfully!
