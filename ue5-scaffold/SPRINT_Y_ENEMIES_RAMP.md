# Sprint Y — enemies that face you, sprint-fire, difficulty ramp (2026-09-08)

Richard's second playthrough: "Light is great, character great, enemies still suck. Shooting is fine
except when running. Difficulty should start super easy then more spawning enemies and more
shooting at me too." Three fixes, all code, all in `UGameConfig` knobs.

## Enemies

- **Face the player in combat range.** `AAlienBot` used orient-to-movement everywhere, so a strafing
  alien pointed sideways and its tracers left its hip. In `StrafeBurst` the movement component's
  orient-to-movement is off and `FaceTarget` yaws toward the player at
  `AlienFaceTargetTurnRateDegPerSec` (540). A burst only starts once the yaw error is within
  `AlienFireFacingToleranceDegrees` (25). `ChasePlayer` turns orient-to-movement back on.
  `bUseControllerRotationYaw` is now false on the bot so the AIController never fights this.
- **Plant to shoot.** `bAlienPlantsDuringBurst` — no strafe input while the attack clip / burst is
  live, so the punch reads and the Walk↔Attack clip swap happens standing still. Strafing resumes
  between bursts (DESIGN's stop / strafe / burst loop is intact).
- **No more shivering against cover.** Obstacle steering used to flip the lateral sign every blocked
  frame. Now it probes left and right once (60° off axis), keeps the clearer side, and holds it for
  `AlienSteerCommitSeconds` (0.6).

## Shooting while running

`bFireCancelsSprint` (default on): holding fire caps the player at `WalkSpeed`. The 9 m/s sprint
played the jog clip at 1.9× and threw the rifle (tracer origin, muzzle light) all over the frame.
Sprint comes back `SprintResumeAfterFireSeconds` (0.4) after the trigger is released while Shift is
still held. Hitscan itself was already camera-accurate; this is a feel fix.

## Difficulty ramp

`bDifficultyRamp` (default on). Alpha = max(kills / `RampKillsToMax` 15, time / `RampSecondsToMax`
150 s), clamped 0..1. On alpha:

| Knob | Start (alpha 0) | End (alpha 1) |
|---|---|---|
| live aliens | `RampStartLiveAliens` 2 | `MaxLiveAliens` 6 (+1 every ~3.75 kills) |
| alien accuracy | `RampStartAlienAccuracy` 10 % | `RampEndAlienAccuracy` 35 % |
| burst interval | `RampStartBurstIntervalSeconds` 3.0 s | `RampEndBurstIntervalSeconds` 1.2 s |

Pool size stays `MaxLiveAliens`; `EnsureAlienPopulation` tracks `GetTargetLiveAliens()`. Dead bots
still self-respawn after 3 s, so the count never drops below target. HUD shows `Threat N / 5` under
the timer (amber from 3, red at 5); the log prints one line per tier change. With the ramp off the
flat DESIGN numbers (6 live, 30 %, 1.5 s) apply unchanged.

## Verification

- Self-test **35 / 35**: start shows 2 of 6 aliens at 10 % accuracy; firing while sprinting reads
  600 cm/s; restart repopulates to the target; 25 kills → ramp maxed (6 aliens, threat 5).
- `docs/sprinty_aliens_facing.png` — auto-start run, aliens turned toward the player, Threat readout.

## Knobs to try next

Too slow a ramp → lower `RampKillsToMax` (10) or `RampSecondsToMax` (100). Late game too mean →
`RampEndAlienAccuracy` 0.3 / `RampEndBurstIntervalSeconds` 1.5. Want a tougher opening →
`RampStartLiveAliens` 3.
