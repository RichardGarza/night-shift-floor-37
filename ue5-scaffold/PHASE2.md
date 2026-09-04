# Phase 2 — Arena package

Scaffold root: this directory (`ue5-scaffold/`).

Hardens `AOfficeArena` + `UArenaCollision` from Phase 0/1 stubs into real logic that runs **without Editor art** (procedural defaults) while keeping full APIs for `AArenaGameMode` / `AAlienBot`. Phase 0/1 public APIs preserved; new helpers are additive.

## Deliverables

| Area | What shipped |
|---|---|
| **ClampToBounds** | OBB clamp into `BoundsVolume` local box; Z also capped below `CeilingClamp` underside |
| **EnforceBoundsOnActor** | Sets actor location to clamped point if outside (ceiling-only when XY inside) |
| **EnsureDefaultSpawns** | If gather yields fewer than `AlienSpawnPointCount`, builds 8 edge transforms |
| **RefreshSpawnGather** | Soft-reset helper: gather markers → ensure defaults |
| **Cover API** | `RegisterCoverVolume`, `IsPointInCover`, `GetNearestCoverPoint` |
| **Default cover** | 4 cubicle boxes + 2 resin clusters as subobjects around atrium |
| **Layout metadata** | `ArenaHalfExtentCm`, `AtriumTowerHeightCm` synced from `UGameConfig` on BeginPlay |
| **PushApartNearbyAliens** | Sphere overlap into member `OverlapScratch`; XY half-penetration push |
| **ApplyFallDamageIfNeeded** | `(FallMeters - Threshold) * 15` HP via `TakeDamage` |
| **Traces** | Real `LineTraceSingleByChannel` for ground (down) and cover height (up) |

## Default 8-spawn layout

Centered on the arena actor. Walkable edge = `ArenaHalfExtentCm - 200` (~**2300 cm** when half-extent is 2500 / 50 m floor). **Z = arena origin Z** (floor).

| Index | Offset (cm) | Conceptual label |
|---|---|---|
| 0 | (0, +Edge) | Stairwell_N |
| 1 | (0, −Edge) | Stairwell_S |
| 2 | (+Edge, 0) | LoadingDock_E |
| 3 | (−Edge, 0) | ElevatorBank_W |
| 4 | (+Edge, +Edge) | ServiceCorridor_NE |
| 5 | (−Edge, +Edge) | ServiceCorridor_NW |
| 6 | (+Edge, −Edge) | ServiceCorridor_SE |
| 7 | (−Edge, −Edge) | ServiceCorridor_SW |

Count follows `GameConfig::AlienSpawnPointCount` (default 8). If `SpawnPointActors` are wired and count ≥ expected, those transforms win and defaults are skipped.

`GetFarthestSpawnFrom` unchanged — GameMode/bots pick the farthest point from the player on respawn.

## Bounds / ceiling usage

```
GameMode tick or Character:
  Arena->EnforceBoundsOnActor(Pawn);
// or
  Pawn->SetActorLocation(Arena->ClampToBounds(Pawn->GetActorLocation()));
```

- `BoundsVolume` half-extent XY synced to `ArenaSizeMeters * 50` cm.
- `CeilingClamp` sits at `AtriumTowerHeightCm + 200` so atrium top stays inside but roof escapes are blocked.

## Cover usage

```
Arena->RegisterCoverVolume(MyBox);           // optional Editor / runtime boxes
if (Arena->IsPointInCover(Loc)) { ... }
FVector CoverPt;
if (Arena->GetNearestCoverPoint(Loc, CoverPt)) { ... }
```

Prototype ships six default query boxes (cubicles N/E/S/W + resin NE/SW). Replace or add real art volumes for shipping.

## Collision helpers (bots / character)

| Call | Behavior |
|---|---|
| `PushApartNearbyAliens(Self)` | ECC_Pawn sphere `AlienPushApartRadiusCm`; push Self along XY by `0.5 * (Radius - Dist)` |
| `ApplyFallDamageIfNeeded(Char, FallM, ThresholdM)` | If FallM > Threshold: damage = excess × **15**; `TakeDamage` |
| `TraceGroundDistance(From, MaxCm, Hit)` | Visibility line down |
| `TraceCoverHeight(From, HeightCm, Hit)` | Visibility line up from +10 cm |

## Layout zones (DESIGN — comments in `OfficeArena.h`)

Atrium center (~14 m tower) · cubicles · server racks · cable tray @ 1.5 m · conference pad · drywall berm · resin clusters · perimeter glass/planters. Procedural cover only approximates cubicles/resin; place the rest in Editor.

## Editor still recommended for final art

1. Place `AOfficeArena` (or BP) once in the persistent level.
2. Prefer **8 Target Point / empty actors** at real stairwells / dock / elevator / corridors → `SpawnPointActors`.
3. Add real cover boxes / meshes; register extras via `RegisterCoverVolume` or `CoverVolumes`.
4. NavMesh + art per `LEVEL_SETUP_CHECKLIST.md`.
5. Assign shared `DA_GameConfig`.

Defaults exist so PIE / AI / GameMode can exercise Phase 2 before art lands.

## Not in this phase

- Full NavMesh MoveTo AI
- Cooked build / art / audio
- GameMode population wiring beyond existing stubs
