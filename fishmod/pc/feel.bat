@echo off
rem fish-oot: the fishing feel tuner. Load a test build with play.bat first and start it; then double-click this: a
rem window of sliders that change the fight's and the bite's numbers on the console while you play (through the
rem SummerCart64's USB). Close it before the next upload. See the top of fishmod\pc\feel.py.
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
%PY% "%~dp0feel.py" %*
echo.
echo Finished. This window can be closed.
pause
