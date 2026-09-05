# Night Shift — Floor 37: Project Map

Last updated: 2026-09-04. Keep this file current when a phase closes or a tree changes shape.

One spec, two implementations. `DESIGN.md` is the contract. `web/` is the playable reference. `ue5-scaffold/` is the real target.

Lineage: `DESIGN.md` is a rewrite of an earlier Three.js FPS prompt (desert oil-field arena, first-person) re-themed to the Floor 37 office and third-person camera. The same numbers (100 HP, 30/90, 600 RPM, 6 bots, 25 kills) carry through from that prompt. A copy of the original DESIGN.md text also sits outside the repo at `../night-shift-floor-37.md`; the repo copy is the maintained one.

```
night-shift-floor-37/
├── README.md                 GitHub front page: what it is, build, run, controls.
├── DESIGN.md                 The spec. Every number in both implementations traces here.
├── PROJECT_MAP.md            This file: layout, status, next steps.
├── .gitignore                Unreal build output, IDE files, .DS_Store.
│
├── web/                      Three.js prototype — playable now, browser only.
│   ├── README.md             How to serve it.
│   ├── index.html            Import map (three r0.170 via esm.sh CDN), DOM HUD.
│   ├── css/style.css
│   └── js/
│       ├── config.js         Every tunable. Mirrors UGameConfig.
│       ├── main.js           Entry: new Game(canvas).
│       ├── game.js           Renderer, input, state machine, main loop, shot-collider list.
│       ├── arena.js          Static geometry, lights, collision solids, 8 spawn points.
│       ├── player.js         Movement, camera, recoil, health, soft-lock.
│       ├── rifle.js          Ammo, fire rate, reload, hitscan + muzzle re-trace.
│       ├── combat.js         Raycast, pooled tracers / muzzle lights, hit marker, vignette.
│       ├── alien.js          Bot AI, steering, LOS, burst fire; AlienManager pool + spawns.
│       ├── collision.js      AABB / OBB helpers, rampHeightAt.
│       └── hud.js            DOM overlay.
│
└── ue5-scaffold/             UE 5.8 project. Builds and runs standalone; everything is code-built.
    ├── README.md             Drop-in steps, module table, verified compile status.
    ├── EDITOR_DROP_IN.md     Optional Editor assets that override the code-built defaults.
    ├── LEVEL_SETUP_CHECKLIST.md  How to build the office/atrium level.
    ├── INPUT_MAPPING.md      8 Input Actions + IMC_NightShift key table.
    ├── NAVMESH_NOTES.md      Optional AIController MoveTo path.
    ├── PHASE0.md … PHASE5.md History of what each authoring phase shipped.
    ├── NightShiftFloor37.uproject   EngineAssociation 5.8, EnhancedInput plugin.
    ├── Config/               DefaultEngine / DefaultGame / DefaultInput merge stubs.
    ├── Content/Maps/Floor37.umap  Generated map: OfficeArena + FXPoolManager + PlayerStart.
    ├── Scripts/make_floor37_map.py  Headless map generator (Python commandlet).
    └── Source/
        ├── NightShiftFloor37.Target.cs, NightShiftFloor37Editor.Target.cs
        └── NightShiftFloor37/
            ├── NightShiftFloor37.Build.cs
            ├── Public/*.h    9 classes (below) + module header
            └── Private/*.cpp
```

## UE5 module: who owns what

