# The morning runner

`morning.bat` runs bench ROMs on the console one after another and sends the results back, with nobody there after
the first minute.

## Once

1. The repository on this branch: `git fetch origin claude/harkinian-tp-fishing-refactor-clean`, then
   `git checkout claude/harkinian-tp-fishing-refactor-clean`. After that, each run starts with a `git pull`.
2. Python: the `fishoot` environment from `setup.bat` is found on its own, with no Anaconda Prompt needed. Any other
   Python 3 works too, because the runner needs nothing beyond the standard library.
3. The SummerCart64 connected over USB, with `sc64deployer.exe` anywhere under Downloads, Desktop or Documents, or
   on PATH, or in `fishmod\pc\`. If it isn't found, the runner asks once and remembers the answer.

## Each time

1. Switch the console on, then double-click `fishmod\pc\morning.bat`.
2. On the first run it asks for your ROM's path (OoT NTSC 1.2, any byte order) and remembers it. The ROM stays where
   it is: each patched copy is made in `fishmod\pc\work\` and deleted after its run.
3. If the console isn't already running a bench build that answers the reboot request (`frames7` or later), the
   runner beeps and asks you to **press RESET** once. After that it needs nobody: each run ends with the game saying
   it's done, and the next ROM is loaded with `sc64deployer upload --reboot`.

The queue is `fishmod\pc\morning_queue.txt`, one run a line (patch, most minutes, label). You can also drop .bps
files onto `morning.bat` to run just those.

## The results

Each morning gets a folder, `fishmod\pc\results\morning\runs\<date-time>\`. It holds a folder per run with
`upload.log`, `debug.log`, `framelog.txt` (the frame log's summary) and `packets.zip` (everything the game sent), plus
a `summary.txt` for the whole morning.

The folder is committed to the `morning-results` branch and pushed, without touching your checkout, so I can read it
there. `fishmod\pc\results\morning_<date-time>.zip` is made either way: if the push fails (no git login, say), send
that instead.

## When it stops early

If a game stops answering (it hung, or the reboot didn't take), the runner can't go on without someone pressing
RESET. It stops, keeps what it has, and pushes that. Run it again to do the rest.

The PC is kept awake while it runs. A bench route takes about 11 minutes, plus a minute for each upload.
