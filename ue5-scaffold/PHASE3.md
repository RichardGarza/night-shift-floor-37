# Phase 3 — Aliens package

Scaffold root: this directory (`ue5-scaffold/`).

Hardens `AAlienBot` combat AI + `AArenaGameMode` live-alien pool from Phase 0–2 stubs. Matches **DESIGN.md** aliens section. Does **not** edit `AOfficeArena` cover/spawns/checklist ownership — bots/GameMode only call `GetFarthestSpawnFrom` / existing spawn APIs. Phase 0–2 public APIs preserved; new helpers are additive.

## Design contract

| Spec | Value |
|---|---|
| Live aliens | **6** (`UGameConfig::MaxLiveAliens`) |
| Move | **4 m/s** toward player + simple obstacle steering |
| Combat range | **≤12 m** with LOS → stop, strafe L/R |
| Burst | **3** hitscan rounds, **~0.09 s** intra-shot delay, then **1.5 s** to next burst |
| Accuracy / damage | **30%** / **10** HP per hit |
| Kill | **3** body **or** **2** head (head = top **25%** of capsule) |
| Hit flash | White **80 ms** wall time (`HitFlashTimeRemaining` in **seconds**) |
| Death | No ragdoll — hide / collapse / respawn |
| Respawn | After **3 s** at farthest of **8** edge spawns from player |
| Push-apart | `UArenaCollision::PushApartNearbyAliens` (kept) |

## Alien AI (`AAlienBot`)

### States

`Idle` → no target · `Chase` → outside range or no LOS · `StrafeBurst` → ≤12 m + LOS · `Dead` → hidden, collision off

### Burst timing (fixed)

Do **not** dump all 3 shots in one `Tick`.

1. When `BurstCooldownRemaining ≤ 0` and no shots queued → start burst (`BurstShotsRemaining = AlienBurstRoundCount`), flip strafe sign.
2. Each shot: `TryBurstShot()` then wait `AlienBurstIntraShotDelaySeconds` (**0.09** default) before the next.
3. After the last shot → set `BurstCooldownRemaining = AlienBurstIntervalSeconds` (**1.5**).

Leaving combat range cancels an in-progress burst; cooldown still paces the next volley.

### Chase steering

Forward visibility line trace (`SteerHitScratch`, ~180 cm). If blocked (and not the player), add lateral offset with alternating `SteerSideSign` so bots can round cover without NavMesh.

```
// TODO (Editor follow-up): NavMesh MoveTo via AIController for stairs/ramps when
// simple steering is not enough. Waypoint graph is fine if MoveTo cannot climb.
```

### Hit flash

`PlayHitFlash` stores remaining as **seconds** (`HitFlashDurationMs * 0.001`). `Tick` subtracts clamped `DeltaSeconds` → expires in ~**80 ms** wall time. `bIsFlashing` + BlueprintAssignable `OnHitFlash(bool)` for material hooks.

### Respawn loop

```
Die → RegisterKill(GameMode) → ScheduleRespawn(3s, bRespawnScheduled)
     → PerformRespawn
         → GameMode::RespawnAlien(this)   // preferred: farthest spawn + ActivateAtSpawn
         OR AOfficeArena::GetFarthestSpawnFrom(player) + ActivateAtSpawn
```

`ActivateAtSpawn`: reset hit counts, unhide, collision on, clear flash/burst, state **Chase**.

### SoftReset

Clears respawn timer / `bRespawnScheduled`, combat counters, burst timers, flash; then `SoftDespawn`. Used by GameMode soft restart before redistribution.

## Spawn pool (`AArenaGameMode`)

| Piece | Behavior |
|---|---|
| `AlienPool` | `TArray` of up to `MaxLiveAliens` (6) |
| `BuildAlienPool` | Adopt level-placed bots, then `SpawnActor` fill; all start SoftDespawn'd |
| `CachedArena` | `EditAnywhere` + auto-find `AOfficeArena` on BeginPlay |
| `AlienBotClass` | Spawn class (default `AAlienBot`) |
| `StartMatch` | Refresh spawns, `EnsureAlienPopulation` |
| `RegisterKill` | ++kill count / win check; bot self-respawns |
| `RespawnAlien` | Farthest spawn from player → `ActivateAtSpawn` |
| `EnsureAlienPopulation` | Activate non-alive pool bots **without** pending respawn until 6 live |
| `SoftRestart` | Player `SoftResetPlayerState` + `SoftRestartAlienPool` (SoftReset all → redistribute) |

`IsRespawnPending()` prevents GameMode from double-activating a bot waiting on its 3 s death timer.

## Tunables (`UGameConfig`)

Existing aliens category plus:

- `AlienBurstIntraShotDelaySeconds` = **0.09**

## Remaining NavMesh / Editor work

1. Place `AOfficeArena` (or BP) once; prefer 8 real spawn markers → `SpawnPointActors` (procedural edge defaults still work).
2. Assign `UGameConfig` data asset on GameMode / bots.
3. Build NavMesh on the office floor + atrium stairs/ramps; wire `AAIController` **MoveTo** in `ChasePlayer` (commented TODO).
4. Optional: waypoint graph if MoveTo fails on tower climbs.
5. Material hook for `OnHitFlash` / white emissive overlay (TODO in `PlayHitFlash`).
6. Optional alien muzzle tracer FX.

## Files touched

| File | Change |
|---|---|
| `Public/AlienBot.h` / `Private/AlienBot.cpp` | Burst pacing, flash seconds + events, steer, respawn, SoftReset |
| `Public/ArenaGameMode.h` / `Private/ArenaGameMode.cpp` | Alien pool, RespawnAlien, SoftRestart pool, CachedArena wire |
| `Public/GameConfig.h` | `AlienBurstIntraShotDelaySeconds` |
| `PHASE3.md` | This doc |

**Not edited:** `OfficeArena.*` (NumberFour ownership), Three.js, git.

## Smoke checklist

- [ ] StartMatch → exactly 6 live aliens at edge spawns
- [ ] Chase steers around a visibility blocker (no NavMesh yet)
- [ ] At ≤12 m + LOS: strafe; 3 spaced shots; ~1.5 s gap between bursts
- [ ] Kill (3 body / 2 head) → hide → 3 s → farthest spawn; kill count ++
- [ ] SoftRestart → timers cleared, 6 redistributed, kills/timer reset
