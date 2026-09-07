import unreal

spawner_bp = unreal.EditorAssetLibrary.load_asset('/Game/Variant_TwinStick/Blueprints/AI/BP_TwinStickSpawner')
if spawner_bp:
    gen_cls = spawner_bp.generated_class()
    cdo = unreal.get_default_object(gen_cls)
    print("BP_TwinStickSpawner CDO:", cdo)
    for prop in ['actor_template', 'actor_class', 'spawn_class', 'npc_class']:
        if hasattr(cdo, prop):
            print(f"  {prop}: {getattr(cdo, prop)}")
    # Check components
    scs = spawner_bp.get_editor_property('simple_construction_script') if hasattr(spawner_bp, 'simple_construction_script') else None
    # Let's check UActorPool on spawner
    for prop in dir(cdo):
        val = getattr(cdo, prop)
        if isinstance(val, unreal.ActorPool):
            print(f"  Found ActorPool component: {prop} with actor_template: {val.get_editor_property('actor_template')}")
