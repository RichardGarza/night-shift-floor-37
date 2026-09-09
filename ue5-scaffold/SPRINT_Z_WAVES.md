# Sprint Z — wave progression + alien variants (2026-09-08)

Richard on Sprint Y: "There should be a progression, not just a ramp up to 100 %. Also the alien
models are super lame." This sprint is the progression; the model swap (Mixamo Mutant) follows once
the FBX files are downloaded — `Scripts/import_mixamo_alien.py` is ready for them.

## Waves

`bWaveProgression` (default on) supersedes the Sprint Y ramp and the flat 25-kill win.

| Wave | Live aliens | Kill quota | Accuracy | Burst every | Chase speed | New |
|---|---|---|---|---|---|---|
| 1 | 2 | 4 | 10 % | 3.0 s | 400 | — |
| 2 | 3 | 5 | 14 % | 2.7 s | 410 | |
| 3 | 4 | 6 | 18 % | 2.4 s | 420 | **Brute** |
| 4 | 5 | 7 | 22 % | 2.1 s | 430 | |
| 5 | 6 | 8 | 26 % | 1.8 s | 440 | **Stalker** |
| 6 | 7 | 9 | 30 % | 1.5 s | 450 | |
| 7 | 8 | 10 | 34 % | 1.2 s | 460 | |
| 8 | 8 | 11 | 38 % | 0.9 s | 470 | win on clear (60 kills) |

Formulas live on `UGameConfig` (`Wave*` knobs): quota = `WaveKillQuotaBase` 3 + `WaveKillQuotaPerWave` 1 × N;
live = `WaveStartLiveAliens` 2 + `WaveLiveAliensPerWave` 1 × (N−1), capped at `WaveMaxLiveAliens` 8
(the pool grows to 8 in wave mode — one per edge spawn); accuracy / burst / speed step per wave with caps.
`WavesToWin` 8; set 0 for endless (HUD shows `Wave N`).

**Clearing a wave** (`AArenaGameMode::OnWaveCleared`): the floor empties, ammo and HP restock
(`bWaveClearRefillsAmmo`, `bWaveClearHeals`), the banner reads "Wave N cleared — Wave N+1 in 5 s ·
ammo and HP restocked" (`WaveBreatherSeconds`), then the next wave spawns at the far edges with a
2 s grace (`WaveStartGraceSeconds`) and the usual 1.5 s fire lock. Kills during the breather do not count.
Clearing `WavesToWin` shows "Floor cleared — 8 waves · 60 kills · m:ss".

HUD: `Kills 4 / 6` is this wave's quota; `Wave 3 / 8` replaces the Threat line (amber from wave 4, red on 8).

## Variants (`EAlienVariant`, `FAlienVariantTuning`)

| | Grunt | Brute (`UGameConfig::Brute`) | Stalker (`UGameConfig::Stalker`) |
|---|---|---|---|
| unlock | — | wave 3 | wave 5 |
| share of live | rest | ≤ 34 % (min 1) | ≤ 34 % (min 1) |
| scale | 1.0 | 1.35 | 0.8 |
| speed | 1.0 | 0.8 | 1.4 |
| kill | 3 body / 2 head | 6 / 3 | 2 / 1 |
| burst interval | 1.0 | 1.0 | 0.7 |
| accuracy bonus | 0 | 0 | +5 % |
| glow | hit flash only | red 400 cd | cyan 300 cd |

`AArenaGameMode::PickVariantForSpawn` fills Brute then Stalker quotas from the live count on every
activation (respawns included). `AAlienBot::ApplyVariantPresentation` refits the capsule to the scaled
mesh, re-seats the feet on the floor, sets speed and the always-on glow. The atlas material ignores
tints, so scale + glow are the read; the Mixamo swap will let materials differ per variant.

## Testing

- Self-test **47 / 47**: wave 1 opens with 2 aliens; forcing the quota → breather, floor empty, ammo
  restocked, fire locked; wave 2 fields 3 aliens with higher accuracy; restart resets the wave;
  jumping to wave 8 and clearing it → Won with variants in the spawn mix.
- `-NightShiftStartWave=N` with `-NightShiftAutoStart` opens on wave N (screenshots / smoke).
  `docs/sprintz_wave6_variants.png` — wave 6, Brute beside Grunts, HUD `Wave 6 / 8`.

## Knobs to try

Pace: `WaveKillQuotaPerWave` (0 = flat quotas), `WaveBreatherSeconds`. Bite: `WaveAccuracyPerWave`,
`WaveBurstIntervalPerWave`. Length: `WavesToWin` (5 for a short session, 0 endless).
