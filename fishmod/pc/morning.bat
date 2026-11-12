@echo off
rem fish-oot's morning runner: double-click it (or drop .bps patches on it). It patches your ROM for each queued
rem patch (fishmod\pc\morning_queue.txt), runs each on the console through the SummerCart64, and pushes the results to
rem the morning-results branch. See fishmod\pc\MORNING.md.
setlocal
set "PY="
for %%D in ("%USERPROFILE%\miniconda3" "%USERPROFILE%\anaconda3" "%LOCALAPPDATA%\miniconda3" "%LOCALAPPDATA%\anaconda3" "%ProgramData%\miniconda3" "%ProgramData%\anaconda3" "C:\miniconda3" "C:\anaconda3") do (
    if not defined PY if exist "%%~D\envs\fishoot\python.exe" set PY="%%~D\envs\fishoot\python.exe"
)
if not defined PY (
    where py >nul 2>nul && set "PY=py -3"
)
if not defined PY (
    where python >nul 2>nul && set "PY=python"
)
if not defined PY (
    echo No Python found: install Miniconda and run fishmod\pc\setup.bat once (see fishmod\pc\README.md^).
    pause
    exit /b 1
)
%PY% "%~dp0morning.py" %*
echo.
echo Finished. This window can be closed.
pause
