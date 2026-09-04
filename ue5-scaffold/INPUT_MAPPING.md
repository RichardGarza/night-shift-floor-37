# Enhanced Input — Night Shift Floor 37

Create these assets under `Content/` (e.g. `Content/Input/`) and assign them on `ANightShiftCharacter`.

## Input Actions

| Asset name | Value type | Consume input | Notes |
|---|---|---|---|
| `IA_Move` | Axis2D (Vector2D) | No | WASD |
| `IA_Look` | Axis2D (Vector2D) | No | Mouse |
| `IA_Jump` | Digital (bool) | Yes | Space — jump / mantle |
| `IA_Sprint` | Digital (bool) | No | Left Shift — hold |
| `IA_Fire` | Digital (bool) | No | LMB — hold for full-auto |
| `IA_Reload` | Digital (bool) | Yes | R |
| `IA_ShoulderSwap` | Digital (bool) | Yes | Q |
| `IA_Pause` | Digital (bool) | Yes | Esc — pause / unlock |

## Mapping Context: `IMC_NightShift`

| Action | Key | Modifiers | Triggers |
|---|---|---|---|
| `IA_Move` | W | Swizzle / Negate as needed for Axis2D Y+ | — |
| `IA_Move` | S | Negate Y | — |
| `IA_Move` | A | Negate X | — |
| `IA_Move` | D | X+ | — |
| `IA_Look` | Mouse XY | — | — |
| `IA_Jump` | Space | — | Pressed |
| `IA_Sprint` | Left Shift | — | Ongoing / Pressed+Released |
| `IA_Fire` | Left Mouse Button | — | Pressed + Released (Started/Completed) |
| `IA_Reload` | R | — | Pressed |
| `IA_ShoulderSwap` | Q | — | Pressed |
| `IA_Pause` | Escape | — | Pressed |

Recommended: also map gamepad sticks/face buttons later; prototype is KBM-first.

## Character wiring

`ANightShiftCharacter::SetupPlayerInputComponent` binds:

- Move / Look → `Triggered`
- Jump → `Started` / `Completed` (Jump / StopJumping)
- Sprint → `Started` / `Completed`
- Fire → `Started` / `Completed` → `Rifle->Fire` / `StopFire`
- Reload → `Started`
- ShoulderSwap → `Started`
- Pause → `Started` → GameMode pause

Soft lock (alien in reticle cone) is gameplay logic on the rifle/aim path — not an Input Action.
