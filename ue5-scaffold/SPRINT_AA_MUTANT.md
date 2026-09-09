# Sprint AA — Mixamo Mutant replaces the Quaternius alien (2026-09-09)

Richard: "the alien models are super lame." He downloaded the Mixamo **Mutant** (T-pose FBX + Idle,
Walking, Run, Punch, Dying, Hit Reaction clips) into `~/Downloads/NightShiftAlien/`; macOS blocked
the shell from reading Downloads even with Full Disk Access on Terminal, so Finder (AppleScript)
copied the folder to `Content/Imported/NightShiftAlien/` (FBX sources, untracked).

## Import

`Scripts/import_mixamo_alien.py` (env `NS_MIXAMO_DIR`, `NS_MIXAMO_NAME=Mutant`) via the Python
commandlet → `/Game/Imported/Aliens/Mutant/`: `SK_Mutant`, `SK_Mutant_Skeleton`, `mutant_M` +
diffuse/normal textures, `A_Mutant_{Idle,Walking,Run,Punch,Dying,Hit_Reaction}`. Bounds 1.86 m tall,
feet at Z 0. All clips then got `force_root_lock` + root motion off (one-off Python pass) so the loops
cannot drift off the capsule even if "In Place" was not ticked on Mixamo. No physics asset was
generated (capsule remains the hit volume; head = top 25 %).

## Wiring (`UGameConfig`)

`bAutoPickAlienModelSet` (default true): `EnsurePhase8DefaultSoftPaths` checks whether the
`SK_Mutant` package exists and calls `ApplyAlienModelSet(true)` — Mutant soft paths plus
`MutantMeshScale` 1.15 (≈ 2.1 m Grunt; Brute ×1.35 ≈ 2.9 m), `MutantMeshYawDegrees` −90,
`MutantRunAnimRefSpeed` 420. Otherwise, or if the load fails, the Quaternius set (0.55 / −90 / 350)
applies unchanged. Set the flag false in a Data Asset to hand-assign paths.
`AAlienBot::Die` now speeds the death clip to fit the respawn window (Mixamo Dying is 4.6 s).

## Verification

Self-test **47 / 47** with `alienSkel=SK_Mutant`; `docs/sprintaa_mutant_wave4.png` — wave 4, two
Grunts and a Brute Mutant facing the player.

## Next

Per-variant materials (Brute/Stalker tints now possible: `mutant_M` is a real material instance
with a diffuse), hit-react clip on damage, physics asset for per-bone headshots, AnimBlueprint blends.
