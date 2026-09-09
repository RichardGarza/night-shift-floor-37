# Night Shift — Floor 37: Project Map

Last updated: 2026-09-08 (Sprints V, W, X). Keep this file current when a phase closes or a tree changes shape.

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
    ├── Content/Characters/Mannequins/  UE template Manny/Quinn + rifle/death/unarmed anims (Sprint W, committed).
    ├── Content/Weapons/Rifle/     UE template SM_Rifle / SKM_Rifle (Sprint W, committed).
    ├── Content/Imported/Aliens/Skel/  Quaternius Alien as SK_Alien + 14 anim takes + atlas (Sprint X, committed).
    ├── Scripts/make_floor37_map.py  Headless map generator (Python commandlet).
    ├── Scripts/import_alien_skeletal.py  Headless FBX → skeletal mesh + anims import (Sprint X).
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
| `ANightShiftCharacter` | `NightShiftCharacter.h/.cpp` | Enhanced Input bindings with **runtime-built actions and mapping context**, OTS camera + Q swap, health/regen, recoil, mantle, fall damage; **skeletal mannequin body + rifle prop + code-driven rifle anims** (Sprint W), exposure bias on the camera |
| `URifleComponent` | `RifleComponent.h/.cpp` | Fire / reload / ammo, soft-lock, **complex-collision** visibility hitscan from the camera, tracer from the rifle muzzle, FX pool calls |
| `AAlienBot` | `AlienBot.h/.cpp` | Chase / strafe / burst state machine with tracers and muzzle light, hit counting, death + respawn; **skeletal body fitted to the capsule, Idle/Run/Walk/Attack/Death clips, white material-swap hit flash** (Sprint X) |
| `UArenaCollision` | `ArenaCollision.h/.cpp` | Push-apart between bots, fall damage, extra traces |
| `UHUDWidget` | `HUDWidget.h/.cpp` | **Builds its own UMG tree in C++** (HP bar, ammo, kills, timer, crosshair, prompts, damage vignette, Esc menu with sensitivity slider / resume / quit); click-to-start; BP events still fire for custom art |
| `AFXPoolManager`, `APooledTracerActor` | `FXPoolInterface.h/.cpp` | Pooled tracer actors (visible) and muzzle point lights |
| `UDamageCameraShake` | `DamageCameraShake.h/.cpp` | Short perlin shake on player damage |
| `ANightShiftSelfTest` | `NightShiftSelfTest.h/.cpp` | Scripted match-loop test, spawned when launched with `-NightShiftSelfTest` |

Cross-references: GameMode pushes `UGameConfig` into everything at BeginPlay. Bots call back into GameMode on death. Character asks GameMode for pause state. Rifle finds `AFXPoolManager` by class lookup, so one must be placed in the level.

## Status board

