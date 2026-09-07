import unreal

def inspect_bp(path):
    print(f"=== Inspecting {path} ===")
    asset = unreal.EditorAssetLibrary.load_asset(path)
    if not asset:
        print("Could not load asset:", path)
        return
    print(f"Class: {asset.get_class().get_name()}")
    if hasattr(asset, 'generated_class'):
        gen_class = asset.generated_class()
        print(f"GeneratedClass: {gen_class.get_name()}")
        cdo = unreal.get_default_object(gen_class)
        print(f"CDO: {cdo}")
        for prop in ['destruction_proxy_class', 'score', 'pickup_spawn_chance', 'score_update']:
            if hasattr(cdo, prop):
                print(f"  CDO.{prop} = {getattr(cdo, prop)}")
        # Check components
        # We can check parent class
        parent = gen_class.get_super_class()
        print(f"Parent class: {parent.get_name() if parent else 'None'}")

inspect_bp('/Game/Variant_TwinStick/Blueprints/AI/BP_TwinStickNPC')
inspect_bp('/Game/Variant_TwinStick/Blueprints/AI/BP_TwinStickNPCDestruction')
inspect_bp('/Game/Variant_TwinStick/Blueprints/BP_TwinStickProjectile')
inspect_bp('/Game/Variant_TwinStick/Blueprints/BP_TwinStickGameMode')
