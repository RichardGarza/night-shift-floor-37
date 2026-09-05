# Night Shift — Floor 37

Hyper-real 3rd-person arena shooter for Unreal Engine. Small map. Dreary futuristic-punk office. Aliens.

This is the original Three.js FPS spec rewritten for UE. It is the shared design contract for the two implementations in this repo: `web/` (playable Three.js prototype, see `web/README.md`) and `ue5-scaffold/` (UE 5.8 C++ module plus Editor drop-in docs, see `ue5-scaffold/README.md`). Layout, status, and next steps live in `PROJECT_MAP.md`.

---

## Pitch

One rainy night on Floor 37. Wet tile, dying fluorescents, cheap holograms, sealed glass, alien resin on the cubicles. You clear the floor. They keep coming.

Feel: readable, violent, tight. Best sightline is the atrium. Worst place to get caught is also the atrium.

---

## Engine and stack

- Unreal Engine (5.x)
- Third Person template + Enhanced Input
- One persistent level
- Nanite + Lumen for the look
- Shared meshes and materials
- Object pools for tracers and muzzle lights
- No Chaos vehicles, no audio requirement for the prototype, no external marketplace packs required
- One `UGameConfig` data asset holds every tunable (the old `CONFIG` object)

Target: 60 fps on mid / integrated-class hardware. Keep light count tight. No per-frame allocations in hot paths where you can avoid them.

---

## Camera and controls

3rd person, over-the-shoulder, right shoulder default.

| Input | Action |
|---|---|
| WASD | Move |
| Mouse | Orbit / aim |
| Left click | Shoot |
| R | Reload |
| Shift | Sprint |
| Space | Jump / mantle |
| Q | Shoulder swap |
| Esc | Pause / unlock |

Soft lock when an alien is in the reticle cone. Full free-aim still works.

---

## Player

- 100 HP
- Regen 10 HP/s after 5 s without damage
- Capsule / eye height ~1.8 m so cover heights still read in 3rd person
- Walk 6 m/s
- Sprint 9 m/s
- Jump 5 m/s
- Gravity 15 m/s²
- Fall damage above 6 m

### Rifle

- 30-round mag, 90 reserve
- 600 RPM full-auto
- Hitscan
- 25 dmg body / 50 head
- Small random recoil kick that recovers
- 1.5 s reload

---

## Map

Small office floor + open atrium. About 50 × 50 m. Walkable everywhere that looks walkable.

Mood: sick green / amber practicals, wet floors, long shadows from a dying sun through dirty glass, dusty volumetric fog. Dreary futuristic punk. No logos, no brand names, no texture-pack noise in the prototype — readable materials, dirt, wetness, neon trim.

### Center

3-story open atrium stair / catwalk tower (~14 m). Open platforms. Connected by ramps and stairs. No rails. Top is the best sightline and the worst place to get caught.

### Around the atrium (all walkable on top)

- Cubicle maze as low cover
- 6 server racks / IT cages (two stacked, one at an angle)
- Cable tray / pipe run at 1.5 m height
- Raised conference pad / glass-broken boardroom
- Collapsed drywall berm + planter beds
- 4 clusters of alien egg / resin barrels as low cover

### Perimeter

Exterior glass + low planter walls. Invisible ceiling / bounds clamp so nobody leaves the floor.

Fall damage still applies if you drop more than 6 m inside the atrium.

---

## Aliens

6 live at a time.

- Capsule body, one bright bio-color so they read in the gloom
- Head = top 25% of capsule
- Spawn at 8 fixed edge points: stairwells, loading dock, elevator bank, service corridor
- On spawn, pick the point farthest from the player
- Move toward the player at 4 m/s
- Navmesh + simple raycast / steering around obstacles
- They can walk ramps and stairs — the tower is contested, not safe
- Waypoint graph is fine if steering alone cannot climb

### Combat AI

At ≤12 m with line of sight:

- Stop
- Strafe left / right
- Fire a 3-round hitscan burst every 1.5 s
- 30% accuracy
- 10 dmg per hit

Kill: 3 body hits or 2 headshots.

Dead alien respawns after 3 s.

Aliens collide with the map and each other (cheap sphere push-apart).

---

## Combat feedback

- Raycast from camera / muzzle
- On hit: alien flashes white 80 ms
- Hit-marker on crosshair
- 60 ms tracer line from muzzle to impact
- Brief point-light muzzle flash
- Player taking damage: red edge vignette + camera shake
- No bullet holes required for the prototype

---

## HUD

DOM / UMG overlay:

- Crosshair
- HP bar
- Ammo `30 / 90`
- Kill count
- Timer
- Start: “Click to play”
- Death: “You died — click to restart”
- Win at 25 kills, show time

Restart resets match state without unloading the level.

---

## Collision and movement

Character Movement Component + capsule vs world.

Hand-rolled extra traces only where CMC is not enough (cover heights, atrium drops, cheap bot push-apart).

No physics library beyond what UE already gives you. No ragdoll requirement for v1 — collapse / hide / respawn is enough.

---

## Win / lose

- Die → restart prompt
- 25 kills → win screen with time
- Soft reset: HP, ammo, kills, timer, alien state, player transform. Same level.

---

## Performance rules

- Share geometries and materials
- Pool tracers and muzzle lights
- Clamp dt (treat spikes above ~50 ms as 50 ms)
- One config data asset
- Keep Niagara and Lumen cost in check — practical lights, not a forest of dynamic spots

---

## Code / content shape

Keep it small on purpose.

Suggested modules (UE, not the old JS files):

| Module | Owns |
|---|---|
| `UGameConfig` | All tunables |
| `AArenaGameMode` | Match, timer, kills, win/lose, restart |
| `AOfficeArena` | Bounds, spawn points, cover volumes |
| `ANightShiftCharacter` | Move, camera, health, recoil |
| `URifleComponent` | Fire, reload, ammo, traces |
| `AAlienBot` | Move, strafe, burst fire, flash, death |
| `UArenaCollision` | Extra traces, push-apart, fall damage |
| `UHUDWidget` | Crosshair, HP, ammo, prompts |

Plain actors and components over deep class trees. Nothing important hanging off globals.

---

## What this is not

Not a cooked Unreal build and not a marketplace pack list. `ue5-scaffold/` is source only: it ships no `.uasset` content, so the Editor assets in `ue5-scaffold/EDITOR_DROP_IN.md` still have to be created by hand.

If you need something playable in a browser right now, serve `web/` (same spec, Three.js, 3rd person, office floor, aliens).
