# Content/Data — GameConfig

## Expected Editor asset (not shipped in this scaffold)

Create a **Data Asset** of class `UGameConfig`:

| Soft path | Name |
|---|---|
| `/Game/Data/DA_GameConfig` | `DA_GameConfig` |

Assign it on `AArenaGameMode::GameConfig` (and optionally Character / bots). Tunables default to **DESIGN.md** values on the CDO.

## PIE without the asset (Phase 5 — TODO)

`UGameConfig::ResolveOrCreate` is **not landed yet**. Until Phase 5:

1. Create and assign `DA_GameConfig` in the Editor, **or**
2. Rely on CDO / EditAnywhere defaults wired on GameMode / Character.

Planned Phase 5 behavior (when implemented): EditAnywhere ref → `StaticLoadObject` `/Game/Data/DA_GameConfig` → else `NewObject<UGameConfig>` with DESIGN defaults.
