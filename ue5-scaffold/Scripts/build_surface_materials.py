# Night Shift — Floor 37 | Sprint AC: Poly Haven CC0 textures → world-projected surface materials.
#   UnrealEditor-Cmd <proj>.uproject -run=pythonscript -script=Scripts/build_surface_materials.py -unattended -nop4 -nosplash
# Reads Content/Imported/Textures/PolyHaven/roles.json ({asset: role}), imports <asset>_{Diffuse,nor_gl,arm}.jpg
# to /Game/Imported/Surfaces/<asset>/, builds M_WorldSurface (world-position UVs: PlaneSel 0=XY 1=XZ 2=YZ,
# Tile cm per repeat, Tint, RoughnessScale) plus MI_<Role> instances, and M_DirtyGlass → MI_Glass.
import os, json, unreal
proj = unreal.Paths.project_content_dir()
src_root = os.path.join(proj, "Imported", "Textures", "PolyHaven")
roles = json.load(open(os.path.join(src_root, "roles.json")))
dest_root = "/Game/Imported/Surfaces"
tools = unreal.AssetToolsHelpers.get_asset_tools()
lib = unreal.MaterialEditingLibrary
EAL = unreal.EditorAssetLibrary
tile_cm = {"floor": 220.0, "concrete": 320.0, "wall": 300.0, "berm": 260.0, "metal": 160.0}

def import_tex(path, dest, name, kind):
    full = "%s/%s" % (dest, name)
    tex = unreal.load_asset(full)
    if not tex:
        task = unreal.AssetImportTask()
        task.filename = path; task.destination_path = dest; task.destination_name = name
        task.automated = True; task.save = False; task.replace_existing = True
        tools.import_asset_tasks([task])
        tex = unreal.load_asset(full)
    if not tex:
        unreal.log_error("NS_TEX: import failed %s" % path); return None
    if kind == "N":
        tex.set_editor_property("srgb", False)
        tex.set_editor_property("flip_green_channel", True)  # nor_gl (OpenGL +Y) → UE DirectX (-Y)
        tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP)
        tex.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_WORLD_NORMAL_MAP)
    elif kind == "ARM":
        tex.set_editor_property("srgb", False)
        tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_MASKS)
    EAL.save_loaded_asset(tex)
    return tex

def expr(mat, cls, x, y, **props):
    e = lib.create_material_expression(mat, cls, x, y)
    for k, v in props.items():
        e.set_editor_property(k, v)
    return e

def connect(a, aout, b, bin_names):
    for n in (bin_names if isinstance(bin_names, (list, tuple)) else [bin_names]):
        if lib.connect_material_expressions(a, aout, b, n):
            return True
    unreal.log_warning("NS_MAT: could not connect %s -> %s (%s)" % (a.get_name(), b.get_name(), bin_names))
    return False

