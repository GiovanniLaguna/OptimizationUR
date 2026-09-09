import unreal

print("=== VERIFYING LOCOMOTION MESHES & ANIMATION BLUEPRINTS ===")

classes_to_check = [
    ('/Game/Variant_WildGuns/Blueprints/BP_WildGunsWalker', 'BP_WildGunsWalker'),
    ('/Game/Variant_WildGuns/Blueprints/BP_WildGunsCover', 'BP_WildGunsCover'),
    ('/Game/Variant_WildGuns/Blueprints/BP_WildGunsForeground', 'BP_WildGunsForeground'),
    ('/Game/Variant_TwinStick/Blueprints/AI/BP_TwinStickNPC', 'BP_TwinStickNPC'),
    ('/Game/Variant_TwinStick/Blueprints/BP_TwinStickCharacter', 'BP_TwinStickCharacter')
]

for asset_path, name in classes_to_check:
    bp = unreal.EditorAssetLibrary.load_asset(asset_path)
    if bp:
        gen_cls = bp.generated_class()
        cdo = unreal.get_default_object(gen_cls)
        mesh = cdo.get_editor_property('mesh')
        sk_mesh = mesh.get_editor_property('skeletal_mesh_asset') if mesh else None
        anim_cls = mesh.get_editor_property('anim_class') if mesh else None
        print(f"[{name}]")
        print(f"  Mesh: {sk_mesh.get_name() if sk_mesh else 'None'}")
        print(f"  AnimClass: {anim_cls.get_name() if anim_cls else 'None'}")
    else:
        print(f"[{name}] Failed to load asset: {asset_path}")

# Check C++ native classes
for cpp_name in ['WildGunsCharacter', 'WildGunsEnemyBase', 'WildGunsWalkerNPC', 'WildGunsCoverNPC', 'WildGunsForegroundNPC']:
    cls = unreal.load_class(None, f"/Script/Project_URO.{cpp_name}")
    if cls:
        cdo = unreal.get_default_object(cls)
        mesh = cdo.get_editor_property('mesh')
        sk_mesh = mesh.get_editor_property('skeletal_mesh_asset') if mesh else None
        anim_cls = mesh.get_editor_property('anim_class') if mesh else None
        print(f"[C++ {cpp_name}]")
        print(f"  Mesh: {sk_mesh.get_name() if sk_mesh else 'None'}")
        print(f"  AnimClass: {anim_cls.get_name() if anim_cls else 'None'}")
    else:
        print(f"[C++ {cpp_name}] Could not find class")

print("=== VERIFICATION SCRIPT FINISHED ===")
