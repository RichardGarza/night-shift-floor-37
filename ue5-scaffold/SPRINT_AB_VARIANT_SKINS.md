# Sprint AB — variant skins, hit-react stagger, per-bone hits (2026-09-09)

Follow-on to the Mutant swap (Sprint AA), all on the Desktop checkout, pushed as it landed.

- **M_AlienVariant** (`Scripts/mutant_material_physics.py`, built node-by-node with
  `MaterialEditingLibrary`): `Diffuse` × `Tint` → BaseColor, `Normal`, `EmissiveColor` × `EmissiveStrength`,
  roughness 0.65; Mixamo textures as parameter defaults; assigned to `SK_Mutant` slot 0. `AAlienBot`
  drives the parameters per variant — Grunt `AlienGruntTint` (white), Brute `Tint` (1.0, 0.42, 0.38) +
  red resting emissive, Stalker (0.45, 0.85, 1.0) + cyan — and the hit flash now lerps the tint to white
  and spikes emissive instead of swapping to the white greybox material (the swap remains for the
  Quaternius atlas fallback).
- **Hit-react stagger** (`bAlienHitReact`, `AlienHitReactSeconds` 0.4, `AlienHitReactCooldownSeconds`
  1.1): a non-lethal hit plays `A_Mutant_Hit_Reaction` sped to fit 0.4 s while the bot holds position
  and drops its burst. The cooldown stops sustained fire from stun-locking a Brute.
- **Physics asset** `SK_Mutant_PhysicsAsset` (generated via `SkeletalMeshEditorSubsystem`). With it,
  the capsule ignores Visibility so the rifle trace hits the bodies; `IsHeadBone` matches
  `mixamorig:Head` → true per-bone headshots. Self-test still hits (47 / 47).

Next: arena surfaces (Poly Haven CC0 textures on floor / walls / tower / ramps) — Richard: "the walls
need texture, I'd love to see it start to come to life".
