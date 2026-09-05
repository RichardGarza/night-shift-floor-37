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

### Editor placement notes — atrium

- [ ] Place atrium tower at arena origin (world XY of `AOfficeArena`)
- [ ] Height target **~1400 cm** (`UGameConfig::AtriumTowerHeightMeters` → arena `AtriumTowerHeightCm` / DESIGN 14 m); 3 open platforms + ramps/stairs between
- [ ] **No railings** on platforms or stairs — fall risk is intentional
- [ ] Ensure any atrium drop **> 6 m** still triggers fall damage (`FallDamageHeightMeters`)
- [ ] Keep top platform inside `CeilingClamp` (sits at tower height + ~2 m headroom)
- [ ] NavMesh must cover ramps/stairs so aliens can contest the tower

### Editor placement notes — cubicle maze (low cover)

- [ ] Build cubicle desks/partitions as low cover (~1 m tall) around the atrium
- [ ] Reference default query-box ring: cubicle centers at ~**±800 cm** from arena center (N/E/S/W)
  - Default extents: ~220×80×55 cm (long axis along the ring tangent)
- [ ] Walkable on top of desks if art allows; collision should still block standing sightlines at torso height
- [ ] Optional: register extra `UBoxComponent` cover volumes via `CoverVolumes` or `RegisterCoverVolume`

### Editor placement notes — server racks / IT cages

- [ ] Place **6** server racks / IT cages around atrium (DESIGN)
  - [ ] **Two stacked** pairs → tall cover (art can read taller; query proxy half-extent Z is **120 cm** → ~240 cm full height)
  - [ ] **One angled** (~35° yaw proxy) single rack
- [ ] Scaffold proxies already on `AOfficeArena` (query-only, hidden):
  - [ ] `DefaultCover_RackStack_A` @ ~(450, −550) cm, extent ~90×60×120 (stacked pair proxy)
  - [ ] `DefaultCover_RackStack_B` @ ~(−500, 450) cm, same stacked extent
  - [ ] `DefaultCover_RackAngled` @ ~(200, 600) cm, yaw **35°**, extent ~90×60×90
- [ ] Align final meshes to those proxies (or move proxies to match art)
- [ ] Wire any extra rack collision boxes into `CoverVolumes` / `RegisterCoverVolume`

### Editor placement notes — resin / egg barrel clusters

- [ ] Place **4** alien egg / resin barrel clusters at corner-ish positions around atrium
- [ ] Default cover proxies (query boxes, ~120×120×70 cm):
  - [ ] `DefaultCover_ResinNE` @ ~(1100, 1100) cm
  - [ ] `DefaultCover_ResinNW` @ ~(−1100, 1100) cm
  - [ ] `DefaultCover_ResinSE` @ ~(1100, −1100) cm
  - [ ] `DefaultCover_ResinSW` @ ~(−1100, −1100) cm
- [ ] Art clusters should read as low cover; keep tops walkable if meshes allow
- [ ] Confirm all four (plus cubicles + racks) appear in `CoverVolumes` after place

## Gameplay actors

- [ ] Place one `AOfficeArena` (or BP child)
- [ ] **8** alien spawn point markers at edges: stairwells, loading dock, elevator bank, service corridor
- [ ] Wire markers into `AOfficeArena::SpawnPointActors` (optional — Phase 2 auto-generates 8 edge defaults at ±~2300 cm if empty)
- [ ] Cover volumes registered on arena (optional — Phase 2 ships cubicle + 4 resin + rack-stack proxies; replace for final art)
- [ ] Call / rely on `RefreshSpawnGather` after soft reset if markers change at runtime
- [ ] Optional: GameMode/Character tick `EnforceBoundsOnActor` for hard floor clamp
- [ ] NavMesh Bounds Volume covering walkable floors, ramps, stairs (tower contested)
- [ ] Player start in a readable spawn (not atrium top)
- [ ] World Settings: GameMode → `AArenaGameMode` (or BP)
- [ ] Assign shared `UGameConfig` data asset everywhere

### Editor placement notes — 8 named edge spawn markers

Place Target Points (or empty actors) on the walkable square edge (`ArenaHalfExtentCm − 200` ≈ **±2300 cm** when half-extent is 2500). Z = floor. Tag or label with the Id so `GatherSpawnPointsFromActors` fills both `AlienSpawnPointData` and `AlienSpawnPoints`.

| Id | Position (relative to arena) | DESIGN role |
|---|---|---|
| `Stairwell_N` | (0, +Edge) | Mid north stairwell |
| `Stairwell_S` | (0, −Edge) | Mid south stairwell |
| `LoadingDock_E` | (+Edge, 0) | Mid east loading dock |
| `ElevatorBank_W` | (−Edge, 0) | Mid west elevator bank |
| `ServiceCorridor_NE` | (+Edge, +Edge) | NE corner corridor |
| `ServiceCorridor_NW` | (−Edge, +Edge) | NW corner corridor |
| `ServiceCorridor_SE` | (+Edge, −Edge) | SE corner corridor |
| `ServiceCorridor_SW` | (−Edge, −Edge) | SW corner corridor |

- [ ] Create 8 markers with Ids above (Actor Tag preferred; ActorLabel ok in Editor)
- [ ] Add all 8 to `AOfficeArena::SpawnPointActors` (order optional)
- [ ] PIE / call `RefreshSpawnGather` — confirm `AlienSpawnPointData` has 8 named entries and `AlienSpawnPoints` mirrors transforms
- [ ] If leaving `SpawnPointActors` empty, rely on `EnsureDefaultSpawns` procedural fill (same Ids/offsets)
- [ ] Soft reset path should re-call `RefreshSpawnGather` so marker moves stick

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
