# Phase 8 Wire — Sprint G

Tip baseline: `51b700c`. Soft refs live on `UGameConfig` (`EnsurePhase8DefaultSoftPaths`).

## Staged FBX (import sources)

| Role | Source FBX | Import as StaticMesh asset path |
|------|------------|----------------------------------|
| Alien | `Content/Imported/Aliens/Alien.fbx` (Quaternius CC0) | `/Game/Imported/Aliens/SM_Alien` |
| Cover | `Content/Imported/Props/Office/SM_Cubicle.fbx` (Omie CC0) | `/Game/Imported/Props/Office/SM_Cubicle` |
| Desk (optional) | `…/SM_Desk.fbx` | `/Game/Imported/Props/Office/SM_Desk` |
| Chair (optional) | `…/SM_Chair.fbx` | `/Game/Imported/Props/Office/SM_Chair` |
| Fluorescent | `Content/Imported/Props/Lights/SM_MountedFluorescent.fbx` (Poly Haven CC0) | `/Game/Imported/Props/Lights/SM_MountedFluorescent` |

Also staged textures for Poly Haven under `Content/Imported/PolyHaven_MountedFluorescentLights/textures/`.

## One clean cover path

`CoverPropMesh` → Omie `SM_Cubicle` only. `AOfficeArena::ApplyConfiguredCoverMeshes` stamps non-colliding visuals at each cover volume. Query collision stays on `UBoxComponent` cover volumes.

## Runtime behavior

- Soft load succeeds → alien body swaps (head sphere hidden); cover props stamp; fluorescent soft ref ready for later lighting prop placement.
- Soft load fails (no `.uasset` yet) → greybox cylinders / no cover stamp (safe fallback).
- Bio tint: `ApplyFlashToMaterials` after static swap (`BodyColor` bright green).
- **Hit-flash on skeletal**: still follow-up when `AlienSkeletalMesh` is used (Quaternius path is static `AlienBodyMesh`).

## Editor import (SoftwareStarter / human)

1. Open `NightShiftFloor37.uproject` (UE 5.8).
2. Content Browser → import each staged FBX into the matching `/Game/Imported/...` folder.
3. Rename asset to `SM_Alien` / `SM_Cubicle` / `SM_MountedFluorescent` if importer uses FBX filename.
4. Optional: assign Poly Haven emission texture on fluorescent material.
5. PIE — aliens should show Quaternius mesh with bio tint; cover volumes show cubicle props.

## DA_GameConfig

Runtime `ResolveOrCreate` fills soft paths when null. Creating `DA_GameConfig` in Editor inherits CDO paths after recompile.


## Sprint H — fluorescent placement

`AOfficeArena::ApplyConfiguredFluorescentMeshes` stamps ≤4 `FluorescentLightMesh` instances on the greybox practical ring (indices 0,2,3,5). No collision, no mesh shadows. Soft-load miss → no stamp.


## Sprint I — hit-flash / bio tint

- `ApplyFlashToMaterials` drives Body/Head static MIDs **and** skeletal slot MIDs (`SkelMIDs`).
- After any mesh swap, `InvalidateFlashMIDs` forces fresh MIDs (stale MID after `SetStaticMesh` was the SM_Alien tint gap).
- Color params tried: `Color`, `BaseColor`, `Tint`, `DiffuseColor`; plus emissive fallbacks + stronger FlashLight (≤1400) so flash stays readable when mat pins differ.


## Sprint L — cubicle-only cover stamps

`ApplyConfiguredCoverMeshes` stamps `CoverPropMesh` only on volumes whose name contains `Cubicle` (N/E/S/W). Resin and rack cover volumes are query-only (no SM_Cubicle).


## Sprint N — cubicle desk/chair dress (NumberFourCoding)

`AOfficeArena::ApplyConfiguredOfficeDressMeshes` stamps **one desk + one chair** per cubicle cover volume (N/E/S/W only — same `Cubicle` name filter as Sprint L). Soft refs on `UGameConfig`:

| Soft ref | Asset |
|----------|-------|
| `DeskPropMesh` | `/Game/Imported/Props/Office/SM_Desk` |
| `ChairPropMesh` | `/Game/Imported/Props/Office/SM_Chair` |

- Count stays low (≤8 dress meshes).
- Soft miss → no stamp (greybox cover blocks remain).
- Resin/rack volumes are **not** dressed.
- Non-colliding visuals; query collision stays on cover boxes.
- Called from `BeginPlay` + GameMode soft-reset path alongside cover/fluorescent stamps.


## Sprint Q — server rack stamps (NumberFourCoding)

`AOfficeArena::ApplyConfiguredServerRackMeshes` stamps `ServerRackPropMesh` on cover volumes whose name contains `Rack` (`RackStack_A`, `RackStack_B`, `RackAngled` only). Parallel to Sprint L cubicle-only filter.

| Soft ref | Asset |
|----------|-------|
| `ServerRackPropMesh` | `/Game/Imported/Props/Office/SM_ServerRack` |

- Soft miss → no stamp (greybox rack blocks remain).
- Cubicle / resin volumes are **not** stamped.
- Cached via `UGameConfig::ResolvePhase8LoadedMeshes` (`CachedServerRackPropMesh`).
- Stand-in mesh: Kenney Space Station Kit **CC0** (`SM_ServerRack.fbx` under `Content/Imported/Props/Office/` and `Props/Server/`). Soft miss until Editor `.uasset` import. Not expecting Sketchfab Dreadler for this sprint.


