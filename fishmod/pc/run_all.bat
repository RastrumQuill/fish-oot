@echo off
rem Runs every pack in fishmod\pc\packs\ (self-test first). Usage: run_all.bat C:\path\to\oot.z64
call conda activate fishoot
python "%~dp0run_all.py" --rom "%~1" %2 %3 %4 %5
