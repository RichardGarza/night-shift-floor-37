# Night Shift — Floor 37 (UE5 Scaffold)

This folder is a **runnable Unreal Engine 5.8 project**. Everything needed to play is created from C++ at startup (input, HUD, arena greybox, lighting, FX pool, player start, config defaults), plus one generated map. Editor content in `EDITOR_DROP_IN.md` is optional polish that overrides the code-built defaults when assigned.

Authoritative design numbers: `../DESIGN.md` (and mirrored in `UGameConfig` defaults).

Repo-wide layout, status board, and roadmap: `../PROJECT_MAP.md`.

## Compile status

Verified 2026-09-04: `NightShiftFloor37Editor` (Mac, Development) builds and links against **UE 5.8** with zero errors and zero warnings via

```
"/Users/Shared/Epic Games/UE_5.8/Engine/Build/BatchFiles/Mac/Build.sh" NightShiftFloor37Editor Mac Development -Project="<path>/NightShiftFloor37.uproject"
```

## Run it

```
cd ue5-scaffold
"/Users/Shared/Epic Games/UE_5.8/Engine/Binaries/Mac/UnrealEditor" "$PWD/NightShiftFloor37.uproject" /Game/Maps/Floor37 -game -windowed -ResX=1600 -ResY=900
```

Or open `NightShiftFloor37.uproject` in the Editor and press Play (`EditorStartupMap` is Floor37).

If `Content/Maps/Floor37.umap` is missing, regenerate it headlessly:

```
"/Users/Shared/Epic Games/UE_5.8/Engine/Binaries/Mac/UnrealEditor-Cmd" "$PWD/NightShiftFloor37.uproject" -run=pythonscript -script=Scripts/make_floor37_map.py -unattended -nop4 -nosplash
```

## Drop-in steps (merging into an existing project instead)

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
