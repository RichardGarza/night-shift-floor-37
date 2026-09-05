"""Generate Content/Maps/Floor37.umap headlessly.

Run from the project folder:
  "/Users/Shared/Epic Games/UE_5.8/Engine/Binaries/Mac/UnrealEditor-Cmd" NightShiftFloor37.uproject \
      -run=pythonscript -script=Scripts/make_floor37_map.py -unattended -nop4 -nosplash

The map only needs the three actors below; AArenaGameMode would spawn them anyway if missing.
AOfficeArena builds the greybox floor, tower, cover, and lighting from C++.
"""
import unreal

MAP_PATH = "/Game/Maps/Floor37"

level_sys = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actor_sys = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

if not level_sys.new_level(MAP_PATH):
    raise SystemExit("new_level failed for %s" % MAP_PATH)

arena = actor_sys.spawn_actor_from_class(unreal.OfficeArena, unreal.Vector(0, 0, 0))
arena.set_actor_label("OfficeArena")
pool = actor_sys.spawn_actor_from_class(unreal.FXPoolManager, unreal.Vector(0, 0, 100))
pool.set_actor_label("FXPoolManager")
start = actor_sys.spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(-1000, 0, 120), unreal.Rotator(0, 0, 0))
start.set_actor_label("PlayerStart")

if not level_sys.save_current_level():
    raise SystemExit("save_current_level failed")
unreal.log("Floor37 map saved with OfficeArena, FXPoolManager, PlayerStart")
