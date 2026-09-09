# Sprint AD — dressing pass: glass, atrium lights, resin, work pods (2026-09-09)

Second "bring it to life" pass after the textured arena (Sprint AC).

- **Glass band** darker and rougher (`M_DirtyGlass` tint 0.012/0.03/0.034, roughness 0.42, metallic 0.12) so
  the strip above the painted wall stops reading as a white wall of sky.
- **Atrium plate lights** — three cool point lights (`GB_PlateLight_*`, 900 cd, 11 m, no shadows) under the
  L1/L2/L3 plates (`UGameConfig::AtriumPlateLightIntensity`). The column and plate undersides now catch light
  instead of sitting in their own shadow.
- **Resin** — `M_Resin` (dark glossy green, faint emissive 0.10) on the four egg/resin cover blocks via
  `EArenaSurface::Resin` / `UGameConfig::SurfaceResin`. First pass glowed like mint ice; toned down.
- **Work pods** — `OfficePodCount` (8) desk pairs + askew chairs on a ring (`OfficePodRingRadiusCm` 2050)
  at the half-angles between the eight edge spawns, facing the atrium. Non-colliding dress, like the cubicle
  desks. Total office dress is now 40 meshes. The Omie office props (`Content/Imported/Props/Office`) and the
  Poly Haven fluorescent (`Props/Lights`) are now committed (2.3 MB) so a fresh clone gets them.

Self-test 47 / 47; `34 blocks textured`, `stamped 40 desk/chair props`. `docs/sprintad_pods_resin_lights.png`.

Next candidates: ceiling decision (the sky is doing the lighting), wall-mounted fixtures / signage,
floor grime decals, alien resin growths on walls, AnimBlueprint blends, recoil model.