## Sprint W2 — player grounded + rifle OTS (NumberFourCoding)

`ANightShiftCharacter::ApplyConfiguredPlayerVisuals`:
- Soft refs: `PlayerSkeletalMesh` (Y Bot default / Manny soft-miss), `PlayerBodyMesh` (optional static), `RifleMesh` (`SM_Rifle`)
- Feet: skeletal/static mesh Z = `-CapsuleHalfHeight + PlayerMeshZOffsetCm` (always re-applied)
- Soft-miss: grounded greybox cylinder (must not float)
- Rifle: socket try order `RifleSocketName` → `HandGrip_R` → `hand_r` → `weapon_r` → `ik_hand_gun` → `hand_r_socket`; then always apply `RifleRelative*` + `RifleMeshScale`


## Enemy swap — Mutant default + Kenney blaster

- `AlienSkeletalMesh` default: `/Game/Imported/Aliens/Mutant/SK_Mutant` (Mixamo Mutant) when package exists.
- `AlienBodyMesh` is **not** defaulted to Quaternius `SM_Alien`.
- `RifleMesh` default: `/Game/Imported/Weapons/SM_Rifle` (Kenney Blaster Kit `blaster-r`). **uasset READY.**
- Player W2 grounding unchanged.

## OTS gun verify (post SoftStarter SM_Rifle.uasset)

- Soft path `/Game/Imported/Weapons/SM_Rifle` resolves (uasset on disk).
- Locked OTS attach defaults — see **Rifle OTS attach polish** below.
- Mutant: `MutantMeshScale` **1.15** (~2.1 m vs player ~1.8 m) via `FitSkeletalBody` bounds fit — not tiny/huge.
- `ResolvePhase8LoadedMeshes` retries null rifle/alien caches after late SoftStarter imports.


## Rifle OTS attach polish (NumberFourCoding)

Locked `UGameConfig` defaults (Kenney `SM_Rifle`, OTS-readable on Y Bot / Manny hand sockets). Do not change casually — prior Testing PASS.

| Property | Default | Notes |
|----------|---------|-------|
| `RifleSocketName` | `HandGrip_R` | Fallbacks: Mixamo `mixamorig:RightHand` / `RightHand`, then `hand_r`, `weapon_r`, `ik_hand_gun`, `hand_r_socket` |
| `RifleRelativeLocation` | `(6, 2, -3)` cm | Socket-space offset after attach |
| `RifleRelativeRotation` | `(0, 90, 0)` | Yaw 90° — barrel forward in OTS |
| `RifleMeshScale` | `1.35` | Import size 1.0 reads tiny; clamp ≥0.05 in apply |

Apply path (`ApplyConfiguredPlayerVisuals`):
1. Attach to first existing socket (or soft-attach to skeletal/body/capsule).
2. Always set `RifleRelativeLocation` / `RifleRelativeRotation` / `RifleMeshScale`.
3. Soft-miss socket **and** ZeroVector location → hard-coded shoulder `(30, 25, 40)` (config location wins when non-zero).
4. Soft-miss mesh → hide prop (hitscan still works).


## Audit P2 — late Y Bot upgrade + Mixamo rifle sockets (NumberFourCoding)

1. **Late Y Bot after Manny soft-miss:** `ResolvePhase8LoadedMeshes` sets `bPlayerSkeletalMannySoftMiss` when soft-miss fills the cache with `SKM_Manny_Simple`. On later calls (after SoftStarter import), if `/Game/Imported/Player/SK_Mixamo_YBot` exists and that flag is set (or cache still null), upgrade soft-ref + cache to Y Bot — no longer blocked by a filled Manny cache.
2. **Mixamo rifle sockets:** socket try order now includes `mixamorig:RightHand`, `mixamorig_RightHand`, `RightHand`, `Hand_R` before mesh-root soft-attach. `RifleRelative*` still applied for HandGrip / all socket hits (HandGrip-tuned defaults kept).


## Optional KayKit Warrior (secondary enemy set)

- Toggle: `UGameConfig::bPreferKayKitWarrior` (**default false** — Mutant stays primary).
- Soft path preferred: `/Game/Imported/Enemies/KayKitWarrior/SK_KayKit_Warrior` (alt: `KayKit_Staged/SK_Skeleton_Warrior` if only that uasset exists).
- Requires SoftStarter FBX→uasset; if package missing with toggle on → warning, keep Mutant.
- Scale/yaw: `KayKitWarriorMeshScale` / `KayKitWarriorMeshYawDegrees` (1.0 / -90).
- Anims: Mutant clips as stand-in until KayKit anims imported.

## Audit-C — KayKit soft-miss → Mutant

- If `bPreferKayKitWarrior` is on but KayKit **anims** are not staged, `ApplyKayKitWarriorModelSet` falls back to the **full Mutant model set** (mesh+anims) — not greybox, not Mutant clips on a KayKit skeleton.
- If KayKit skeletal soft-misses at resolve time → Mutant model set.
- Toggle remains for when KayKit anims land.

## Player soft-ref — Mixamo Y Bot

- **Default soft path:** `/Game/Imported/Player/SK_Mixamo_YBot` (ModelFinder Mixamo Y Bot).
- **Editor uasset:** READY — `SK_Mixamo_YBot.uasset` + Skeleton + PhysicsAsset (SoftStarter). Soft-ref should resolve in PIE.
- **Soft-miss:** `/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple` (Epic Manny interim).
- Not defaulting to Quaternius `SK_Player`.
- W2 grounding + rifle OTS offsets unchanged after mesh swap.

