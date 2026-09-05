# Phase 1 — Gunfeel package

Scaffold root: this directory (`ue5-scaffold/`).

## What changed

Hardened existing Phase 1 Character + Rifle paths (did not rewrite Enhanced Input wiring or GameConfig defaults).

| Area | Change |
|---|---|
| **Recoil** | `Look()` is mouse-axis only. `AddRecoilKick` accumulates `RecoilOffsetDegrees` **and** immediately `AddControllerPitch/YawInput`. Tick `UpdateRecoilRecovery` Interps residual to zero with `RecoilRecoverySpeed` and applies the recovered delta as reverse controller input (camera settles). `KickRecoil` still randomizes pitch/yaw within `GameConfig` maxes. |
| **Soft-lock** | `URifleComponent::Trace` sphere-overlaps pawns in `HitscanRangeMeters`, keeps closest alive `AAlienBot` inside `SoftLockConeHalfAngle`, biases aim dir toward capsule center or head (whichever is closer to current aim). Free-aim when none in cone. Reuses member `SoftLockOverlaps` (no per-shot heap growth). |
| **Fall damage** | Snapshot `LastGroundedZ` on leave-ground via `OnMovementModeChanged` + Tick edge detect. `Landed` still uses ArenaCollision path. Formula: **~15 HP per excess meter over 6 m** (`FallDamageHeightMeters`). |
| **Damage feedback** | BlueprintAssignable: `OnDamaged(amount)`, `OnDied`, `OnHealthChanged(health, max)` — broadcast from `TakeDamage` / regen / `SoftResetPlayerState` / heal. |
| **FX pools** | Concrete `AFXPoolManager` + `APooledTracerActor`: preallocates `TracerPoolSize` / `MuzzleLightPoolSize` in `BeginPlay`/`WarmPools`. Ring Acquire/Release; no `SpawnActor`/`NewObject` on fire hot path after warmup. Rifle resolves pool via soft ref or `GetActorOfClass`. |
| **Alien head** | `IsLocationOnHead` / `IsHeadBone` match DESIGN (top 25% capsule); bone name matching hardened. |

## Gunfeel → DESIGN map

| DESIGN | Code / GameConfig |
|---|---|
| Soft lock in reticle cone | `SoftLockConeHalfAngle` (default 3°) + Trace bias |
| Small random recoil that recovers | `RecoilPitchMaxDegrees` / `RecoilYawMaxDegrees` / `RecoilRecoverySpeed` |
| 600 RPM hitscan 25/50 | `RoundsPerMinute`, `BodyDamage`, `HeadDamage` |
| 30/90, 1.5 s reload | `MagSize`, `ReserveAmmo`, `ReloadSeconds` |
| Fall damage >6 m | `FallDamageHeightMeters` + 15 HP/m excess |
| Pool tracers 60 ms / muzzle lights | `TracerDurationMs`, `MuzzleFlashDurationMs`, pool sizes |
| Head = top 25% capsule | `AlienHeadFraction` + bot helpers |

## Remaining Editor-only steps

1. Create **IMC / IA** assets per `INPUT_MAPPING.md`; assign on character BP (`DefaultMappingContext`, Move/Look/Jump/Sprint/Fire/Reload/ShoulderSwap/Pause).
2. Create **DA_GameConfig** from `UGameConfig` (CDO defaults already match DESIGN); assign on Character, Rifle, GameMode, Aliens, FXPoolManager.
3. Place **`AFXPoolManager`** in the persistent level (or spawn from GameMode BeginPlay) and optionally assign `GameConfig`.
4. Character BP from Third Person template: set parent to `ANightShiftCharacter` (or reparent), assign camera/mesh as needed.
5. Optional: Niagara tracers reading `APooledTracerActor::TracerStart/End`; hit-marker / vignette / camera shake bound to `OnHitConfirmed` / `OnDamaged`.
6. Compile in Editor (this machine has no UE toolchain).

## Not in this phase

- Full NavMesh MoveTo AI (steering stub remains; shipped later in Phase 5)
- Cooked / packaged build
- Art / audio / marketplace packs

