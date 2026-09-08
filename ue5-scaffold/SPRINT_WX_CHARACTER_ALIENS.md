# Sprints W + X — real player body, rifle, animated aliens, hit fix, brighter arena

Shipped 2026-09-08. Everything here is code-driven; assets are plain meshes and clips.

## Assets (committed)

| Path | Source | Used by |
|---|---|---|
| `Content/Characters/Mannequins/Meshes/SKM_Manny_Simple` (+ `SK_Mannequin`, `PA_Mannequin`, Manny/Quinn materials) | UE 5.8 `Templates/TemplateResources/High/Characters` | `UGameConfig::PlayerSkeletalMesh` |
| `Content/Characters/Mannequins/Anims/Rifle/**`, `Anims/Death/**`, `Anims/Unarmed/**` | same | `UGameConfig::PlayerAnims` (idle ADS, walk/jog ×4 dirs, jump start/fall/land, reload, death) |
| `Content/Weapons/Rifle/Meshes/SM_Rifle` | `TemplateResources/Standard/Weapons` | `UGameConfig::RifleMesh`, socket `HandGrip_R` |
| `Content/Imported/Aliens/Skel/SK_Alien` + `SK_AlienCharacterArmature_*` + `Atlas_Monsters` | Quaternius Ultimate Monsters (CC0), `Scripts/import_alien_skeletal.py` | `UGameConfig::AlienSkeletalMesh`, `AlienAnims` (Idle, Walk, Run, Punch, HitReact, Death) |

Soft paths default in `UGameConfig::EnsurePhase8DefaultSoftPaths`. A missing asset falls back to the greybox cylinder / static alien.

## Player (`ANightShiftCharacter`)

- `ApplyConfiguredPlayerVisuals()` (from `ApplyResolvedGameConfig`): sets the skeletal mesh on `GetMesh()` at `-CapsuleHalfHeight`, yaw `PlayerMeshYawDegrees` (-90), hides the cylinder, attaches `RifleMeshComp` to `RifleSocketName`, applies `ExposureBiasEV` to the follow camera.
- `UpdateLocomotionAnim()` each tick: Idle < 15 cm/s; Walk clips below `PlayerWalkJogSplitSpeed` (320), Jog above; direction from actor-space velocity (Fwd/Bwd/Left/Right); play rate = speed / `Player{Walk,Jog}AnimRefSpeed`. Falling → FallLoop. One-shots: JumpStart (on Jump), Land (on Landed), Reload (fit to `ReloadSeconds`), Death (holds last frame).
- `GetMuzzleLocation()`: `Muzzle` socket if the rifle mesh has one, else the front edge of the prop bounds. Tracers and muzzle light start there; the **hit trace still comes from the camera** along the crosshair.

## Aliens (`AAlienBot`)

- Skeletal path in `ApplyConfiguredMeshes()`: `FitSkeletalBody()` scales by `AlienMeshScale` (0.55 → ~1.9 m), refits the capsule to the scaled ref-pose bounds, puts the feet on the capsule bottom, yaw `AlienMeshYawDegrees`.
- `UpdateAlienAnim()`: Idle (grace / no target), Run while chasing (rate = speed / `AlienRunAnimRefSpeed`), Walk while strafing, Punch when a burst starts, Death on kill (actor hides when the clip ends or just before respawn).
- Hit flash on the skeletal body: all material slots swap to a white MID for `HitFlashDurationMs`, then restore (the atlas material has no parameters, so the old param-driven flash was invisible).

## Shooting fix

Symptom: "shooting doesn't cause damage". Cause: `URifleComponent::Trace` hit only the alien capsule (Pawn profile + explicit Visibility block); the visible `SM_Alien` was scaled/offset from that capsule, so aiming at the body missed. Fix: trace with `bTraceComplex`, player + alien meshes block Visibility, and the alien capsule is refit to the visible body. Self-test asserts hits still land and kills count.

## Lighting

`AOfficeArena::BuildGreybox` albedos ×~2.5 (floor 0.20/0.23/0.22), `BuildGreyboxLighting` sun 5.0 / sky 1.6 / fog density 0.009 / extinction 0.45 / practicals 600 cd, plus `ExposureBiasEV` 0.6 (auto-exposure is off project-wide). Still too dark → raise `ExposureBiasEV` first.

## Verify

```
UnrealEditor <proj> /Game/Maps/Floor37 -game -windowed -NightShiftSelfTest -LogCmds="LogNightShift Verbose" -abslog=<log>
UnrealEditor <proj> /Game/Maps/Floor37 -game -windowed -NightShiftAutoStart      # screenshots with aliens in frame
```
31 / 31 self-test checks pass, including: player skeletal body, rifle prop visible, alien skeletal body, both animations playing.
