#!/usr/bin/env bash
# Runs a ROM in ares on a virtual X display and captures screenshots (fishmod/tools/harness.py does the work).
#
# Usage: fishmod/tools/ares_capture.sh <rom> <outdir> <seconds> [interval]
#   Takes a screenshot every [interval] seconds (default 5) for <seconds>,
#   cropped to the game view. Needs ARES=/path/to/ares (built with the N64 core),
#   Xvfb and ImageMagick. DISP picks the X display (default :99).
set -euo pipefail
exec python3 "$(dirname "$0")/harness.py" capture "$@"
