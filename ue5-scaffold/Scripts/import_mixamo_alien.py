# Night Shift — Floor 37 | Import a Mixamo creature (character FBX + animation FBXs) as the alien.
# Run headless:
#   NS_MIXAMO_DIR=~/Downloads/NightShiftAlien NS_MIXAMO_NAME=Mutant \
#   UnrealEditor-Cmd <proj>.uproject -run=pythonscript -script=Scripts/import_mixamo_alien.py -unattended -nop4 -nosplash
# Expects: <NAME>.fbx (T-pose, with skin) plus any number of animation FBXs ("Mutant Idle.fbx" …).
# Output: /Game/Imported/Aliens/<NAME>/SK_<NAME> + SK_<NAME>_Skeleton + one AnimSequence per clip
#         named A_<NAME>_<Clip> (spaces → underscores). The log lists NS_ANIM lines for the C++ soft paths.
import os, glob, re, unreal

src_dir = os.path.expanduser(os.environ.get("NS_MIXAMO_DIR", "~/Downloads/NightShiftAlien"))
name = os.environ.get("NS_MIXAMO_NAME", "Mutant")
dest = "/Game/Imported/Aliens/%s" % name
tools = unreal.AssetToolsHelpers.get_asset_tools()

fbxs = sorted(glob.glob(os.path.join(src_dir, "*.fbx")) + glob.glob(os.path.join(src_dir, "*.FBX")))
if not fbxs:
    unreal.log_error("NS_MIXAMO: no FBX files in %s" % src_dir)
    raise SystemExit(1)

def is_character(path):
    base = os.path.splitext(os.path.basename(path))[0].strip().lower()
    return base == name.lower() or base in ("character", "%s t-pose" % name.lower(), "%s_t-pose" % name.lower())

chars = [f for f in fbxs if is_character(f)]
if not chars:
    # Fall back to the largest FBX (skinned character files dwarf animation-only files).
    chars = [max(fbxs, key=os.path.getsize)]
char_fbx = chars[0]
anim_fbxs = [f for f in fbxs if f != char_fbx]
unreal.log("NS_MIXAMO: character=%s anims=%d" % (os.path.basename(char_fbx), len(anim_fbxs)))

def base_ui(skeleton=None):
    ui = unreal.FbxImportUI()
    ui.import_materials = skeleton is None
    ui.import_textures = skeleton is None
    ui.import_mesh = skeleton is None
    ui.import_as_skeletal = True
    ui.import_animations = skeleton is not None
    ui.mesh_type_to_import = unreal.FBXImportType.FBXIT_SKELETAL_MESH if skeleton is None else unreal.FBXImportType.FBXIT_ANIMATION
    ui.skeletal_mesh_import_data.set_editor_property("import_morph_targets", False)
    ui.skeletal_mesh_import_data.set_editor_property("convert_scene", True)
    ui.skeletal_mesh_import_data.set_editor_property("normal_import_method", unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS_AND_TANGENTS)
    ui.anim_sequence_import_data.set_editor_property("import_bone_tracks", True)
    ui.anim_sequence_import_data.set_editor_property("convert_scene", True)
    ui.set_editor_property("create_physics_asset", skeleton is None)
    if skeleton is not None:
        ui.skeleton = skeleton
    return ui

def run(filename, dest_name, ui):
    task = unreal.AssetImportTask()
    task.filename = filename
    task.destination_path = dest
    task.destination_name = dest_name
    task.automated = True
    task.save = True
    task.replace_existing = True
    task.options = ui
    tools.import_asset_tasks([task])
    return list(task.imported_object_paths)

paths = run(char_fbx, "SK_%s" % name, base_ui())
for p in paths:
    unreal.log("NS_IMPORT: %s" % p)
skel_mesh = unreal.load_asset("%s/SK_%s" % (dest, name))
if not skel_mesh:
    unreal.log_error("NS_MIXAMO: skeletal mesh import failed")
    raise SystemExit(1)
skeleton = skel_mesh.skeleton
b = skel_mesh.get_bounds()
unreal.log("NS_BOUNDS: origin=%s extent=%s" % (b.origin, b.box_extent))

for f in anim_fbxs:
    clip = os.path.splitext(os.path.basename(f))[0]
    clip = re.sub(r"(?i)^%s[\s_-]*" % re.escape(name), "", clip).strip() or clip
    clip = re.sub(r"[^A-Za-z0-9]+", "_", clip).strip("_")
    out = run(f, "A_%s_%s" % (name, clip), base_ui(skeleton))
    for p in out:
        if "AnimSequence" in p or "/A_" in p:
            unreal.log("NS_ANIM: %s <- %s" % (p, os.path.basename(f)))
