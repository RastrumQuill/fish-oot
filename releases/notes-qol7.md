# qol7: the fish's new behaviour, to play

**`fish-oot-qol7-testmode.bps`** (emulators), **`fish-oot-qol7-hw-testmode.bps`** (console): qol6 (see
[its notes](history/notes-qol6.md)) plus the fish AI work (the same as frames9's, see [its notes](history/notes-frames9.md)):

- **Schools:** one fish takes the bait, not the school. A fish that goes to the bobber, gets hooked or is caught leaves
  its school, and the next fish leads the rest; it rejoins when it's free. Members keep a little room between them.
- **Each fish has a day:** a home by cover and a feeding place a little way off, where it goes when its species is
  active (dawn and dusk for most). Away from Link it carries on: come back later and it's where its day took it.
  **Chase a fish off and leave, and it's still on its way when you come back** (it appears where it got to, not back
  home).
- **Wading or swimming scares the fish round about** (each by its own nerves: wary and frightened fish from further,
  and more when Link moves fast).
- **Natural bait has a scent** (larvae, bugs, worms, fish): the longer it soaks, the further off fish smell it (to
  about 140 units after a minute), and they come to it seen or not.
- **A fish learns a bait:** one that got away with yours, or broke the line, mostly leaves that bait alone for the
  rest of the day. Try another bait, or another fish.
- Far-away fish cost less (school members out past 300 units just keep their places) and the shores no longer cost
  repeated collision raycasts.

Things to look for: schools splitting and re-forming round a catch, fish moving between cover and feeding grounds at
dawn and dusk, the scared ring round Link in the water. The fish behave differently from qol6, so tell me what feels
wrong.
