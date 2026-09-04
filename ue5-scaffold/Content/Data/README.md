# Content/Data — GameConfig

## Expected Editor asset (not shipped in this scaffold)

Create a **Data Asset** of class `UGameConfig`:

| Soft path | Name |
|---|---|
| `/Game/Data/DA_GameConfig` | `DA_GameConfig` |

Assign it on `AArenaGameMode::GameConfig` (and Character / FX pool / bots as listed in [`../../EDITOR_DROP_IN.md`](../../EDITOR_DROP_IN.md)). Tunables default to **DESIGN.md** values on the CDO.

**Why create it if ResolveOrCreate exists?** Shared designer tuning and packaging — one asset everyone reads. Runtime fallback is for PIE convenience, not a substitute for Content.

## Runtime resolve (Phase 5 — landed)

`UGameConfig::ResolveOrCreate`:

1. Use the EditAnywhere reference if already set.
2. Else `StaticLoadObject` `/Game/Data/DA_GameConfig.DA_GameConfig`.
3. Else `NewObject<UGameConfig>` owned by GameMode/Character with DESIGN UPROPERTY defaults.

GameMode propagates the resolved config to alien pool bots, player character, rifle, and `UArenaCollision`.

See [`../../EDITOR_DROP_IN.md`](../../EDITOR_DROP_IN.md) for click-path steps.
