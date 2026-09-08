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
