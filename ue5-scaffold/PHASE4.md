# Phase 4 — Match loop package

Scaffold root: this directory (`ue5-scaffold/`).

Wires **click-to-play / death restart / win @ 25 kills / soft reset / Esc pause / HUD prompts + hit-marker**. Preserves Phase 3 alien pool (`BuildAlienPool` / `RespawnAlien` / `EnsureAlienPopulation` / `SoftRestartAlienPool`). Does **not** rewrite `AOfficeArena` cover/spawns.

## Design contract

| Spec | Behavior |
|---|---|
| Start | HUD **"Click to play"** → left click → `RequestStartOrRestart` → `StartMatch` |
| Death | `NotifyPlayerDied` → **"You died — click to restart"** → click → `SoftRestart` + `StartMatch` |
| Win | `KillCount >= KillsToWin` (25) → **"Floor cleared — {time}s"** → click restarts |
| Soft reset | Same level: player transform, HP/ammo, kills, timer, alien pool |
| HUD | Crosshair, HP, ammo, kills, timer, hit-marker (`IsHitMarkerVisible`) |
| Esc | `RequestPause` toggles `PauseMatch` → cursor + GameAndUI vs GameOnly |

## State machine

```
WaitingToStart ──click──► InProgress ──die──► Lost ──click──► (SoftRestart) ► InProgress
                     │                              ▲
                     └──25 kills──► Won ──click─────┘
                     
InProgress + Esc ⇄ paused (timer frozen; input UI unlocked)
```

| State | Timer | Aliens | Input | Prompt |
|---|---|---|---|---|
| `WaitingToStart` | frozen | pool built, not chasing until StartMatch | GameAndUI + cursor | Click to play |
| `InProgress` | runs | `EnsureAlienPopulation` | GameOnly | cleared |
| `InProgress` + paused | frozen | live but match tick skipped | GameAndUI + cursor | (none / optional) |
| `Lost` | frozen | as-is | GameAndUI + cursor | You died — click to restart |
| `Won` | frozen | as-is | GameAndUI + cursor | Floor cleared — Ns |

`OnMatchStateChanged` broadcasts on every `SetMatchState` transition.

## Flow (code)

### BeginPlay (`AArenaGameMode`)

1. `FindOrCacheArena` / `BuildAlienPool` (Phase 3)
2. `RecordStartTransform` — prefer `APlayerStart`, else player pawn
3. `CreateAndBindHUD` — if `HUDWidgetClass` set: `CreateWidget` → `AddToViewport` → `BindToMatch` → `ShowStartPrompt`
4. Bind `Rifle->OnHitConfirmed` → `UHUDWidget::ShowHitMarker`
5. `WaitingToStart` + `UpdatePlayerInputMode`

### Click path

`UHUDWidget::HandlePrimaryClick` / `NativeOnMouseButtonDown` → `RequestStartOrRestart`:

- `WaitingToStart` → `StartMatch` (clear prompt, GameOnly, populate aliens)
- `Lost` / `Won` → `SoftRestartInternal(false)` + `StartMatch` (one click; no second prompt)

### SoftRestart

1. Reset kills / timer / unpause → `WaitingToStart`
2. `ResetPlayerTransform` to recorded start
3. `ANightShiftCharacter::SoftResetPlayerState` (HP + ammo)
4. `SoftRestartAlienPool` (Phase 3 SoftReset + redistribute)
5. `ShowStartPrompt` (skipped visually when immediately followed by `StartMatch` from click)

### Combat → HUD

- Death: character `OnDied` path → `NotifyPlayerDied` → death prompt
- Win: `RegisterKill` → `CheckWinCondition` → `ShowWin(MatchTimeSeconds)`
- Hit-marker: rifle `OnHitConfirmed` → `ShowHitMarker` (~120 ms via `IsHitMarkerVisible`)

### Esc pause

`ANightShiftCharacter::RequestPause` → `PauseMatch(!IsMatchPaused())` → mouse cursor + `FInputModeGameAndUI` vs `FInputModeGameOnly`.

## Editor setup

1. Create UMG Widget Blueprint parented to **`UHUDWidget`** (e.g. `WBP_NightShiftHUD`).
2. Implement `OnRefreshHUD` / `OnPromptChanged` (crosshair, HP bar, ammo `Mag / Reserve`, kills, timer, prompt text). Bind hit-marker visibility to `IsHitMarkerVisible()`.
3. On **`AArenaGameMode`** (or BP GameMode): assign that WBP to **`HUDWidgetClass`**.
4. Assign `UGameConfig` data asset (`KillsToWin = 25`, `PlayerMaxHealth`, etc.).
5. Ensure level has `APlayerStart` (soft-reset teleport target) + Phase 3 arena/pool wiring.
6. Esc → Enhanced Input `PauseAction` already bound on the character.

If `HUDWidgetClass` is unset, match logic still runs and a one-time warning is logged.

## Files touched

| File | Change |
|---|---|
| `Public/ArenaGameMode.h` | Already had Phase 4 API (`HUDWidgetClass`, `RequestStartOrRestart`, pause, start transform helpers) |
| `Private/ArenaGameMode.cpp` | HUD create/bind, start transform, soft restart transform, pause input modes, prompts, win/death, rifle→hit-marker |
| `Public/HUDWidget.h` | `ClearPrompt`, `HandlePrimaryClick`, `IsHitMarkerVisible`, mouse-down |
| `Private/HUDWidget.cpp` | Prompt clear/click, MaxHP from `GameConfig`, hit-marker helper |
| `Private/NightShiftCharacter.cpp` | `RequestPause` toggles pause |
| `PHASE4.md` | This doc |

**Not edited:** `OfficeArena.*`, Phase 3 pool semantics, Three.js, git.

## Smoke checklist

- [ ] PIE → "Click to play" → click → timer runs, 6 aliens, GameOnly cursor locked
- [ ] Esc → cursor unlock / pause; Esc again → resume
- [ ] Die → death prompt → click → soft reset (HP/ammo/kills/timer/transform) + live again
- [ ] 25 kills → win prompt with time → click restarts
- [ ] Hit alien → hit-marker flashes (~0.12 s)
- [ ] HUD shows HP (from GameConfig max), ammo, kills, timer

## NumberTwoTesting note (fixed)

`SoftRestartAlienPool` SoftResets/despawns only. `EnsureAlienPopulation` runs only when `MatchState == InProgress` (via `StartMatch` / tick). WaitingToStart no longer leaves chasing bots.
