@echo off
setlocal
cd /d "%~dp0"
set PATH=E:\qt\6.11.1\msvc2022_64\bin;%PATH%
start "" "%~dp0build\Desktop_Qt_6_11_1_MSVC2022_64bit_Debug\debug\hill-climbing-qt.exe"
