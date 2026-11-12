@echo off
rem The capture card viewer (fishmod\pc\capture.py): the console's picture and sound with as little delay as possible.
rem Run it from the Anaconda Prompt with the fishoot environment, or double-click it (it activates the environment).
rem   capture.bat --list      the devices it can see
rem   capture.bat --video 1 --audio "Cam Link"
call conda activate fishoot 2>nul
python -c "import cv2, sounddevice" 2>nul || python -m pip install --quiet opencv-python sounddevice
python "%~dp0capture.py" %*
