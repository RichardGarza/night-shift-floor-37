# Phase 5 — Polish (code)

Canonical: `/home/box/sand-data/projects/night-shift-floor-37/ue5-scaffold/`

## Checklist (Audit scope)

| Item | Status |
|---|---|
| (1) GameConfig auto-resolve BeginPlay | **Done** — `UGameConfig::ResolveOrCreate` + `ResolveAndPropagateGameConfig` / Character `ApplyResolvedGameConfig` |
| SoftRestart despawn-only | **Done** — no `EnsureAlienPopulation` call in SoftRestartAlienPool; populate only `InProgress` |
| (2) Alien hit-flash 80ms | **Done** — `HitFlashAlpha` + `OnHitFlash`; bind mesh emissive in Editor MID |
| (3) NavMesh MoveTo notes/stubs | **Done** — `bPreferNavMeshMoveTo` + `TryNavMeshMoveToTarget`; see `NAVMESH_NOTES.md` |
| (4) Optional mantle | **Done** — Space → `TryJumpOrMantle` (ledge teleport stub) |
| Pause gate InProgress | **Done** |
| EDITOR_DROP_IN.md | **Present** (Coding Bot) |

## Editor leftovers

DA_GameConfig asset (optional), IMC/IA, WBP→HUDWidgetClass, build NavMesh, material emissive on aliens, Nanite/Lumen art.
