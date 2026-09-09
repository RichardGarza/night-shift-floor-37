# Sprint AE — infestation and signage: resin growths, dying neon, wave banners (2026-09-09)

- **Resin growths** (`AOfficeArena::ApplyResinGrowths`, `ResinGrowthsPerCover` 7): squashed engine spheres in
  `M_Resin` climbing each egg-cover block and spilling onto the floor, leaning toward the nearest wall.
  Deterministic `FRandomStream` per cover so the layout is stable. 28 blobs, transient, no collision.
- **Dying fluorescents**: the arena now ticks (cheap) and modulates each neon trim MID — a mains hum plus
  Perlin-driven dips (`NeonFlickerDepth` 0.75, `NeonFlickerRate` 0.6). Strips flicker independently.
- **Wave-start banner**: "Wave N of 8 — K kills to clear" for `WaveStartBannerSeconds` (2.2) at match start
  and after each breather; the timer only clears its own banner (death / win / breather prompts win).
- Resin material a touch rougher / less specular after the first pass read as glossy mint.

Self-test 47 / 47. `docs/sprintae_resin_growths.png`.
