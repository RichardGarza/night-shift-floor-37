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
