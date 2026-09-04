# NavMesh MoveTo — Editor follow-up

Aliens ship with **simple steering** chase. Optional NavMesh path:

1. Place **Nav Mesh Bounds Volume** covering walkable floors, ramps, atrium stairs (DESIGN: tower contested).
2. Build paths (Editor → Build → Build Paths).
3. Alien BP / placed `AAlienBot`: possess with **AIController** (or set AutoPossessAI).
4. Set **`bPreferNavMeshMoveTo = true`** on the bot.
5. `TryNavMeshMoveToTarget()` calls `AAIController::MoveTo`; on failure ChasePlayer falls back to ray-steer.

If MoveTo cannot climb atrium geometry, add a simple waypoint graph (DESIGN allows it) — not required for C++ drop-in.
