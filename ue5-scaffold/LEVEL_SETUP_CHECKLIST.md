# Level setup checklist — Floor 37

One **persistent** level. Soft restart never unloads it.

## Layout

- [ ] Map size ~**50 × 50 m** walkable office floor + open atrium
- [ ] Center: 3-story atrium stair/catwalk tower ~**14 m**, ramps/stairs, **no rails**
- [ ] Around atrium (walkable on top):
  - [ ] Cubicle maze (low cover)
  - [ ] 6 server racks / IT cages (two stacked, one angled)
  - [ ] Cable tray / pipe run at **1.5 m** height
  - [ ] Raised conference pad / broken glass boardroom
  - [ ] Collapsed drywall berm + planters
  - [ ] 4 clusters of alien egg / resin barrels (low cover)
- [ ] Perimeter: exterior glass + low planters
- [ ] Invisible **bounds** + **ceiling clamp** (`AOfficeArena` volumes)
- [ ] Fall damage still applies for atrium drops **> 6 m**

## Gameplay actors

- [ ] Place one `AOfficeArena` (or BP child)
- [ ] **8** alien spawn point markers at edges: stairwells, loading dock, elevator bank, service corridor
- [ ] Wire markers into `AOfficeArena::SpawnPointActors`
- [ ] Cover volumes registered on arena (optional query boxes)
- [ ] NavMesh Bounds Volume covering walkable floors, ramps, stairs (tower contested)
- [ ] Player start in a readable spawn (not atrium top)
- [ ] World Settings: GameMode → `AArenaGameMode` (or BP)
- [ ] Assign shared `UGameConfig` data asset everywhere

## Look (later — Nanite + Lumen)

- [ ] Mood: sick green / amber practicals, wet floors, dirty glass sun, dusty volumetric fog
- [ ] Dreary futuristic punk — no logos/brands; readable dirt/wetness/neon trim
- [ ] Shared meshes/materials; keep dynamic light count tight
- [ ] Nanite static meshes + Lumen GI when art lands

## Pools / perf

- [ ] Place or spawn `AFXPoolManager` (or equivalent) — **tracer pool 32**, **muzzle light pool 8**
- [ ] No per-frame allocations in fire / AI / push-apart paths
- [ ] Clamp dt spikes (~50 ms) via GameConfig / GameMode

## UI

- [ ] WBP child of `UHUDWidget`: crosshair, HP, ammo `30 / 90`, kills, timer, prompts
- [ ] Create HUD on match begin; bind to GameMode + Character
