@echo off
chcp 65001 >nul
cd /d "%~dp0"
g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic scheduler.cpp -o scheduler.exe
if errorlevel 1 exit /b 1
scheduler.exe --test
if errorlevel 1 exit /b 1
scheduler.exe --file data/basic.txt
scheduler.exe --random --seed 20261108 --summary
pause
