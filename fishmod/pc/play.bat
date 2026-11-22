@echo off
rem fish-oot: load a copy onto the console to play. Double-click it and pick: the game, or test mode at Lake Hylia
rem (adult, child or drained) or in Kokiri Forest (or give 1-5). It patches your ROM, sends it to the SummerCart64, and then you
rem switch the console off and on. See the top of fishmod\pc\play.py.
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
%PY% "%~dp0play.py" %*
echo.
echo Finished. This window can be closed.
pause
