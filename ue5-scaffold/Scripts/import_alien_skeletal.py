# Night Shift — Floor 37 | Import Quaternius Alien.fbx as a skeletal mesh + animations.
# Run headless:
#   UnrealEditor-Cmd <proj>.uproject -run=pythonscript -script=Scripts/import_alien_skeletal.py -unattended -nop4 -nosplash
# Env: NS_ALIEN_FBX (path to Alien.fbx; Atlas_Monsters.png beside it).
import os, unreal

fbx = os.environ.get("NS_ALIEN_FBX", "")
dest = "/Game/Imported/Aliens/Skel"
if not fbx or not os.path.exists(fbx):
    unreal.log_error("NS_ALIEN_FBX not set or missing: %s" % fbx)
    raise SystemExit(1)

ui = unreal.FbxImportUI()
ui.import_mesh = True
ui.import_as_skeletal = True
ui.import_animations = True
ui.import_materials = True
ui.import_textures = True
ui.mesh_type_to_import = unreal.FBXImportType.FBXIT_SKELETAL_MESH
ui.skeletal_mesh_import_data.set_editor_property("import_morph_targets", False)
ui.skeletal_mesh_import_data.set_editor_property("convert_scene", True)
ui.skeletal_mesh_import_data.set_editor_property("normal_import_method", unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS_AND_TANGENTS)
ui.anim_sequence_import_data.set_editor_property("import_bone_tracks", True)
ui.set_editor_property("create_physics_asset", True)

task = unreal.AssetImportTask()
task.filename = fbx
task.destination_path = dest
task.destination_name = "SK_Alien"
task.automated = True
task.save = True
task.replace_existing = True
task.options = ui

unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
paths = list(task.imported_object_paths)
unreal.log("NS_IMPORT: %d objects" % len(paths))
for p in paths:
    unreal.log("NS_IMPORT: %s" % p)

# Report skeletal mesh bounds + animation names for the C++ soft paths.
reg = unreal.AssetRegistryHelpers.get_asset_registry()
reg.scan_paths_synchronous([dest], True)
for a in reg.get_assets_by_path(dest, True):
    unreal.log("NS_ASSET: %s (%s)" % (a.get_editor_property("package_name"), a.asset_class_path.asset_name))
    obj = a.get_asset()
    if isinstance(obj, unreal.SkeletalMesh):
        b = obj.get_bounds()
        unreal.log("NS_BOUNDS: origin=%s extent=%s" % (b.origin, b.box_extent))
