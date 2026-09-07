import unreal

bp = unreal.EditorAssetLibrary.load_asset('/Game/Variant_TwinStick/Blueprints/AI/BP_TwinStickNPC')
if bp:
    gen_cls = bp.generated_class()
    cdo = unreal.get_default_object(gen_cls)
    print("BP_TwinStickNPC CDO:", cdo)
    mesh = cdo.get_editor_property('mesh')
    print("Mesh:", mesh.get_editor_property('skeletal_mesh_asset'))
    anim = mesh.get_editor_property('anim_class')
    print("AnimClass:", anim)
    capsule = cdo.get_editor_property('capsule_component')
    print("Capsule radius:", capsule.get_scaled_capsule_radius(), "half_height:", capsule.get_scaled_capsule_half_height())
    print("Destruction proxy:", cdo.get_editor_property('destruction_proxy_class'))