| Class | File | Owns |
|---|---|---|
| `UGameConfig` | `GameConfig.h/.cpp` | Every tunable as a Data Asset; `ResolveOrCreate` falls back to DESIGN defaults |
| `AArenaGameMode` | `ArenaGameMode.h/.cpp` | Match state, timer, kills, win/lose, soft restart, alien pool, HUD creation, bounds enforcement; spawns arena / FX pool / PlayerStart if the map lacks them; default pawn + HUD classes |
| `AOfficeArena` | `OfficeArena.h/.cpp` | Bounds + ceiling clamp, 8 spawn points, 11 cover boxes, spawn selection, **greybox geometry and lighting** (floor, walls, atrium tower with spiral ramps, cover blocks, sun, sky, fog, practicals) |
| `ANightShiftCharacter` | `NightShiftCharacter.h/.cpp` | Enhanced Input bindings with **runtime-built actions and mapping context**, OTS camera + Q swap, health/regen, recoil, mantle, fall damage, greybox body |
| `URifleComponent` | `RifleComponent.h/.cpp` | Fire / reload / ammo, soft-lock, visibility hitscan, FX pool calls |
| `AAlienBot` | `AlienBot.h/.cpp` | Chase / strafe / burst state machine, hit counting, flash, death + respawn |
| `UArenaCollision` | `ArenaCollision.h/.cpp` | Push-apart between bots, fall damage, extra traces |
| `UHUDWidget` | `HUDWidget.h/.cpp` | **Builds its own UMG tree in C++** (HP bar, ammo, kills, timer, crosshair, prompts, damage vignette, Esc menu with sensitivity slider / resume / quit); click-to-start; BP events still fire for custom art |
| `AFXPoolManager`, `APooledTracerActor` | `FXPoolInterface.h/.cpp` | Pooled tracer actors (visible) and muzzle point lights |
| `UDamageCameraShake` | `DamageCameraShake.h/.cpp` | Short perlin shake on player damage |

Cross-references: GameMode pushes `UGameConfig` into everything at BeginPlay. Bots call back into GameMode on death. Character asks GameMode for pause state. Rifle finds `AFXPoolManager` by class lookup, so one must be placed in the level.

## Status board

| Area | State | Evidence |
|---|---|---|
| DESIGN.md | Stable | Both implementations match every specified number |
| web/ | Playable, bug-fixed | Loads clean; module-level checks pass; real playthrough after latest fixes still pending |
| UE5 compile | **Verified** on UE 5.8 Mac | Zero errors, zero warnings, `ue5-scaffold/README.md` records the command |
| UE5 standalone run | **Verified** 2026-09-04 | Launches to the start prompt with greybox arena, HUD, lighting; startup log clean |
| UE5 gameplay | Code-complete, **not yet played through** | Shoot / pause / spawn-spread / bounds / death / win need a human at the keyboard |
| UE5 Editor content | Optional now | Code-built defaults cover input, HUD, config, arena, map; `EDITOR_DROP_IN.md` assets override them when assigned |
| Art / audio / packaging | Out of scope so far | DESIGN calls for Nanite + Lumen mood pass; nothing exists |

## Build and run

Web:
```
cd web && python3 -m http.server 8000     # then open http://localhost:8000
```

UE5 build in place (`.gitignore` covers Binaries / Intermediate / Saved):
```
cd ue5-scaffold
"/Users/Shared/Epic Games/UE_5.8/Engine/Build/BatchFiles/Mac/Build.sh" NightShiftFloor37Editor Mac Development -Project="$PWD/NightShiftFloor37.uproject"
```

UE5 run standalone (or open the `.uproject` in the Editor and press Play):
```
"/Users/Shared/Epic Games/UE_5.8/Engine/Binaries/Mac/UnrealEditor" "$PWD/NightShiftFloor37.uproject" /Game/Maps/Floor37 -game -windowed -ResX=1600 -ResY=900
```

Regenerate the map if it is ever lost:
```
"/Users/Shared/Epic Games/UE_5.8/Engine/Binaries/Mac/UnrealEditor-Cmd" "$PWD/NightShiftFloor37.uproject" -run=pythonscript -script=Scripts/make_floor37_map.py -unattended -nop4 -nosplash
```
Build logs: `~/Library/Application Support/Epic/UnrealBuildTool/Log.txt`.

Do not merge from the copy under `~/Documents/Unreal Projects/NightShiftFloor37/`. It predates the compile fixes. Overwrite it from this repo.

---

## Next steps

Ordered by what unblocks the most. Phases 6 and 7 are the critical path to a playable Unreal build. 8 and 9 can run in parallel with them.

