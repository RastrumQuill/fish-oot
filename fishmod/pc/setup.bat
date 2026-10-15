@echo off
rem One-time setup for the fish-oot PC test runner: the conda environment, then ares.
rem Run it from the Anaconda Prompt.
call conda env create -f "%~dp0environment.yml"
call conda activate fishoot
python "%~dp0install_ares.py"
echo.
echo Done. Put the packs in fishmod\pc\packs\ and run: fishmod\pc\run_all.bat YOUR_ROM.z64