| Area | State | Evidence |
|---|---|---|
| DESIGN.md | Stable | Both implementations match every specified number |
| web/ | Playable, bug-fixed | Loads clean; module-level checks pass; real playthrough after latest fixes still pending |
| UE5 compile | **Verified** on UE 5.8 Mac | Soft-ref PIE + Sprint O mesh cache |
| UE5 standalone / PIE | **Visual unlock confirmed** | SoftwareStarter: green Quaternius `SM_Alien` ×6 (not greybox capsules), cubicle stamps, 4 fluorescents |
| UE5 gameplay loop | **Verified by self-test** | Start → hit → kill → respawn → grace+fire-lock wait → pause → bounds → death → restart → win (26/26, `Saved/selftest-sprintv.log`) |
| Phase 6 softer start | **Shipped** | Spawn grace 4s + `MinStartSeparation` 18m (superseded by Sprint V) |
| Phase 7 lighting + knobs | **Shipped** | Greybox lighting; mantle + muzzle intensity on `UGameConfig` |
| Phase 8 mesh wire | **Shipped** | Soft paths + bio tint/flash; cubicle-only cover; fluorescents; desk/chair dress (N) |
| Sprint M demo facing | **Shipped** | `OrientPlayerTowardStartFocus` — yaw to nearest alien/atrium after safer-start |
| Sprint N office dress | **Shipped** | Omie `SM_Desk`/`SM_Chair` near cubicles only (`ApplyConfiguredOfficeDressMeshes`) |
| Sprint O mesh cache | **Shipped** | `ResolvePhase8LoadedMeshes` — one LoadSynchronous batch; Cached* reuse |
| Sprint Q server racks | **Shipped** | `ServerRackPropMesh` on Rack* volumes; Kenney CC0 — `/Game/Imported/Props/Office/SM_ServerRack` **imported** |
| Sprint R ceiling fluorescents | **Shipped** | Mount Z = CeilingClamp underside − 35cm (`3ac5f1a`) |
| Sprint V softer start + readability | **Shipped** | Grace 7s, `MinStartSeparation` 24m, `PostGraceAlienFireDelaySeconds` 1.5s (chase OK, no fire); brighter sun/sky, thinner fog, stronger practicals. Self-test waits out grace. `Saved/sprintv_lighting_start.png` |
| Sprint W player body + rifle | **Shipped** | UE template `SKM_Manny_Simple` on `GetMesh()`, `SM_Rifle` on `HandGrip_R`, single-node clips (idle ADS, walk/jog ×4 dirs, jump/fall/land, reload, death) driven from C++; cylinder stays as soft-miss fallback |
| Sprint X animated aliens | **Shipped** | `SK_Alien` (Quaternius, 0.55 scale ≈ 1.9 m) with Idle/Run/Walk/Punch/Death takes; capsule refit to mesh bounds so shots hit what you see; death clip plays before hide |
| Sprint X shooting fix | **Shipped** | Rifle trace uses complex collision + player/alien meshes block Visibility; tracer starts at the rifle muzzle. Root cause of "shots do nothing": hits only registered on the capsule, which no longer matched the visible body |
| Sprint X lighting lift | **Shipped** | Sun 5.0 / sky 1.6 / fog 0.009, practicals 600 cd, floor/concrete albedo ×2.5, `ExposureBiasEV` 0.6 on the camera. `docs/sprintx_mannequin_aliens.png` |
| Sprint Y enemies + sprint-fire + ramp | **Shipped** | Aliens face the player and plant to shoot, committed obstacle steering; firing drops sprint to walk; kill/time difficulty ramp 2→6 aliens, 10→35 % accuracy, 3.0→1.2 s bursts with HUD `Threat N / 5`. `ue5-scaffold/SPRINT_Y_ENEMIES_RAMP.md` |
| Sprint Z waves + variants | **Shipped** | 8 numbered waves with kill quotas, breather + restock between waves, Brute (wave 3) / Stalker (wave 5) variants, win on clearing wave 8 (~60 kills). `-NightShiftStartWave=N` for smoke runs. `ue5-scaffold/SPRINT_Z_WAVES.md` |
| Sprint AA Mutant model | **Shipped** | Mixamo Mutant (`/Game/Imported/Aliens/Mutant/SK_Mutant` + 6 clips, root-locked) auto-selected by `bAutoPickAlienModelSet`; Quaternius alien is the fallback. `ue5-scaffold/SPRINT_AA_MUTANT.md`, `docs/sprintaa_mutant_wave4.png` |
| Sprint AB variant skins | **Shipped** | `M_AlienVariant` (Tint/Emissive params) on SK_Mutant, Brute red / Stalker cyan skins, hit-react stagger, physics asset → per-bone headshots. `ue5-scaffold/SPRINT_AB_VARIANT_SKINS.md` |
| Sprint AC arena surfaces | **Shipped** | Poly Haven CC0 textures on a world-projected master material: wet tile floor, peeling painted 2 m walls + dirty-glass band + neon trim, concrete tower, corrugated ramps. Fixed invisible cover blocks and a stale serialized greybox list. `ue5-scaffold/SPRINT_AC_SURFACES.md`, `docs/sprintac_textured_arena_wave5.png` |
| Sprint AD dressing | **Shipped** | Darker glass band, under-plate atrium lights, `M_Resin` on egg cover, 8 perimeter desk pods; office props committed. `ue5-scaffold/SPRINT_AD_DRESSING.md` |
| Self-test | 47 / 47 | Adds wave-clear breather, restock, wave 2 growth, wave reset, final-wave win + variant mix |
| `Content/Imported` | **Partly committed** | `Aliens/Skel/` (SK_Alien + anims) is committed; office props / fluorescents / `SM_Alien` remain local-only on the Desktop copy — optional Git LFS |
| UE5 feel | Needs human | Recoil accumulate vs self-cancel still open |
| Silhouette | **Confirmed** | `docs/sprintx_mannequin_aliens.png` (mannequin + rifle + three aliens in frame) |
| Art / audio / packaging | Partial art | Player + aliens are real rigged meshes; arena is still greybox + props. Nanite+Lumen mood + audio still open |

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
Flags: `-NightShiftAutoStart` skips Click-to-play after 1.5 s (screenshots); `-NightShiftSelfTest` runs the scripted 31-check match loop (`-LogCmds="LogNightShift Verbose"` adds per-hit damage lines).

