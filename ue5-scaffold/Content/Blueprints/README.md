# Blueprints

Place Blueprint assets here when dropping into a UE5 project.

## Editor-created BPs (not shipped)

| Suggested name | Parent C++ | Purpose |
|---|---|---|
| `BP_NightShiftCharacter` | `ANightShiftCharacter` | Wire `GameConfig`, `IMC_NightShift`, and all Input Actions |
| `BP_ArenaGameMode` | `AArenaGameMode` | Wire `GameConfig`, `HUDWidgetClass`, Default Pawn Class |

These Blueprints are **created in the Editor** after the C++ scaffold compiles. They are not included as `.uasset` files in this repo.

Checklist (Input wiring, HUDWidgetClass, World Settings): **[`../../EDITOR_DROP_IN.md`](../../EDITOR_DROP_IN.md)**.
