# Level setup checklist — Floor 37

One **persistent** level. Soft restart never unloads it.

## Layout

- [ ] Map size ~**50 × 50 m** walkable office floor + open atrium
- [ ] **Atrium (center)**
  - [ ] 3-story stair/catwalk tower ~**14 m** (`AtriumTowerHeightCm` / `GameConfig::AtriumTowerHeightMeters`)
  - [ ] Ramps + stairs connect platforms; **no rails**
  - [ ] Top = best sightline / worst place to get caught
  - [ ] Fall damage still applies for atrium drops **> 6 m** (`FallDamageHeightMeters`)
- [ ] **Cubicle maze (low cover)**
  - [ ] Walkable on top of desks/partitions
  - [ ] Prototype query boxes: `DefaultCover_CubicleN/E/S/W` on a ~**800 cm** ring from arena center (~1 m tall)
  - [ ] Replace with real cubicle meshes; register extras via `RegisterCoverVolume`
- [ ] **6 server racks / IT cages**
  - [ ] Two stacked pairs + one angled unit (DESIGN)
  - [ ] Prototype proxies: `DefaultCover_RackStack_A`, `DefaultCover_RackStack_B`, `DefaultCover_RackAngled` (35° yaw)
  - [ ] Place real rack meshes near atrium (not blocking all sightlines); wire cover boxes into `CoverVolumes`
- [ ] Cable tray / pipe run at **1.5 m** height
- [ ] Raised conference pad / broken glass boardroom
- [ ] Collapsed drywall berm + planters
- [ ] **4 resin / egg barrel clusters (low cover)**
  - [ ] DESIGN: four clusters around atrium
  - [ ] Prototype: `DefaultCover_ResinNE/NW/SE/SW` at ~**(±1100, ±1100)** cm
  - [ ] Art: sticky resin read, still traversable as low cover
- [ ] Perimeter: exterior glass + low planters
- [ ] Invisible **bounds** + **ceiling clamp** (`AOfficeArena` volumes — NumberTwoCoding bounds slice)

## Spawns (8 edge points)

Place markers at map edges matching DESIGN labels (or rely on Phase 2 procedural defaults at ±~**2300 cm**):

| Id | Edge | Role |
|---|---|---|
| `Stairwell_N` | Mid +Y | Stairwell |
| `Stairwell_S` | Mid −Y | Stairwell |
| `LoadingDock_E` | Mid +X | Loading dock |
| `ElevatorBank_W` | Mid −X | Elevator bank |
| `ServiceCorridor_NE` | +X +Y | Service corridor |
| `ServiceCorridor_NW` | −X +Y | Service corridor |
| `ServiceCorridor_SE` | +X −Y | Service corridor |
| `ServiceCorridor_SW` | −X −Y | Service corridor |

- [ ] **8** alien spawn markers (Target Points / empty actors) named or tagged with the Ids above
- [ ] Wire into `AOfficeArena::SpawnPointActors` (gather prefers actor tags → label → `Spawn_N`)
- [ ] Confirm `AlienSpawnPointData` fills on BeginPlay / `RefreshSpawnGather` (transforms mirrored to `AlienSpawnPoints` for legacy callers)
- [ ] Soft reset calls `RefreshSpawnGather` if markers change at runtime

## Cover API (runtime)

- [ ] Default cover set registered in ctor (4 cubicles + 4 resin + 3 rack proxies)
- [ ] Optional Editor boxes: `RegisterCoverVolume` / `UnregisterCoverVolume`
- [ ] AI helpers available: `IsPointInCover`, `GetNearestCoverPoint`, `FindCoverAwayFromThreat`, `DoesLineHitCover`

## Gameplay actors

- [ ] Place one `AOfficeArena` (or BP child)
- [ ] NavMesh Bounds Volume covering walkable floors, ramps, stairs (tower contested)
- [ ] Player start in a readable spawn (not atrium top)
- [ ] Optional: GameMode/Character tick `EnforceBoundsOnActor` for hard floor clamp
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