Re-import the alien if the skeletal assets are ever lost (FBX from https://quaternius.com/packs/ultimatemonsters.html, CC0):
```
NS_ALIEN_FBX=/path/to/Alien.fbx "/Users/Shared/Epic Games/UE_5.8/Engine/Binaries/Mac/UnrealEditor-Cmd" "$PWD/NightShiftFloor37.uproject" -run=pythonscript -script="$PWD/Scripts/import_alien_skeletal.py" -unattended -nop4 -nosplash
```

Regenerate the map if it is ever lost:
```
"/Users/Shared/Epic Games/UE_5.8/Engine/Binaries/Mac/UnrealEditor-Cmd" "$PWD/NightShiftFloor37.uproject" -run=pythonscript -script=Scripts/make_floor37_map.py -unattended -nop4 -nosplash
```
Build logs: `~/Library/Application Support/Epic/UnrealBuildTool/Log.txt`.

Do not merge from the copy under `~/Documents/Unreal Projects/NightShiftFloor37/`. It predates the compile fixes. Overwrite it from this repo.

---

## Next steps

### After Sprint AC (2026-09-09)

Arena is textured and lit with neon trim; Mutant variants tinted (Brute red, Stalker cyan). Richard to judge in play. Candidates next: darker glass band, tower fill light, resin cover material, more office dressing, ceiling decision, AnimBlueprint blends, recoil.

### After Sprint AA (2026-09-09)

Mutant is in. Richard to judge: silhouette/scale (`MutantMeshScale` 1.15), facing (`MutantMeshYawDegrees`), and whether the Mixamo material reads in the arena light. Next: per-variant materials, hit-react clip, physics asset, AnimBlueprint blends, recoil model.

### After Sprint Z (2026-09-08 night)

Richard on Y: wants a real progression, not a ramp; alien model is "super lame". Z ships waves + variants. Model swap is next: Mixamo Mutant FBXs → `import_mixamo_alien.py` → point `UGameConfig` alien soft paths at `/Game/Imported/Aliens/Mutant/`, refit scale/yaw, screenshot. Then: per-variant materials, AnimBlueprint blendspace, recoil model.

### After Sprint Y (2026-09-08 evening)

Richard's read after W+X: light great, character great, enemies weak, shooting off while running, wants an easy start that ramps. Sprint Y answers all three (see status table). Next human pass: does the ramp pace feel right (`RampKillsToMax` / `RampSecondsToMax`), and do the aliens now read as threats? Still open: AnimBlueprint blendspace for clip pops, physics asset for `SK_Alien`, recoil model, Git LFS.

### Resume checklist (after the 2026-09-08 Desktop access loss) — done

1. `git pull` on the Desktop checkout (it was left at `71f694f`; Sprints W+X live at `72a518b`).
2. Rebuild in place, run the self-test, expect **31 / 31**.
3. Run with `-NightShiftAutoStart`, confirm mannequin + rifle, animated aliens, brighter arena.
4. Human playthrough. Knobs if something is off: `ExposureBiasEV` (brightness), `AlienMeshYawDegrees` (facing), `AlienMeshScale` (size).
5. Follow-ups: AnimBlueprint + blendspace to remove clip pops; physics asset for `SK_Alien` (per-bone headshots); Git LFS decision.


Phases 6–8 + Sprints M/N/O/Q/R/V/W/X shipped 2026-09-08. Player is the UE mannequin with a rifle; aliens are animated Quaternius creatures; hitscan hits the visible bodies. **Visual unlock + silhouette confirmed** (`ue5-scaffold/Saved/sprintm_aliens_in_frame.png`). **Server-rack `.uasset` imported** (`/Game/Imported/Props/Office/SM_ServerRack`). Open items: optional **Git LFS** for `Content/Imported`, **human feel** (recoil). Phase 9 web + Phase 10 package remain parallel/last.

### Phase 6: First playthrough + softer start — **SHIPPED**

Self-test green. Richard feedback (“don’t die right away”) → Sprint C: spawn grace (~4s, no alien fire/aggro) + safer start spacing (`MinStartSeparationMeters` 18). Sprint V (2026-09-08, “too hard off the bat”): grace 7s, separation 24m, plus a 1.5s post-grace fire lock so aliens close in before they shoot. Mid/late DESIGN numbers unchanged. Still wants a human smoke pass for feel.

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

### Phase 7: Feel and visibility — **SHIPPED (code)**

Done earlier + 2026-09-08: greybox lighting tune (`BuildGreyboxLighting`); mantle reach/height + muzzle light intensity moved into `UGameConfig`. Sensitivity / Esc menu / vignette / shake already in.

Still open (human / content):

- Judge feel in PIE — **recoil** accumulate vs self-cancel still an open decision.
- Sprint X lighting is the third lift; judge in motion. Next knob if still dark: `UGameConfig::ExposureBiasEV` (0.6).
- Animation is single-node clip switching (no blends). If pops between clips bother you, the next step is an AnimBlueprint with a blendspace (template `ABP_Unarmed` shows the pattern) — the clips are already in `Content/Characters/Mannequins/Anims`.
- Alien facing: `AlienMeshYawDegrees` (-90) if they run sideways. Alien size: `AlienMeshScale` (0.55).
- Richer materials / Lumen mood beyond greybox + imported props.

### Phase 8: Mesh wire + mood props — **SHIPPED; visual unlock confirmed**

Soft refs + `ResolvePhase8LoadedMeshes` cache: `SM_Alien`, `SM_Cubicle`, desk/chair, fluorescents. Cubicle-only cover stamps; Omie desk/chair dress (Sprint N); Kenney rack stamps (Sprint Q — `SM_ServerRack` imported); ceiling fluorescents on underside (Sprint R); hit-flash/bio tint. Sprint M yaws start cam toward nearest alien so silhouettes read on Click-to-play. Docs: `ue5-scaffold/PHASE8_WIRE.md`.

**Open:** optional Git LFS for `Content/Imported/`; recoil feel; NavMesh flip / fuller atrium art from `LEVEL_SETUP_CHECKLIST.md`. Silhouette: `ue5-scaffold/Saved/sprintm_aliens_in_frame.png`. Rack asset: `/Game/Imported/Props/Office/SM_ServerRack`.

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
- **Alien body**: resolved — skeletal `SK_Alien`. No physics asset was generated on import, so hits use the refit capsule and headshots use the top-25% test; generating `PA_Alien` in the Editor would enable per-bone hits.
- **Where the canonical Unreal project lives**: Desktop Test `ue5-scaffold/` is canonical. ~120 MB of template + alien `.uasset`s are now committed without LFS (largest file < 20 MB); office props / fluorescents remain local-only.
