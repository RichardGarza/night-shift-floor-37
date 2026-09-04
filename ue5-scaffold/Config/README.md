# Config drop-in

1. Merge `DefaultInput.ini` Enhanced Input class defaults into your project's `Config/DefaultInput.ini`.
2. Merge `DefaultEngine.ini` (`GlobalDefaultGameMode=/Script/NightShiftFloor37.ArenaGameMode`) and `DefaultGame.ini` stubs into the host project (create those ini files in the host if missing).
3. Create IMC / IA assets per `../INPUT_MAPPING.md`.
4. **Expected missing Editor content** (create in Editor — see `../EDITOR_DROP_IN.md`):
   - `Content/Data/DA_GameConfig` — Data Asset of class `UGameConfig` (optional until Phase 5 ResolveOrCreate; assign on GameMode/Character when present).
   - IMC / Input Actions — assign on Character BP.
   - `WBP_NightShiftHUD` — assign `HUDWidgetClass` on GameMode.
   - Place `AFXPoolManager` in the level for tracers / muzzle lights.
