@echo off
call "D:\Visual Studio Cute\VC\Auxiliary\Build\vcvars64.bat"
set PATH=D:\Program Files\bin;%PATH%
cd /d "D:\Cute Flower\2026-05-18-task-1\sephirot-rs"
nvcc -arch sm_89 -o gpu_runner.exe gpu_runner.cu -lcuda -I "D:\Program Files\include"
echo EXIT_CODE=%ERRORLEVEL%
