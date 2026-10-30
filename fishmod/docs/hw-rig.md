# The hardware test rig (plan)

A CachyOS VM on the user's PC with the SummerCart64's USB and a capture card for the N64's AV out passed through,
so Claude can put a build on the console, run it, watch it and read its results without the user swapping SD cards.
No controller input yet (an ESP32 pretending to be a controller comes later), so tests use ROMs that drive
themselves: the debug harness's autoplay scripts and the benches already do (`fishmod/scripts/`).

## Who does what

Measured: the cloud container is 4 vCPUs of a shared Xeon at 2.1 GHz with 15 GB and no GPU (a Python loop that
takes it 1.85 s should take a 5700X3D core about 1.0-1.2 s). So each VM core is worth roughly 1.5-2 cloud cores:
a 4-core VM out-runs the cloud container, a 2-core one about matches it, and neither has a GPU (ares already runs
here without one). The cloud's other limit is that it's reclaimed when idle, which cuts long runs short.

| Work | Where | Why |
|---|---|---|
| Talking to the cart and the capture card | VM | the hardware is there |
| Builds, open-ended ares runs, hardware tests | VM when it has its cores, cloud otherwise | the VM is faster and always on; but it shares the user's PC |
| Long soaks | **the console** | the real thing, and it costs the VM almost nothing (a frame grab now and then) |
| Many ares runs at once (A/B sets, tours) | cloud | so the user's PC stays free |
| Tests that take a fixed time (a 30-minute bench, a tour, a capture set) | cloud | they finish inside the container's life, and run alongside the VM's work |
| Anything when the VM is busy or the user needs their PC | cloud | the fallback |
| Asset generation (props, scatter, collision, item page) | either | quick |

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

## Profiling on the console (for the LLVM side project)

The compiler profile comes from real hardware, collected by the VM with nobody touching the console. The N64's CPU
(VR4300) has no performance counters: it can't count cache misses. What it can do is tell time exactly (its `Count`
register) and be interrupted, which gives two kinds of profile:

- **Where the time goes (sampling).** A profiling build sets the `Compare` timer to fire every few thousand cycles;
  the exception handler (libultra's, which the decomps have as source) records the interrupted PC into a buffer
  before handling the interrupt as usual. Time lost to cache misses and RDRAM waits lands on the instructions that
  waited, so this shows the misses even without a counter for them. Mapped through the build's debug line tables, it
  becomes LLVM's sample profile (text format, written by our own converter).
- **How often each branch goes each way (instrumented).** A build with LLVM's `-fprofile-instr-generate` counts
  edges in RAM. Counts don't depend on timing, so the counters' own cost doesn't spoil them: that's the profile LLVM's
  optimizer uses for layout, inlining and hot/cold splitting.

Each run plays the same input, so runs compare: the fish-oot builds already drive themselves (autoplay scripts);
other decomps would each need a small input-replay hook until the ESP32 controller can replay the same input file
into any game unmodified. The results stream to the VM over the SC64's USB debug link (or, short runs, the save).

**Hybrid, where the console can't see:** the console's numbers are the truth, but it can't say *why* time was
lost (which accesses missed which cache). ares emulates the VR4300's caches, so it can count misses per instruction.
Run the same build and input on both, compare the per-step times (the `FISHMOD_SECT` timers already do this on both),
and use ares's miss counts only where its timing agrees with the console's; where they disagree, the console wins and
the gap is itself worth knowing (ares's model is missing something there). The goal is the best result; the rig is
there to make the best convenient, not to settle for what's easy.

What the VM needs for this beyond the plan above: resetting the console between runs without a person (the open
question below), and the converter from PC samples to LLVM's profile format.

## Open questions

- **Rebooting the console** (needed for unattended runs, the profiling above most of all). After an upload, the N64 has to be reset to boot the new ROM. Whether the SC64 can do
  that from USB isn't confirmed yet (check `sc64deployer --help` and its docs on the VM). If it can't, the options are
  pressing reset by hand for now, a USB relay or smart plug on the N64's power, or later the ESP32 wired to the reset
  line.
- **Capture card:** which one (model, resolution, deinterlacing of the N64's 240p). Its frames won't match ares's
  pixel for pixel; comparisons need to allow for that.
- **The cart's own SD card and save:** back up the save before every test run; the bench ROMs write their results
  over the start of SRAM (`save-format.md`), so a real save on the cart would be overwritten.
- **The ESP32 controller (later):** an N64 controller emulator on the ESP32's RMT peripheral, fed over USB serial
  from the VM; it would also let tests go through the title screen and file select like a player does.
