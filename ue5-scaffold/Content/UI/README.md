# UI

Place UMG assets here when dropping into a UE5 project.

## Phase 4 HUD

1. Create `WBP_NightShiftHUD` parented to C++ `UHUDWidget`.
2. Implement `OnRefreshHUD` / `OnPromptChanged`; poll `IsHitMarkerVisible` for hit-marker.
3. Wire click → `HandlePrimaryClick`.
4. Assign the WBP to `AArenaGameMode::HUDWidgetClass` in Editor.

See `../../PHASE4.md` for the full match-loop bind checklist.
