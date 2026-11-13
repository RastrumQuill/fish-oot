@echo off
rem fish-oot's lab: the console's runs ordered through git, until this window is closed. It takes the queue from the
rem lab branch (fishmod\pc\lab_queue.txt), starts each run with the smart plug, and pushes the results to the
rem morning-results branch (lab/ there). See the top of fishmod\pc\lab.py. --test-plug checks the plug.
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
%PY% "%~dp0lab.py" %*
echo.
echo The lab stopped. This window can be closed.
pause
