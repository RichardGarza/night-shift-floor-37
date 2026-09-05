# Phase 0 — Delivered

Scaffold root: this directory (`ue5-scaffold/`).

## What shipped

1. **UE-shaped layout**: `.uproject` template, `Source/NightShiftFloor37/` (Build.cs, Targets, module), `Public/` + `Private/` class stubs, `Config/`, `Content/` placeholders (Maps, Blueprints, UI, Data).
2. **Classes** with UCLASS/UPROPERTY/UFUNCTION + TODO/stub bodies:
   - `UGameConfig` — every DESIGN tunable with matching defaults
   - `AArenaGameMode` — match, timer, kills, win/lose, soft restart
   - `AOfficeArena` — bounds, spawn points, cover
   - `ANightShiftCharacter` — Phase 1 API (Move/Look/Sprint/Jump/SwapShoulder/Fire/Reload/TakeDamage/Regen)
   - `URifleComponent` — Phase 1 API (Fire/StopFire/Reload/GetAmmo/Trace/ApplyDamage)
   - `AAlienBot` — chase, strafe burst, flash, death/respawn
   - `UArenaCollision` — push-apart, fall damage, extra traces
   - `UHUDWidget` — crosshair/HP/ammo/prompts hooks
   - `FXPoolInterface` / `AFXPoolManager` — tracer & muzzle light pool stubs
3. **Docs**: `README.md`, `INPUT_MAPPING.md`, `LEVEL_SETUP_CHECKLIST.md`, this file.
4. **Phase 1**: Character + Rifle stubs are wire-ready for Enhanced Input (not empty TODOs).

## Drop-in (short)

1. Third Person + Enhanced Input UE5 project.
2. Merge `Source/` module (or classes) + Build.cs deps.
3. Create `DA_GameConfig` from `UGameConfig` (defaults already correct).
4. Build IMC/IA per `INPUT_MAPPING.md`; assign on character BP.
5. Build level per `LEVEL_SETUP_CHECKLIST.md`.
6. Compile in Editor.

## Not included

- Compilable binary / cooked build on this machine
- Art, audio, Niagara assets, marketplace packs
- Full NavMesh AI MoveTo implementation — *superseded in Phase 5* (`bPreferNavMeshMoveTo` + `TryNavMeshMoveToTarget`)
- Concrete pool actor implementations — *superseded in Phase 1* (`AFXPoolManager`, `APooledTracerActor`)
