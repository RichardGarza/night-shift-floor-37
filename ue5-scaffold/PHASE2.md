# Phase 2 — Arena cover + spawn data (NumberFourCoding slice)

Scaffold root: this directory (`ue5-scaffold/`).

Builds on NumberTwoCoding’s `AOfficeArena` bounds/ceiling/clamp core. This slice owns **cover volume helpers**, **labeled 8-edge spawn point data**, and **level checklist** placement notes. Bounds/core left alone. Stubs are UE-shaped — not compilable on this machine.

## What changed

| Area | Change |
|---|---|
| **`FOfficeArenaSpawnPoint`** | `USTRUCT`: `FName Id` + `FTransform Transform` |
| **`AlienSpawnPointData`** | Preferred labeled spawn array; mirrors into legacy `AlienSpawnPoints` |
| **`EnsureDefaultSpawns`** | Fills 8 DESIGN Ids on walkable square (±half-extent − 200 cm): Stairwell_N/S, LoadingDock_E, ElevatorBank_W, ServiceCorridor_NE/NW/SE/SW |
| **Spawn helpers** | `GetSpawnPointCount`, `GetSpawnPointById`, `GetFarthestSpawnFrom` prefers data array |
| **Gather** | `GatherSpawnPointsFromActors` syncs both arrays (tag → label → `Spawn_N`) |
| **Default cover** | 4 cubicles + **4 resin** (NE/NW/SE/SW) + **RackStack_A/B** + **RackAngled** (35° yaw) |
| **Cover helpers** | `UnregisterCoverVolume`, `GetCoverVolumeCount`, `FindCoverAwayFromThreat`, `DoesLineHitCover` (line vs OBB) |
| **Docs** | `LEVEL_SETUP_CHECKLIST.md` atrium/cubicles/racks/resin/spawn table; this `PHASE2.md` |

## DESIGN → code map

| DESIGN | Code |
|---|---|
| 8 fixed edge spawns (stairwells, dock, elevator, corridor) | `AlienSpawnPointData` + `EnsureDefaultSpawns` Ids |
| On spawn, farthest from player | `GetFarthestSpawnFrom` |
| Cubicle maze low cover | `DefaultCover_CubicleN/E/S/W` (~800 cm ring) |
| 4 resin / egg barrel clusters | `DefaultCover_ResinNE/NW/SE/SW` (~±1100 cm) |
| 6 server racks (two stacked, one angled) | `RackStack_A/B` + `RackAngled` query proxies |
| Cover for AI / soft traces | `IsPointInCover`, `FindCoverAwayFromThreat`, `DoesLineHitCover` |
| ~50×50 m + 14 m atrium | Synced via NumberTwo `SyncLayoutFromConfig` (untouched) |

## Cover search note

`FindCoverAwayFromThreat(From, Threat, Out)` picks the registered cover **center farthest from Threat** among volumes within `CoverSearchRadiusCm` of `From` (default 2500; `≤0` = all cover). Cheap break-contact heuristic — not full tactical cover evaluation.

## Files touched

- `Source/NightShiftFloor37/Public/OfficeArena.h`
- `Source/NightShiftFloor37/Private/OfficeArena.cpp`
- `LEVEL_SETUP_CHECKLIST.md`
- `PHASE2.md` (this file)

## Editor still required

1. Place `AOfficeArena` once in the persistent level.
2. Prefer 8 tagged Target Points → `SpawnPointActors` (overrides procedural defaults).
3. Replace cover proxies with real meshes; register extras via `RegisterCoverVolume`.
4. NavMesh + art per checklist.
5. Assign shared `DA_GameConfig`.
6. Compile in Editor (no UE toolchain here).

## Not in this slice

- Bounds / ceiling clamp ownership (NumberTwoCoding)
- Full NavMesh MoveTo AI
- GameMode population / alien match wiring beyond existing stubs
- Cooked build / art / audio
- Commit / push
