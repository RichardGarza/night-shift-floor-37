# Sprint AC — arena surfaces: the floor comes to life (2026-09-09)

Richard: "add detail, the walls need texture, I'd love to see it start to come to life."

## Textures (Poly Haven, CC0, 1K JPG: Diffuse / nor_gl / arm)

| Role | Asset | Tile (cm) | Used on |
|---|---|---|---|
| floor | `floor_tiles_02` | 220 | GB_Floor (RoughnessScale 0.55 → wet tile) |
| wall | `peeling_painted_wall` | 300 | 2 m perimeter wall (GB_Spandrel*), cubicle cover blocks |
| concrete | `concrete_wall_008` | 320 | atrium column, plates, conference pad |
| metal | `corrugated_iron_02` | 160 | ramps and bridges |
| berm | `concrete_floor_02` | 260 | collapsed drywall berm |

Sources sit untracked in `Content/Imported/Textures/PolyHaven/<asset>/` with `roles.json`; download again with the
curl loop in the session log (`api.polyhaven.com/files/<asset>` → `1k` jpg URLs). Python 3.13 on this Mac has
no CA bundle (`urllib` fails SSL) — use `curl`.

## Materials (`Scripts/build_surface_materials.py`, headless Python)

- Imports the maps to `/Game/Imported/Surfaces/<asset>/T_<asset>_{D,N,ARM}` — normals `TC_Normalmap`
  with **flip green** (Poly Haven ships OpenGL +Y, UE wants DirectX −Y), ARM `TC_Masks` linear.
- **M_WorldSurface**: world-position UVs so the scaled engine cubes tile at true scale. `PlaneSel`
  0 = XY, 1 = XZ, 2 = YZ (two lerps), `Tile` cm per repeat, `Tint`, `RoughnessScale`; ARM → AO / Roughness / Metallic.
  Node defaults must be real textures of the right kind — Metal refuses Normal / Masks samplers on the
  engine DefaultTexture ("Sampler type is Normal, should be Color"), and the ARM node must be
  `SAMPLERTYPE_MASKS` to match `TC_Masks`. The material is deleted and rebuilt on each run.
- `MI_Floor / MI_Wall / MI_Concrete / MI_Metal / MI_Berm` instances; **M_DirtyGlass → MI_Glass**
  (dark teal, roughness 0.32, metallic 0.25) for the band above the wall; **M_Neon** (EmissiveColor ×
  EmissiveStrength) for the trim strips.

## Arena (`AOfficeArena`)

- `EArenaSurface` per greybox block (`AddGreyboxBox(..., Surface)`); `ApplyConfiguredSurfaceMaterials`
  (called from the GameMode's config propagation) makes a MID per block from the `UGameConfig`
  `Surface*` soft refs, picks `PlaneSel` from the block's thin axis, applies `FloorTint` /
  `ConcreteTint` / `WallTint`, and for Neon uses the block colour as emissive (green N/S, amber E/W,
  `NeonEmissiveStrength` 7). Soft-miss keeps the colour MIDs.
- Perimeter is now a 2 m painted wall + 1.2 m dirty-glass band + 8 cm neon trim at the seam
  (`PerimeterWallHeightCm`).
- Two latent bugs fixed on the way: `GreyboxMeshes` is now **Transient** (the map had saved the
  22-entry list from an older class, so new blocks misaligned with colours / surfaces — bridges wore the
  neon material); and `AddGreyboxBox` resolves the engine cube itself, because the cover-volume blocks
  were created before the constructor's mesh lookup and had **no mesh at all** (racks / resin / cubicles
  were invisible blocks).
- `-NightShiftGodMode` on the command line: the player ignores damage (screenshot / smoke runs).

## Verification

Self-test 47 / 47; log line `ApplyConfiguredSurfaceMaterials — 30 blocks textured`.
`docs/sprintac_textured_arena_wave1.png`, `docs/sprintac_textured_arena_wave5.png`.

## Next

Glass band still reads bright (sky reflection) — candidate: darker tint / higher roughness, or a
window-mullion strip texture. Tower concrete sits in its own shadow — a bounce/fill light or a lighter
`ConcreteTint`. Then resin cover material, more office dressing, ceiling decision.