### Phase 6: First playthrough (done in code; needs a human to play it)

The Editor checklist is no longer the gate. The C++ builds input, HUD, config, arena, lighting, FX pool, and player start at runtime, and `Content/Maps/Floor37.umap` is generated. The standalone game launches to the start prompt. What remains is a person at the keyboard running this smoke list:

- Click to play: HUD shows 30 / 90, kills 0 / 25, timer running.
- Six green aliens come from six different edges, not one corner.
- Shots land: alien flashes white on hit, dies at 3 body / 2 head, kill counter climbs.
- Tracers draw from the muzzle; muzzle light flashes.
- Esc pauses (prompt says so, aliens freeze, gun is dead); Esc again resumes.
- Ramps are climbable; the tower top is reachable; dropping from the top hurts.
- Walking into the perimeter stops you.
- Dying shows the restart prompt and ignores movement; clicking restarts in place.
- 25 kills shows the win screen with the time.

Record what fails here or in `ue5-scaffold/README.md`. Anything broken goes to the top of Phase 7.

### Phase 7: Feel and visibility (UE, code plus a little content)

Only after Phase 6, because every item needs PIE to judge.

Done 2026-09-04: mouse sensitivity setting (default 0.35°/count, Esc-menu slider, saved to GameUserSettings.ini), Esc menu with Resume / Quit, camera boom 300 cm with 70 cm shoulder, soft-lock query capped at `SoftLockRangeMeters` (30 m), fall damage and hit-marker duration moved into `UGameConfig` and four dead tunables removed, red damage vignette on the HUD, perlin camera shake on damage, mapping context added on possession.

Still open:

- Tune the greybox lighting in `AOfficeArena::BuildGreyboxLighting` (sun angle, fog, practicals). Auto exposure is off, so intensities are literal.
- Replace greybox cylinders with real meshes and swap the hit flash from a colour lerp to an emissive material.
- Decide whether recoil should accumulate. It currently self-cancels exactly. If DESIGN's "small kick that recovers" is meant literally, leave it; otherwise let a fraction persist.
- Mantle reach/height and muzzle light intensity are still literals on their classes.

### Phase 8: Level and mood (Editor content, parallel to 7)

`LEVEL_SETUP_CHECKLIST.md` is the spec. Suggested order: atrium tower with ramps, cubicle maze, six server racks, four resin clusters, perimeter glass and planters, then the lighting pass (sick green / amber practicals, wet floor, volumetric fog, Lumen). Build the NavMesh last and flip `bPreferNavMeshMoveTo` on to compare against raw steering.

### Phase 9: Web prototype upkeep (parallel, optional)

The web build is the fast place to test feel changes before porting them. Remaining items from the analysis:

- Real playthrough to confirm the muzzle re-trace and platform blocking feel right.
- Camera collision so the third-person camera does not clip through walls.
- Spatial partitioning for the ~126 solids if alien count or map size grows.
- Ammo economy: 120 rounds total with no pickups can make 25 kills unreachable. Either a small reserve refill per kill or ammo pickups at spawn points. Whichever you choose, mirror it in DESIGN and `UGameConfig`.
- Keyboard path to start (currently click only) and a `visibilitychange` auto-pause.

### Phase 10: Package (last)

Cook a Mac Development build, confirm 60 fps on integrated graphics per DESIGN, and tag a release. `Binaries/` is already excluded by `.gitignore`.

## Open decisions

- **Recoil model**: self-cancelling vs accumulating. Affects Phase 7 and the web build equally.
- **Ammo economy**: refill vs pickups vs larger reserve. DESIGN is silent.
- **Alien body**: capsule mesh vs skeletal. Skeletal enables `IsHeadBone`; capsule uses the top-25% height test that already works.
- **Where the canonical Unreal project lives**: keep building from a scratch copy, or turn `ue5-scaffold/` itself into the full project once content exists. The second option means committing `.uasset` binaries, so consider Git LFS first.
