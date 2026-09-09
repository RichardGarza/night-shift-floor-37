# Sprint AG — hands-off autopilot demo (2026-09-09)

Richard: "make a hands-off demo where it'll play itself for like 60 seconds to demonstrate the game
without user input." Also fixed on the way: "the audio has a ton of static in the background".

## Demo / attract mode

- **Triggers**: `-NightShiftDemo` on the command line (optionally `-NightShiftDemoSeconds=N`), or sitting on
  "Click to play" for `UGameConfig::DemoIdleSeconds` (25 s; 0 disables the idle trigger). Not during the
  self-test.
- **`ANightShiftDemoPilot`** (spawned by `AArenaGameMode::StartDemo`) drives the real player each tick:
  picks the nearest visible alien (distance weighted by how far off-axis it is), tracks its upper chest with
  a smoothed aim plus a little hand wobble, fires only when within 3.5° and 32 m, holds a 7–14 m band from
  the target while strafing with random-period direction flips, drifts toward the atrium when nothing is in
  sight, steers off the walls, tops up the mag between fights, sprints only when no target is visible.
- The autopilot takes `DemoDamageScale` (0.45) of incoming damage so a 60 s demo survives; if it still dies,
  the match soft-restarts under autopilot after a 2 s beat.
- HUD corner note "AUTOPILOT DEMO — click to take over". **Any click** ends the demo and starts a fresh
  match for the human; otherwise after `DemoSeconds` (60) the game returns to "Click to play" and the idle
  timer starts again (attract loop).
- Smoke: `-NightShiftDemo -NightShiftDemoSeconds=45` → wave 1 cleared at 18.6 s, wave 2 at 32.6 s, no death;
  the 20 s run logged `Demo ended after 20 s — 6 kills`. `docs/sprintag_autopilot_demo.png`.

## Audio static fix

The ambient loop was mains hum + *filtered white noise*, which hissed. Regenerated as pure 60/120/180/240 Hz
partials with a slow breath plus a hard-filtered (4 × 70 Hz) HVAC rumble — energy above 400 Hz is ~0.001 %
of the signal. `AmbientVolume` default 0.45 → 0.3. Self-test 49 / 49.
