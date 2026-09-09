import unreal

bp_path = '/Game/Variant_TwinStick/Blueprints/AI/BP_TwinStickNPC'
bp = unreal.EditorAssetLibrary.load_asset(bp_path)
if bp:
    gen_cls = bp.generated_class()
    cdo = unreal.get_default_object(gen_cls)
    mesh = cdo.get_editor_property('mesh')
    
    manny = unreal.EditorAssetLibrary.load_asset('/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple')
    abp = unreal.load_class(None, '/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed.ABP_Unarmed_C')
    
    if manny and abp:
        # Update component template in blueprint simple construction script or default object
        mesh.set_editor_property('skeletal_mesh_asset', manny)
        mesh.set_editor_property('anim_class', abp)
        mesh.set_editor_property('relative_location', unreal.Vector(0.0, 0.0, -90.0))
        mesh.set_editor_property('relative_rotation', unreal.Rotator(0.0, -90.0, 0.0))
        
        unreal.EditorAssetLibrary.save_loaded_asset(bp)
        print("BP_TwinStickNPC updated and saved successfully!")
    else:
        print(f"Failed to load manny ({manny}) or abp ({abp})")
else:
    print("Failed to load BP_TwinStickNPC")
