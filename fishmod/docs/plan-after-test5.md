# After test 5: the plan

In order: finish test 5, then lures and the pause menu, then the refactor, then content, then quests.

## 1. Lures, bottles and the pause menu (before the refactor)

- **Lures as a C-button item tied to the rod**, the way magic arrows are tied to the bow: equip a lure (or the
  sinker) and the rod casts with it.
- **The coral hook** as an upgrade to the rod (like Hookshot to Longshot), not a separate item.
- **The item grid redone.** The rod, the lures and the sinker break the vanilla grid's layout. Everything shrinks
  in proportion to make room. The number of extra bottles isn't fixed: they're there to fill the gaps the new
  items leave.
- **Fish in bottles say what they are:** species and size in the inventory (the item name or description), not
  just "fish".
- **The item broker who buys bottled things** acknowledges the species and size, and prices them by them.

## 2. The refactor

Mostly consolidation: keep everything that works, as one framework, instead of the "million compiler flags".

- **One build profile per target** instead of scattered switches. The **original-hardware** version (the
  constraining platform, the one the work is tuned on) and the **emulator** version (the definitive one: lusher,
  tall kelp forests, plants with more cards) as a first-class split. Each tool (habitat, scatter, thinning, LOD
  tables) takes the profile, and both are built and checked every release. See the roadmap's refactor note.
- **Notes cleaned up significantly**, older notes removed.
- **Everything unused removed** (dead experiments, switches nobody builds).
- **A framework for everything**: shared code for the tools, the benches, the capture scripts, the habitat and the
  fish, so new areas and features plug in instead of being special-cased.
- **Quality-of-life features:**
  - the D-pad as an extension of the C buttons;
  - boots, tunics and swords assignable to C buttons (from the test 5 feedback);
  - others still to be thought of.

## 3. Content

Foliage and fish for the rest of Hyrule's bodies of water with a decent natural source:

- Hyrule Field
- inside the Great Deku Tree
- Kokiri Forest and the Lost Woods
- the Zora's Domain entrance
- inside Zora's Domain (fish, little foliage)
- Gerudo Valley
- the Desert Oasis
- the Hyrule Castle moat
- behind King Zora
- anywhere else with a decent-sized natural water source (not, for example, the Kakariko well)

Also:
- **The beehive in Kokiri moves** somewhere that makes more sense for a beehive.
- **Drinking the larvae** for a very small heal, as in Twilight Princess, with the drinking animation.

## 4. Quests

Once everything looks and feels good. Ideas so far (more will be needed):

1. **A new Kokiri who gives you the rod** with a bobber, for knocking down the beehive: he says he uses the larvae
   for bait. You've just finished the first dungeon and have no bottle yet, but your first bottle is close, and
   you realise it can carry bait.
2. **Giving fish to the Lake Hylia lab.** It keeps your fish in its water; enough variety earns you something.
3. **The Fishing Pond's sinking lure:** the owner is still upset, but essentially tells you to take it with you.
4. **Fishing-related bottle quests.**
5. **Ways to get the other lures and sinkers.**
6. **Bait in the Fishing Pond's quest.**
