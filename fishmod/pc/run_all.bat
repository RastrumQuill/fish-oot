@echo off
rem Runs every pack in fishmod\pc\packs\ (self-test first). Usage: run_all.bat [C:\path\to\oot.z64]
rem (the ROM's path is remembered after the first time)
call conda activate fishoot
if "%~1"=="" (
    python "%~dp0run_all.py" %2 %3 %4 %5
) else (
    python "%~dp0run_all.py" --rom "%~1" %2 %3 %4 %5
)