def build_master(def_d, def_n, def_arm):
    path = dest_root + "/M_WorldSurface"
    if unreal.load_asset(path):
        EAL.delete_asset(path)  # rebuilt each run so node fixes land; MIs are re-parented below
    mat = tools.create_asset("M_WorldSurface", dest_root, unreal.Material, unreal.MaterialFactoryNew())
    wp = expr(mat, unreal.MaterialExpressionWorldPosition, -1500, 0)
    tile = expr(mat, unreal.MaterialExpressionScalarParameter, -1500, 250, parameter_name="Tile", default_value=250.0)
    def uv(mask_r, mask_g, mask_b, y):
        m = expr(mat, unreal.MaterialExpressionComponentMask, -1250, y, r=mask_r, g=mask_g, b=mask_b, a=False)
        connect(wp, "", m, "")
        d = expr(mat, unreal.MaterialExpressionDivide, -1050, y)
        connect(m, "", d, "A"); connect(tile, "", d, "B")
        return d
    uv_xy = uv(True, True, False, -200)
    uv_xz = uv(True, False, True, 0)
    uv_yz = uv(False, True, True, 200)
    sel = expr(mat, unreal.MaterialExpressionScalarParameter, -1250, 450, parameter_name="PlaneSel", default_value=0.0)
    s1 = expr(mat, unreal.MaterialExpressionSaturate, -1050, 450); connect(sel, "", s1, "")
    one = expr(mat, unreal.MaterialExpressionConstant, -1250, 600, r=1.0)
    sub = expr(mat, unreal.MaterialExpressionSubtract, -1050, 600); connect(sel, "", sub, "A"); connect(one, "", sub, "B")
    s2 = expr(mat, unreal.MaterialExpressionSaturate, -900, 600); connect(sub, "", s2, "")
    l1 = expr(mat, unreal.MaterialExpressionLinearInterpolate, -850, 0)
    connect(uv_xy, "", l1, "A"); connect(uv_xz, "", l1, "B"); connect(s1, "", l1, "Alpha")
    l2 = expr(mat, unreal.MaterialExpressionLinearInterpolate, -650, 100)
    connect(l1, "", l2, "A"); connect(uv_yz, "", l2, "B"); connect(s2, "", l2, "Alpha")
    uv_names = ["UVs", "Coordinates"]
    diff = expr(mat, unreal.MaterialExpressionTextureSampleParameter2D, -450, -250, parameter_name="Diffuse", texture=def_d)
    connect(l2, "", diff, uv_names)
    tint = expr(mat, unreal.MaterialExpressionVectorParameter, -450, -50, parameter_name="Tint", default_value=unreal.LinearColor(1, 1, 1, 1))
    mul = expr(mat, unreal.MaterialExpressionMultiply, -200, -200)
    connect(diff, "RGB", mul, "A"); connect(tint, "", mul, "B")
    lib.connect_material_property(mul, "", unreal.MaterialProperty.MP_BASE_COLOR)
    nrm = expr(mat, unreal.MaterialExpressionTextureSampleParameter2D, -450, 150, parameter_name="Normal", texture=def_n, sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
    connect(l2, "", nrm, uv_names)
    lib.connect_material_property(nrm, "RGB", unreal.MaterialProperty.MP_NORMAL)
    arm = expr(mat, unreal.MaterialExpressionTextureSampleParameter2D, -450, 400, parameter_name="ARM", texture=def_arm, sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_MASKS)
    connect(l2, "", arm, uv_names)
    lib.connect_material_property(arm, "R", unreal.MaterialProperty.MP_AMBIENT_OCCLUSION)
    rs = expr(mat, unreal.MaterialExpressionScalarParameter, -450, 650, parameter_name="RoughnessScale", default_value=1.0)
    rmul = expr(mat, unreal.MaterialExpressionMultiply, -200, 450)
    connect(arm, "G", rmul, "A"); connect(rs, "", rmul, "B")
    lib.connect_material_property(rmul, "", unreal.MaterialProperty.MP_ROUGHNESS)
    lib.connect_material_property(arm, "B", unreal.MaterialProperty.MP_METALLIC)
    lib.recompile_material(mat)
    EAL.save_loaded_asset(mat)
    unreal.log("NS_MAT: created %s" % path)
    return mat

def build_glass():
    path = dest_root + "/M_DirtyGlass"
    if unreal.load_asset(path):
        EAL.delete_asset(path)  # rebuilt each run: cheap, and the params below changed in Sprint AC
    mat = tools.create_asset("M_DirtyGlass", dest_root, unreal.Material, unreal.MaterialFactoryNew())
    col = expr(mat, unreal.MaterialExpressionVectorParameter, -500, -100, parameter_name="Tint", default_value=unreal.LinearColor(0.012, 0.03, 0.034, 1))
    lib.connect_material_property(col, "", unreal.MaterialProperty.MP_BASE_COLOR)
    rough = expr(mat, unreal.MaterialExpressionScalarParameter, -500, 100, parameter_name="RoughnessScale", default_value=0.42)
    lib.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    met = expr(mat, unreal.MaterialExpressionScalarParameter, -500, 250, parameter_name="Metallic", default_value=0.12)
    lib.connect_material_property(met, "", unreal.MaterialProperty.MP_METALLIC)
    spec = expr(mat, unreal.MaterialExpressionConstant, -500, 400, r=0.7)
    lib.connect_material_property(spec, "", unreal.MaterialProperty.MP_SPECULAR)
    lib.recompile_material(mat); EAL.save_loaded_asset(mat)
    unreal.log("NS_MAT: created %s" % path)
    mi_path = dest_root + "/MI_Glass"
    mi = unreal.load_asset(mi_path) or tools.create_asset("MI_Glass", dest_root, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    lib.set_material_instance_parent(mi, mat)
    lib.update_material_instance(mi); EAL.save_loaded_asset(mi)
    unreal.log("NS_MI: %s" % mi_path)

def build_resin():
    path = dest_root + "/M_Resin"
    if unreal.load_asset(path):
        EAL.delete_asset(path)
    mat = tools.create_asset("M_Resin", dest_root, unreal.Material, unreal.MaterialFactoryNew())
    col = expr(mat, unreal.MaterialExpressionVectorParameter, -600, -150, parameter_name="Tint", default_value=unreal.LinearColor(0.045, 0.095, 0.035, 1))
    lib.connect_material_property(col, "", unreal.MaterialProperty.MP_BASE_COLOR)
    rough = expr(mat, unreal.MaterialExpressionScalarParameter, -600, 50, parameter_name="RoughnessScale", default_value=0.4)
    lib.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    ecol = expr(mat, unreal.MaterialExpressionVectorParameter, -600, 250, parameter_name="EmissiveColor", default_value=unreal.LinearColor(0.25, 0.9, 0.35, 1))
    estr = expr(mat, unreal.MaterialExpressionScalarParameter, -600, 450, parameter_name="EmissiveStrength", default_value=0.10)
    mul = expr(mat, unreal.MaterialExpressionMultiply, -300, 300)
    connect(ecol, "", mul, "A"); connect(estr, "", mul, "B")
    lib.connect_material_property(mul, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    spec = expr(mat, unreal.MaterialExpressionConstant, -600, 600, r=0.45)
    lib.connect_material_property(spec, "", unreal.MaterialProperty.MP_SPECULAR)
    lib.recompile_material(mat); EAL.save_loaded_asset(mat)
    unreal.log("NS_MAT: created %s" % path)

def build_neon():
    path = dest_root + "/M_Neon"
    if unreal.load_asset(path):
        unreal.log("NS_MAT: exists %s" % path); return
    mat = tools.create_asset("M_Neon", dest_root, unreal.Material, unreal.MaterialFactoryNew())
    black = expr(mat, unreal.MaterialExpressionConstant, -500, -150, r=0.02)
    lib.connect_material_property(black, "", unreal.MaterialProperty.MP_BASE_COLOR)
    ecol = expr(mat, unreal.MaterialExpressionVectorParameter, -600, 50, parameter_name="EmissiveColor", default_value=unreal.LinearColor(0.35, 1.0, 0.55, 1))
    estr = expr(mat, unreal.MaterialExpressionScalarParameter, -600, 250, parameter_name="EmissiveStrength", default_value=6.0)
    mul = expr(mat, unreal.MaterialExpressionMultiply, -300, 100)
    connect(ecol, "", mul, "A"); connect(estr, "", mul, "B")
    lib.connect_material_property(mul, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    rough = expr(mat, unreal.MaterialExpressionConstant, -500, 400, r=0.4)
    lib.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    lib.recompile_material(mat); EAL.save_loaded_asset(mat)
    unreal.log("NS_MAT: created %s" % path)

imported = {}
for asset, role in roles.items():
    folder = os.path.join(src_root, asset)
    dest = "%s/%s" % (dest_root, asset)
    imported[asset] = (
        import_tex(os.path.join(folder, "%s_Diffuse.jpg" % asset), dest, "T_%s_D" % asset, "D"),
        import_tex(os.path.join(folder, "%s_nor_gl.jpg" % asset), dest, "T_%s_N" % asset, "N"),
        import_tex(os.path.join(folder, "%s_arm.jpg" % asset), dest, "T_%s_ARM" % asset, "ARM"))
first = next(iter(imported.values()))
master = build_master(first[0], first[1], first[2])
for asset, role in roles.items():
    d, n, a = imported[asset]
    name = "MI_%s" % role.capitalize()
    mi_path = "%s/%s" % (dest_root, name)
    mi = unreal.load_asset(mi_path) or tools.create_asset(name, dest_root, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    lib.set_material_instance_parent(mi, master)
    if d: lib.set_material_instance_texture_parameter_value(mi, "Diffuse", d)
    if n: lib.set_material_instance_texture_parameter_value(mi, "Normal", n)
    if a: lib.set_material_instance_texture_parameter_value(mi, "ARM", a)
    lib.set_material_instance_scalar_parameter_value(mi, "Tile", tile_cm.get(role, 250.0))
    if role == "floor":
        lib.set_material_instance_scalar_parameter_value(mi, "RoughnessScale", 0.55)  # wet tile
    lib.update_material_instance(mi)
    EAL.save_loaded_asset(mi)
    unreal.log("NS_MI: %s <- %s (tile %.0f)" % (mi_path, asset, tile_cm.get(role, 250.0)))
build_glass()
build_neon()
build_resin()
