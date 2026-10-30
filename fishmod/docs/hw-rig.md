# The hardware test rig (plan)

A CachyOS VM on the user's PC with the SummerCart64's USB and a capture card for the N64's AV out passed through,
so Claude can put a build on the console, run it, watch it and read its results without the user swapping SD cards.
No controller input yet (an ESP32 pretending to be a controller comes later), so tests use ROMs that drive
themselves: the debug harness's autoplay scripts and the benches already do (`fishmod/scripts/`).

## Who does what

With 2-4 cores the VM is for the hardware, not for heavy CPU work:

| Work | Where | Why |
|---|---|---|
| Talking to the cart and the capture card | VM | the hardware is there |
| Builds for the console | VM, incremental | a change to a few files rebuilds in a minute or two; a clean build of the whole decomp is slow on 2-4 cores (once, after setup or a flag change) |
| Long soaks | **the console** | a test on hardware is the real thing, and it costs the VM nothing: it only grabs a frame now and then |
| ares runs, tour captures, A/B benches, anything run several at once | cloud container | they keep several cores busy for hours; ares on 2 cores with no GPU would be slow |
| Asset generation (props, scatter, collision, item page) | either | quick |

So the cloud session stays the main place to write code and run ares, and the VM becomes the place things are
proven on the console, mostly with long unattended runs.

## What a test looks like

1. Build the ROM on the VM from this repo (the ROM never leaves the user's machines: the VM has its own copy of the
   base ROM, as the cloud container does).
2. Back up the cart's save (`sc64deployer download save`), upload the ROM with its save type
   (`sc64deployer upload --save-type sram ...`), and boot it (see *Open questions*: resetting the console).
3. Watch: grab frames from the capture card with ffmpeg, as `ares_capture.sh` does from the emulator, and compare
   them with ares's.
4. Read results: download the save and read it with the report tools (`hwbench_report.py`, `area_report.py`), or,
   better, stream them live over the SC64's USB debug link (a follow-up: the debug builds would print their timings
   and fault records there, so a hang doesn't lose them).

## The VM

- **Host:** Windows on a Ryzen 7 5700X3D, shared with the user's own work: the VM gets **2-4 cores** and up to
  16 GB RAM (8-12 GB is plenty), 100 GB disk. No GPU passthrough.
- **Hypervisor:** one with good USB passthrough, since a video capture card has to work through it. VMware
  Workstation Pro (free for personal use) handles USB 3 webcams well; VirtualBox can with its USB 3 controller.
  Hyper-V and WSL2 can't pass USB through directly (WSL2 needs `usbipd`, and its kernel has no webcam driver), so
  not those.
- **USB passthrough:** two devices, by vendor/product id so they survive replugging:
  - the SummerCart64 (an FTDI FT232H: `0403:6014`; it shows up as `/dev/ttyUSB0`);
  - the capture card (a UVC device; `lsusb` names it).
  Use a USB 3 (xHCI) controller in the VM; video over USB passthrough can drop frames or fail with the default
  controller. If it's unreliable, passing a whole PCIe USB card through to the VM fixes that.
- **No sleep or screen lock** (it runs unattended for hours).

## Software

### From the CachyOS (Arch) repos

```sh
sudo pacman -S --needed base-devel git openssh tmux \
    python python-pip python-pillow python-numpy imagemagick \
    ffmpeg v4l-utils \
    docker docker-buildx docker-compose
sudo systemctl enable --now docker
sudo usermod -aG docker,uucp,video "$USER"   # Docker, the serial port (SC64), the capture card; log in again after
```

- `ffmpeg` + `v4l-utils`: grab frames and video from the capture card (`v4l2-ctl --list-devices`, then
  `ffmpeg -f v4l2 -i /dev/video0 -frames:v 1 shot.png`). Capture cards often give audio as well (`arecord -l`):
  useful later for checking the fish-on music on hardware.
- `python-pillow`, `python-numpy`, `imagemagick`: the repo's tools (the item-page generator, the capture and report
  scripts).

### The build: Docker

Building the decomp natively on Arch needs MIPS cross tools that are only in the AUR, so the build runs in an Ubuntu
container instead (Docker, or a `distrobox` Ubuntu 24.04 box, which shares the home folder and feels native; both
are light on CPU when idle) (the repo has a `Dockerfile`; the mod needs a little more than it installs):
`gcc-mips-linux-gnu`, `binutils-mips-linux-gnu`, Python with `requirements.txt` plus Pillow and NumPy, and Flips for
the `.bps`. To do: a `fishmod/docker/Dockerfile` with those, so the VM builds exactly what the cloud container does.

### SummerCart64: `sc64deployer`

Its Linux build from the SummerCart64 releases on GitHub (`Polprzewodnikowy/SummerCart64`), unpacked to
`~/.local/bin`. It uses the kernel's FTDI serial driver (in CachyOS's kernel); the `uucp` group gives access to
`/dev/ttyUSB0`. Check with `sc64deployer list` and `sc64deployer info`. The SC64's firmware should match the
deployer's version (`sc64deployer firmware ...`).

### Claude Code with Remote Control

This cloud session reaches the VM through Remote Control: Claude Code runs on the VM, serving one folder, and the
session asks to attach to it (the user approves on their device).

- Install Claude Code (the native installer from the Claude Code docs) and sign in with the same account.
- Clone this repo on the VM (the folder must be a git repository's root, not inside a hidden folder), put the base
  ROM at `baseroms/ntsc-1.2/baserom.z64` (git-ignored), and run `claude remote-control` in it, inside `tmux` so it
  survives the terminal closing (or as a systemd user service that starts at boot).

## Open questions

- **Rebooting the console.** After an upload, the N64 has to be reset to boot the new ROM. Whether the SC64 can do
  that from USB isn't confirmed yet (check `sc64deployer --help` and its docs on the VM). If it can't, the options are
  pressing reset by hand for now, a USB relay or smart plug on the N64's power, or later the ESP32 wired to the reset
  line.
- **Capture card:** which one (model, resolution, deinterlacing of the N64's 240p). Its frames won't match ares's
  pixel for pixel; comparisons need to allow for that.
- **The cart's own SD card and save:** back up the save before every test run; the bench ROMs write their results
  over the start of SRAM (`save-format.md`), so a real save on the cart would be overwritten.
- **The ESP32 controller (later):** an N64 controller emulator on the ESP32's RMT peripheral, fed over USB serial
  from the VM; it would also let tests go through the title screen and file select like a player does.
