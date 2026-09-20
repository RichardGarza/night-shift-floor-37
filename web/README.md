# Night Shift — Floor 37 (web prototype)

Playable Three.js build of `../DESIGN.md`: 3rd-person over-the-shoulder arena shooter, 50 × 50 m office floor with a 3-story atrium, 6 aliens live at a time, win at 25 kills.

## Run

Any static file server from this folder works. Three.js is loaded from the esm.sh CDN via the import map in `index.html`, so the page needs network access.

```
cd web
python3 -m http.server 8000
# open http://localhost:8000
```

Click the overlay to start (pointer lock). Esc pauses. Controls are listed on the start screen and in `DESIGN.md`.

## Layout

| File | Owns |
|---|---|
| `js/config.js` | Every tunable (mirrors `UGameConfig` in `../ue5-scaffold`) |
| `js/game.js` | Renderer, input, state machine, main loop |
| `js/arena.js` | Static geometry, lights, collision solids, 8 spawn points |
| `js/player.js` | Movement, camera, recoil, health |
| `js/rifle.js` | Ammo, fire rate, reload, hitscan |
| `js/combat.js` | Raycast, pooled tracers / muzzle lights, hit marker |
| `js/alien.js` | Bot AI, steering, line of sight, burst fire, respawn pool |
| `js/collision.js` | AABB / OBB helpers, ramp height |
| `js/hud.js` | DOM overlay |

No build step, no dependencies beyond the CDN import.

## Softer start

Mirrors UE `UGameConfig` **Match|EarlyGame** (Sprint V). Mid/late DESIGN combat numbers (accuracy, damage, 6 aliens, win at 25 kills, etc.) are unchanged.

| Tunable | Value | Effect |
|---|---|---|
| `spawnGraceSeconds` | **7** | Match start / restart grace window |
| `spawnGraceBlocksAlienAggro` | **true** | Aliens idle during grace (no chase / fire) |
| `spawnGracePlayerDamageImmune` | **true** | Player takes no damage during grace |
| `minStartSeparationMeters` | **24** | Hard floor on start + mid-match respawn picks (prefer ≥24 m; else farthest) |
| `postGraceAlienFireDelaySeconds` | **1.5** | After grace: aliens may chase/strafe but cannot fire for 1.5 s |

Grace timers reset only in `softReset()` (new match / restart). Pause / resume does **not** reset grace or the post-grace fire delay.


## OTS camera collision

Shipped (`520d834`). When walls/cover block the over-the-shoulder camera, it pulls in along the pivot→desired ray.

| Tunable | Value | Effect |
|---|---|---|
| `camera.collisionSkin` | **0.2** | Keep cam this far inside the hit surface |
| `camera.minDistance` | **0.65** | Floor on pull-in so the camera never collapses into the player |

Uses `raycastCameraSolids` in `collision.js`; `player.js` sets `_camSolids` from arena solids each frame.

## Mouse sensitivity

Web camera look uses `CONFIG.camera.defaultMouseSensitivity` (**0.18**), mirroring UE `UGameConfig::DefaultMouseSensitivity` (was 0.35; Richard found that too sensitive).

`mouseSens` is radians per pointer-lock `movementX`/`movementY` pixel: **0.001131** ≈ legacy `0.0022 × (0.18 / 0.35)` so the web default tracks the same ratio as UE.

## Recoil

Mirrors UE rifle recoil defaults:

| Tunable | Web | UE |
|---|---|---|
| Max pitch kick | `recoilPitch` **0.027** rad (~1.55°) | `RecoilPitchMaxDegrees` 1.55 |
| Pitch min fraction | `recoilPitchMinFraction` **0.45** | `RecoilPitchMinFraction` 0.45 |
| Yaw kick | `recoilYaw` **0.0087** rad (~0.5°) | `RecoilYawMaxDegrees` 0.5 |
| Recover | `recoilRecover` **14** via FInterpTo-to-zero | `RecoilRecoverySpeed` 14 (`FInterpTo`) |

Each shot picks pitch in `[minFraction × max, max]` (no overshoot above max). Recover uses the same lerp-to-zero as UE `FInterpTo` (`alpha = clamp(dt × speed, 0, 1)`).
