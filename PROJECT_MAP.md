# Night Shift — Floor 37: Project Map

Last updated: 2026-09-04. Keep this file current when a phase closes or a tree changes shape.

One spec, two implementations. `DESIGN.md` is the contract. `web/` is the playable reference. `ue5-scaffold/` is the real target.

Lineage: `DESIGN.md` is a rewrite of an earlier Three.js FPS prompt (desert oil-field arena, first-person) re-themed to the Floor 37 office and third-person camera. The same numbers (100 HP, 30/90, 600 RPM, 6 bots, 25 kills) carry through from that prompt. A copy of the original DESIGN.md text also sits outside the repo at `../night-shift-floor-37.md`; the repo copy is the maintained one.

```
night-shift-floor-37/
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
└── ue5-scaffold/             UE 5.8 game module. Compiles. No Editor content yet.
    ├── README.md             Drop-in steps, module table, verified compile status.
    ├── EDITOR_DROP_IN.md     Click-paths for every asset a human must create. THE next gate.
    ├── LEVEL_SETUP_CHECKLIST.md  How to build the office/atrium level.
    ├── INPUT_MAPPING.md      8 Input Actions + IMC_NightShift key table.
    ├── NAVMESH_NOTES.md      Optional AIController MoveTo path.
    ├── PHASE0.md … PHASE5.md History of what each authoring phase shipped.
    ├── NightShiftFloor37.uproject   EngineAssociation 5.8, EnhancedInput plugin.
    ├── Config/               DefaultEngine / DefaultGame / DefaultInput merge stubs.
    ├── Content/              Four README placeholders. Zero .uasset files.
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
| `AArenaGameMode` | `ArenaGameMode.h/.cpp` | Match state, timer, kills, win/lose, soft restart, alien pool, HUD creation, bounds enforcement |
| `AOfficeArena` | `OfficeArena.h/.cpp` | Bounds + ceiling clamp, 8 spawn points, 11 cover query boxes, spawn selection |
| `ANightShiftCharacter` | `NightShiftCharacter.h/.cpp` | Enhanced Input bindings, OTS camera + Q swap, health/regen, recoil, mantle, fall damage |
| `URifleComponent` | `RifleComponent.h/.cpp` | Fire / reload / ammo, soft-lock, visibility hitscan, FX pool calls |
| `AAlienBot` | `AlienBot.h/.cpp` | Chase / strafe / burst state machine, hit counting, flash, death + respawn |
| `UArenaCollision` | `ArenaCollision.h/.cpp` | Push-apart between bots, fall damage, extra traces |
| `UHUDWidget` | `HUDWidget.h/.cpp` | Polls match state into `OnRefreshHUD` / `OnPromptChanged` BP events; click-to-start |
| `AFXPoolManager`, `APooledTracerActor` | `FXPoolInterface.h/.cpp` | Pooled tracer actors and muzzle point lights |

Cross-references: GameMode pushes `UGameConfig` into everything at BeginPlay. Bots call back into GameMode on death. Character asks GameMode for pause state. Rifle finds `AFXPoolManager` by class lookup, so one must be placed in the level.

## Status board

| Area | State | Evidence |
|---|---|---|
| DESIGN.md | Stable | Both implementations match every specified number |
| web/ | Playable, bug-fixed | Loads clean; module-level checks pass; real playthrough after latest fixes still pending |
| UE5 compile | **Verified** on UE 5.8 Mac | Zero errors, zero warnings, `ue5-scaffold/README.md` records the command |
| UE5 runtime logic | Fixed, unverified in Editor | Hit channel, pause, spawn spread, bounds, dead-input gating all in source |
| UE5 Editor content | **Not started** | `EDITOR_DROP_IN.md` 58 items, `LEVEL_SETUP_CHECKLIST.md` 63 items, 0 checked |
| Play-In-Editor | **Blocked** on content | No level, no input assets, no HUD widget, no Data Asset |
| Art / audio / packaging | Out of scope so far | DESIGN calls for Nanite + Lumen mood pass; nothing exists |

## Build and run

Web:
```
cd web && python3 -m http.server 8000     # then open http://localhost:8000
```

UE5 headless build (copy the scaffold somewhere first so build output stays out of the repo):
```
"/Users/Shared/Epic Games/UE_5.8/Engine/Build/BatchFiles/Mac/Build.sh" NightShiftFloor37Editor Mac Development -Project="<copy>/NightShiftFloor37.uproject"
```
Build logs: `~/Library/Application Support/Epic/UnrealBuildTool/Log.txt`.

Do not merge from the copy under `~/Documents/Unreal Projects/NightShiftFloor37/`. It predates the compile fixes. Overwrite it from this repo.

---

## Next steps

Ordered by what unblocks the most. Phases 6 and 7 are the critical path to a playable Unreal build. 8 and 9 can run in parallel with them.

### Phase 6: Editor content and first PIE (critical path, manual, Editor only)

Goal: press Play and shoot an alien. Everything here is a human clicking in the Editor; the docs already spell out each step.

1. Open (or create) a UE 5.8 Third Person project and overwrite its `Source/`, `Config/`, `.uproject` with this repo's `ue5-scaffold/`. Compile from the Editor.
2. `DA_GameConfig` Data Asset under `Content/Data/` (`EDITOR_DROP_IN.md` §Data Asset). Defaults already match DESIGN, so no values to type.
3. Eight Input Actions plus `IMC_NightShift` under `Content/Input/` (`INPUT_MAPPING.md`). Look needs a Negate on Y if the muzzle dips instead of rises on recoil.
4. `BP_NightShiftCharacter` with the IMC, the 8 actions, and the Data Asset assigned.
5. `BP_ArenaGameMode` with `HUDWidgetClass`, `DefaultPawnClass`, and the Data Asset. Set it as the World Settings GameMode override.
6. `WBP_NightShiftHUD` parented to `UHUDWidget`. Implement `OnRefreshHUD` and `OnPromptChanged`, keep it hit-testable so clicks start the match.
7. Greybox level `Content/Maps/Floor37`: floor, perimeter, atrium tower, PlayerStart, one placed `AOfficeArena`, one placed `AFXPoolManager`, optionally 8 tagged spawn markers. Full dressing comes later; a flat floor with the arena actor is enough for the first PIE.
8. PIE smoke, in this order. Each one tests a fix from this session:
   - Click to play, HUD shows 30 / 90 and 0 kills.
   - Six aliens appear at six different edge points, not one corner.
   - Shooting an alien increments its hit counters and kills it at 3 body / 2 head.
   - Esc freezes aliens and the gun; Esc again resumes.
   - Walking past the arena edge clamps you back inside.
   - Dying shows the restart prompt and ignores movement input.
   - 25 kills shows the win screen with the time.

Exit criterion: all seven smoke items pass. Check them off in `EDITOR_DROP_IN.md` as you go, and record the result in `README.md` under Compile status.

### Phase 7: Feel and visibility (UE, code plus a little content)

Only after Phase 6, because every item needs PIE to judge.

- Give `APooledTracerActor` a visible component. A thin scaled cube stretched between `TracerStart` and `TracerEnd` is enough; Niagara ribbon later.
- Alien mesh plus a hit-flash material bound to `HitFlashAlpha`. Until then the bot is an invisible capsule.
- Cap the soft-lock overlap sphere in `RifleComponent.cpp` to about 30 m. Today it queries a 200 m radius on every shot at 600 RPM.
- Move `AddMappingContext` from `BeginPlay` to `PossessedBy` so re-possession keeps input.
- Decide whether recoil should accumulate. It currently self-cancels exactly. If DESIGN's "small kick that recovers" is meant literally, leave it; otherwise let a fraction persist.
- Fold the remaining hard-coded numbers into `UGameConfig`: fall damage per metre (two copies), hit-marker duration, mantle reach/height, muzzle light intensity. Delete the four unused tunables.
- Camera shake and damage vignette bound to `OnDamaged`.

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
