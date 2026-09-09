# Night Shift — Floor 37 | Sprint AB: parameterised alien material + physics asset for SK_Mutant.
#   UnrealEditor-Cmd <proj>.uproject -run=pythonscript -script=Scripts/mutant_material_physics.py -unattended -nop4 -nosplash
# Creates /Game/Imported/Aliens/Mutant/M_AlienVariant (Diffuse × Tint → BaseColor, Normal, EmissiveColor × EmissiveStrength)
# with the Mixamo textures as defaults, assigns it to SK_Mutant slot 0, and generates PHYS_SK_Mutant if missing.
import unreal
folder = "/Game/Imported/Aliens/Mutant"
tools = unreal.AssetToolsHelpers.get_asset_tools()
lib = unreal.MaterialEditingLibrary
mesh = unreal.load_asset(folder + "/SK_Mutant")
diffuse = unreal.load_asset(folder + "/Mutant_diffuse")
normal = unreal.load_asset(folder + "/Mutant_normal")
if not mesh or not diffuse:
    unreal.log_error("NS_MAT: SK_Mutant or Mutant_diffuse missing"); raise SystemExit(1)

mat_path = folder + "/M_AlienVariant"
mat = unreal.load_asset(mat_path)
if not mat:
    mat = tools.create_asset("M_AlienVariant", folder, unreal.Material, unreal.MaterialFactoryNew())
    tex = lib.create_material_expression(mat, unreal.MaterialExpressionTextureSampleParameter2D, -700, -100)
    tex.set_editor_property("parameter_name", "Diffuse")
    tex.set_editor_property("texture", diffuse)
    tint = lib.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -700, 200)
    tint.set_editor_property("parameter_name", "Tint")
    tint.set_editor_property("default_value", unreal.LinearColor(1, 1, 1, 1))
    mul = lib.create_material_expression(mat, unreal.MaterialExpressionMultiply, -350, 0)
    lib.connect_material_expressions(tex, "RGB", mul, "A")
    lib.connect_material_expressions(tint, "", mul, "B")
    lib.connect_material_property(mul, "", unreal.MaterialProperty.MP_BASE_COLOR)
    if normal:
        nrm = lib.create_material_expression(mat, unreal.MaterialExpressionTextureSampleParameter2D, -700, 450)
        nrm.set_editor_property("parameter_name", "Normal")
        nrm.set_editor_property("texture", normal)
        nrm.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
        lib.connect_material_property(nrm, "RGB", unreal.MaterialProperty.MP_NORMAL)
    ecol = lib.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -700, 700)
    ecol.set_editor_property("parameter_name", "EmissiveColor")
    ecol.set_editor_property("default_value", unreal.LinearColor(0, 0, 0, 1))
    estr = lib.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -700, 900)
    estr.set_editor_property("parameter_name", "EmissiveStrength")
    estr.set_editor_property("default_value", 0.0)
    emul = lib.create_material_expression(mat, unreal.MaterialExpressionMultiply, -350, 750)
    lib.connect_material_expressions(ecol, "", emul, "A")
    lib.connect_material_expressions(estr, "", emul, "B")
    lib.connect_material_property(emul, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    rough = lib.create_material_expression(mat, unreal.MaterialExpressionConstant, -350, 350)
    rough.set_editor_property("r", 0.65)
    lib.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    lib.recompile_material(mat)
    unreal.EditorAssetLibrary.save_loaded_asset(mat)
    unreal.log("NS_MAT: created %s" % mat_path)
else:
    unreal.log("NS_MAT: exists %s" % mat_path)

mats = list(mesh.materials)
new_mats = []
for m in mats:
    new_mats.append(unreal.SkeletalMaterial(material_interface=mat, material_slot_name=m.material_slot_name))
mesh.set_editor_property("materials", new_mats)
unreal.log("NS_MAT: SK_Mutant slots=%d now %s" % (len(new_mats), mat.get_name()))

pa = mesh.get_editor_property("physics_asset") or unreal.load_asset(folder + "/SK_Mutant_PhysicsAsset")
if not pa:
    sub = unreal.get_editor_subsystem(unreal.SkeletalMeshEditorSubsystem)
    pa = sub.create_physics_asset(mesh)
if pa:
    mesh.set_editor_property("physics_asset", pa)
    unreal.EditorAssetLibrary.save_loaded_asset(pa)
unreal.log("NS_PHYS: physics_asset=%s" % (pa.get_name() if pa else None))
unreal.EditorAssetLibrary.save_loaded_asset(mesh)
check = unreal.load_asset(folder + "/SK_Mutant")
unreal.log("NS_CHECK: material=%s physics=%s" % (check.materials[0].material_interface.get_name(), check.get_editor_property("physics_asset").get_name() if check.get_editor_property("physics_asset") else None))
