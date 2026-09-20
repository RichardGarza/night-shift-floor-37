# Phase 8 Import Manifest — Night Shift Floor 37

> **Location policy (Audit P1):** Mixamo Y Bot and Kenney `SM_Rifle` Imported **packages are NOT READY on tip/monorepo**. Soft paths exist in code (`/Game/Imported/Player/SK_Mixamo_YBot`, `/Game/Imported/Weapons/SM_Rifle`), but `Content/Imported/Player/**` and `Content/Imported/Weapons/**` have **0 tracked packages** on tip. Soft-miss → Epic Manny / no Kenney mesh is **expected** on a clean clone. Desktop SoftStarter-local `.uasset`s (or optional Git LFS later) are the only way those soft paths resolve. Do **not** claim Y Bot / SM_Rifle Imported packages READY.

Canonical root (Desktop SoftStarter-local): `/Users/garzamacbookair/Desktop/Test/night-shift-floor-37/ue5-scaffold/Content/Imported/`

## 1. Omie Office Set — READY (Desktop SoftStarter-local / staged FBX)
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

## 3. Poly Haven Mounted Fluorescent Lights — READY (Desktop SoftStarter-local / staged FBX)
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

## 4. Server Rack (Sprint Q) — FBX STAGED (uasset import pending)
- Soft path: `/Game/Imported/Props/Office/SM_ServerRack`
- Staged FBX: `Props/Office/SM_ServerRack.fbx` and `Props/Server/SM_ServerRack.fbx`
- License: **CC0** (Kenney Space Station Kit) — see `Props/Office/SM_ServerRack.ATTRIBUTION.txt`
- Editor: import FBX → `/Game/Imported/Props/Office/SM_ServerRack` for PIE soft-ref resolve
- Optional later: Sketchfab Dreadler Server Rack (CC-BY) if Sketchfab login available

## Player — Mixamo Y Bot — NOT IN MONOREPO (soft path only; packages empty on tip)
- Soft path (code only): `/Game/Imported/Player/SK_Mixamo_YBot`
- Tip / GitHub monorepo: `Content/Imported/Player/**` = **0 packages** — **not READY**
- Soft-miss on clean clone / tip without SoftStarter: **Epic Manny** (expected)
- Optional: Desktop SoftStarter-local `.uasset` import, or Git LFS later — until then do not claim Imported Y Bot READY

## Weapons — Kenney SM_Rifle — NOT IN MONOREPO (soft path only; packages empty on tip)
- Soft path (code only): `/Game/Imported/Weapons/SM_Rifle`
- Tip / GitHub monorepo: `Content/Imported/Weapons/**` = **0 packages** — **not READY**
- Soft-miss on clean clone / tip without SoftStarter: **no Kenney rifle mesh** (expected; greybox / soft-miss path)
- Optional: Desktop SoftStarter-local `.uasset`, or Git LFS later — until then do not claim Imported SM_Rifle READY
