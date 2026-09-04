# Phase 2 — Arena package (NumberFourCoding slice)

Scaffold root: this directory (`ue5-scaffold/`).

Extends NumberTwoCoding’s `AOfficeArena` bounds / ceiling / clamp / base spawns / cover stubs.
**Do not regress** `BoundsVolume`, `CeilingClamp`, `ClampToBounds`, or `EnforceBoundsOnActor`.
Stubs are UE-shaped only — this machine cannot compile Unreal.

## Files touched

| File | Change |
|---|---|
| `Source/NightShiftFloor37/Public/OfficeArena.h` | `FOfficeArenaSpawnPoint`; spawn data + cover helpers; resin NW/SE + rack proxies |
| `Source/NightShiftFloor37/Private/OfficeArena.cpp` | Default cover set expanded; labeled EnsureDefaultSpawns; line/AABB cover query |
| `LEVEL_SETUP_CHECKLIST.md` | Concrete Editor notes: atrium, cubicles, racks, resin, 8 named spawns |
| `PHASE2.md` | This summary |

## What changed (this slice)

| Area | Change |
|---|---|
| **Cover defaults** | Kept 4 cubicle boxes (~800 cm ring). Expanded resin to **4 clusters** (NE/NW/SE/SW). Added rack proxies: `RackStack_A`, `RackStack_B`, `RackAngled` (DESIGN: 6 racks — two stacked, one angled). All registered in `CoverVolumes`. |
| **Cover helpers** | `UnregisterCoverVolume`, `GetCoverVolumeCount`, `FindCoverAwayFromThreat` (farthest-from-threat among volumes within `CoverSearchRadiusCm` of From), `DoesLineHitCover` (segment vs each box OBB via local AABB). Kept `RegisterCoverVolume` / `IsPointInCover` / `GetNearestCoverPoint` / `SetupDefaultCoverVolume`. |
| **Spawn data** | `FOfficeArenaSpawnPoint` (`Id` + `Transform`). `AlienSpawnPointData` (EditAnywhere). `EnsureDefaultSpawns` fills 8 labeled edge points and **mirrors** transforms into `AlienSpawnPoints`. |
| **Spawn helpers** | `GetSpawnPointCount`, `GetSpawnPointById`. `GetFarthestSpawnFrom` prefers `AlienSpawnPointData` (falls back to `AlienSpawnPoints`). `GatherSpawnPointsFromActors` syncs **both** arrays (tag → ActorLabel → `Spawn_N`). |

## DESIGN map

| DESIGN | Code |
|---|---|
| ~50×50 m floor, invisible ceiling/bounds | `BoundsVolume` + `CeilingClamp` (NumberTwo; preserved) |
| Atrium ~14 m, fall >6 m | `AtriumTowerHeightCm`; fall damage via ArenaCollision / Character |
| Cubicle maze low cover | `DefaultCover_CubicleN/E/S/W` @ ±800 cm |
| 4 resin / egg barrel clusters | `DefaultCover_ResinNE/NW/SE/SW` @ ±1100 cm corners |
| 6 server racks (two stacked, one angled) | `RackStack_A/B` + `RackAngled` query boxes |
| 8 edge spawns: stairwells, dock, elevator, service corridor | Labeled `AlienSpawnPointData` Ids below |
| On spawn, farthest from player | `GetFarthestSpawnFrom` |

## Default 8-spawn layout

Centered on the arena actor. Walkable edge = `ArenaHalfExtentCm - 200` (~**2300 cm** when half-extent is 2500). **Z = arena origin Z** (floor).

| Id | Offset (cm) | Role |
|---|---|---|
| `Stairwell_N` | (0, +Edge) | Mid N stairwell |
| `Stairwell_S` | (0, −Edge) | Mid S stairwell |
| `LoadingDock_E` | (+Edge, 0) | Mid E loading dock |
| `ElevatorBank_W` | (−Edge, 0) | Mid W elevator bank |
| `ServiceCorridor_NE` | (+Edge, +Edge) | NE corner |
| `ServiceCorridor_NW` | (−Edge, +Edge) | NW corner |
| `ServiceCorridor_SE` | (+Edge, −Edge) | SE corner |
| `ServiceCorridor_SW` | (−Edge, −Edge) | SW corner |

Count follows `GameConfig::AlienSpawnPointCount` (default 8). If `SpawnPointActors` are wired and count ≥ expected, those transforms win and defaults are skipped.

## Cover usage

```
Arena->RegisterCoverVolume(MyBox);
Arena->UnregisterCoverVolume(MyBox);
const int32 N = Arena->GetCoverVolumeCount();
if (Arena->IsPointInCover(Loc)) { ... }
FVector CoverPt;
if (Arena->GetNearestCoverPoint(Loc, CoverPt)) { ... }
if (Arena->FindCoverAwayFromThreat(From, Threat, CoverPt)) { ... }
if (Arena->DoesLineHitCover(Start, End)) { ... }
```

`FindCoverAwayFromThreat`: O(n) scan — among cover centers within `CoverSearchRadiusCm` of `From` (all volumes if radius ≤ 0), pick the center **farthest from ThreatLocation**.

Prototype ships **11** default query boxes (4 cubicles + 4 resin + 3 rack proxies). Replace or add real art volumes for shipping.

## Bounds / ceiling (unchanged from NumberTwo)

```
Arena->EnforceBoundsOnActor(Pawn);
// or
Pawn->SetActorLocation(Arena->ClampToBounds(Pawn->GetActorLocation()));
```

- `BoundsVolume` half-extent XY synced to `ArenaSizeMeters * 50` cm.
- `CeilingClamp` sits at `AtriumTowerHeightCm + 200`.

## Editor-only leftovers

1. Place `AOfficeArena` (or BP) once in the persistent level.
2. Prefer **8 Target Point / empty actors** with tags/labels matching spawn Ids → `SpawnPointActors`.
3. Align art to default cubicle / resin / rack proxies; register extras via `RegisterCoverVolume`.
4. Build atrium (14 m, no rails), NavMesh on floors/ramps/stairs — see `LEVEL_SETUP_CHECKLIST.md`.
5. Assign shared `DA_GameConfig`.
6. **Compile in Editor** (no UE toolchain on this machine).

## Not in this phase

- Full NavMesh MoveTo AI
- Cooked build / art / audio
- GameMode population wiring beyond existing stubs
- Chaos / physics beyond CMC + cover query boxes
