# Phase 8 Import Manifest — Night Shift Floor 37

Canonical root: `/Users/garzamacbookair/Desktop/Test/night-shift-floor-37/ue5-scaffold/Content/Imported/`

## 1. Omie Office Set — READY
- Path: `OmieOfficeSet/extracted/` (from `OmieOfficeSet_UE_Ready.zip`)
- License: **CC0**
- Source: https://omies-assets.itch.io/omies-assets-office-set
- Contents: FBX + PBR PNGs (cubicle, desk, chair, computer set, supplies, office man)
- Note: Full itch zip (~503MB) included Substance Painter `.spp` sources; UE-ready subset excludes those.
- Import: drag FBX from `extracted/Office Cubicle/*/Models/` into UE Content Browser

## 2. Quaternius Ultimate Monsters — IN PROGRESS (Alien already present)
- Path: `QuaterniusUltimateMonsters/pack/`
- License: **CC0**
- Source: https://quaternius.com/packs/ultimatemonsters.html
- Drive: https://drive.google.com/drive/folders/18m4KpzpEzhC9wl7jzr6dUc0N8Jozr79C
- Alien FBX (ready now): `QuaterniusUltimateMonsters/pack/Big/FBX/Alien.fbx`
- Also: `Big/glTF/Alien.gltf`, `Big/OBJ/Alien.obj`

## 3. Poly Haven Mounted Fluorescent Lights — READY
- Path: `PolyHaven_MountedFluorescentLights/`
- License: **CC0** (https://polyhaven.com/license)
- Source: https://polyhaven.com/a/mounted_fluorescent_lights
- Files: `mounted_fluorescent_lights_2k.fbx` + `textures/` (diff, emission, metal, nor_gl, rough)
- Import: FBX + assign emission map for Night Shift lighting

## For NumberTwoCoding
Import from `Content/Imported/` into project Content (suggested folders: `Props/Office`, `Characters/Enemies`, `Props/Lights`).


## Sprint G wire status (NumberTwoCoding)

- Staged import-ready FBXs: `Imported/Aliens/Alien.fbx`, `Imported/Props/Office/SM_{Cubicle,Desk,Chair}.fbx`, `Imported/Props/Lights/SM_MountedFluorescent.fbx`
- Soft refs default via `UGameConfig::EnsurePhase8DefaultSoftPaths` — see `ue5-scaffold/PHASE8_WIRE.md`
- Editor `.uasset` import still required for meshes to appear in PIE (greybox until then)
