# frames8b: frames8 that can't freeze on a stuck USB link; two test variants

frames7 froze on the console about 16 seconds into the route, and frames7-noclaim after about 7. The game was still
running (it answered the next run's halt request), so it was crawling rather than crashed. The likeliest cause is the
SummerCart64's USB link no longer finishing its commands: the old code polled the cart up to 100,000 times for each
command, several times a frame, which slows the game to about a frame a second.

**`fish-oot-frames8b.bps`**: frames8 (see [its notes](history/notes-frames8.md)) plus:
- **A cart command not finished in 3,000 polls (a few ms) is given up on,** and the USB sending is left alone for 30
  frames, so a stuck link can't slow the game.
- **When the link works again,** the game says so in the debug log: `FISHMOD-USB-STALL` with the number of commands
  given up on, the frame it started at, and the cart's status register.
- **The waits for the start and done messages are bounded** (they were unbounded).

**`fish-oot-frames8b-nostore.bps`**: the same without the audio's sample store (the samples stream from the cart as
in frames6). **`fish-oot-frames8b-noaux.bps`**: the same without the reboot handshake's poll of the cart every frame (it
can't be rebooted by the runner: it runs last).

Which of these completes the route says what froze frames7: the per-frame poll, the sample store's background loading
from the cart, or neither (then the stall report says what the cart was doing).
