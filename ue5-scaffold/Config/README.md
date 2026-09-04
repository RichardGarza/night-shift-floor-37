# Config drop-in

1. Merge `DefaultInput.ini` Enhanced Input class defaults into your project's `Config/DefaultInput.ini`.
2. Merge `DefaultEngine.ini` (`GlobalDefaultGameMode=/Script/NightShiftFloor37.ArenaGameMode`) and `DefaultGame.ini` stubs into the host project.
3. Create IMC / IA assets per `../INPUT_MAPPING.md`.
4. **Expected missing Editor content** (code auto-resolves where possible):
   - `Content/Data/DA_GameConfig` — C++ loads `/Game/Data/DA_GameConfig` or `NewObject` DESIGN defaults in PIE.
   - IMC / Input Actions — assign on Character BP.
   - `WBP_NightShiftHUD` — assign `HUDWidgetClass` on GameMode.
   - Place `AFXPoolManager` in the level for tracers / muzzle lights.
