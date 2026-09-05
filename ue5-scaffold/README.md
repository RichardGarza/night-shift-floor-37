# Night Shift — Floor 37 (UE5 Scaffold)

This folder is a **drop-in C++/content scaffold** for Unreal Engine **5.8**. `NightShiftFloor37.uproject` + `Source/` build as a standalone game module (no Editor content yet); you can also merge `Source/` into an existing **Third Person + Enhanced Input** project.

Authoritative design numbers: `../DESIGN.md` (and mirrored in `UGameConfig` defaults).

## Compile status

Verified 2026-09-04: `NightShiftFloor37Editor` (Mac, Development) builds and links against **UE 5.8** with zero errors and zero warnings via

```
"/Users/Shared/Epic Games/UE_5.8/Engine/Build/BatchFiles/Mac/Build.sh" NightShiftFloor37Editor Mac Development -Project="<path>/NightShiftFloor37.uproject"
```

Not yet verified: Play-In-Editor. That needs the Editor content in `EDITOR_DROP_IN.md` (Data Asset, Input Actions, HUD widget, Blueprints, a level).

## Drop-in steps

1. Create or open a UE 5.8 project from the **Third Person** template with **Enhanced Input** enabled.
2. Copy `Source/NightShiftFloor37/` into your project's `Source/` (or merge classes into your primary game module and update `Build.cs` deps: `EnhancedInput`, `UMG`, `AIModule`, `NavigationSystem`).
3. Either:
   - Use `NightShiftFloor37.uproject` as a reference for module/plugin entries, **or**
   - Add the `NightShiftFloor37` module (or class files) to your existing `.uproject` / `Target.cs` / `Build.cs`.
4. Copy `Config/` notes into your project; create Enhanced Input assets listed in `INPUT_MAPPING.md`.
5. Create a Data Asset of class `UGameConfig` under `Content/Data/` — defaults already match DESIGN; assign it on GameMode, Character, Rifle, Aliens, Arena.
6. Follow `LEVEL_SETUP_CHECKLIST.md` for the one persistent office/atrium level.
7. Build from the Editor (Generate Visual Studio / Rider project files first on Windows/Mac).

> **Missing Editor Content blockers → see [`EDITOR_DROP_IN.md`](EDITOR_DROP_IN.md)**  
> Exact click-paths for `DA_GameConfig`, Enhanced Input IMC/IA, `WBP_NightShiftHUD` → `HUDWidgetClass`, `AFXPoolManager`, and NavMesh (Audit/Boss gates). Scaffold ships no `.uasset` binaries.

## Modules

| Class | Role |
|---|---|
| `UGameConfig` | All tunables |
| `AArenaGameMode` | Match, timer, kills, win/lose, soft restart |
| `AOfficeArena` | Bounds, 8 spawn points, cover volumes |
| `ANightShiftCharacter` | Move, OTS camera + Q swap, health, recoil |
| `URifleComponent` | Fire, reload, ammo, hitscan |
| `AAlienBot` | Chase, strafe, burst, flash, death/respawn |
| `UArenaCollision` | Extra traces, push-apart, fall damage |
| `UHUDWidget` | Crosshair, HP, ammo, prompts |
| `AFXPoolManager` / pool interfaces | Tracers + muzzle lights |

## Constraints (from DESIGN)

- Plain actors/components — no deep class trees
- No Chaos vehicles, no audio requirement, no marketplace packs
- Object pools for tracers / muzzle lights
- Target 60 fps mid/integrated — no per-frame allocs in hot paths
- Soft restart without unloading the level

See `PHASE0.md` for deliverable summary.
