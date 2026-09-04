# UI

Place UMG assets here when dropping into a UE5 project.

## Phase 4 HUD

1. Create `WBP_NightShiftHUD` parented to C++ `UHUDWidget`.
2. Implement `OnRefreshHUD` / `OnPromptChanged`; poll `IsHitMarkerVisible` for hit-marker.
3. Wire click → `HandlePrimaryClick` (C++ `NativeOnMouseButtonDown` already calls it — keep widget hit-testable when prompts show).
4. Assign the WBP to `AArenaGameMode::HUDWidgetClass` in Editor.

**Full click-path + GameMode binding:** [`../../EDITOR_DROP_IN.md`](../../EDITOR_DROP_IN.md) §3.

See also [`../../PHASE4.md`](../../PHASE4.md) for the match-loop bind checklist.
