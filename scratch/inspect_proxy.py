import unreal

bp = unreal.EditorAssetLibrary.load_asset('/Game/Variant_TwinStick/Blueprints/AI/BP_TwinStickNPCDestruction')
if bp:
    gen_cls = bp.generated_class()
    cdo = unreal.get_default_object(gen_cls)
    print("BP_TwinStickNPCDestruction CDO:", cdo)
    # List components in blueprint
    scs = bp.get_editor_property('simple_construction_script')
    if scs:
        nodes = scs.get_all_nodes()
        for n in nodes:
            comp_temp = n.get_editor_property('component_template')
            print("  Component:", comp_temp.get_name(), comp_temp.get_class().get_name())
            if comp_temp.get_class().get_name() == 'GeometryCollectionComponent':
                print("    RestCollection:", comp_temp.get_editor_property('rest_collection'))
            if comp_temp.get_class().get_name() == 'NiagaraComponent':
                print("    Asset:", comp_temp.get_editor_property('asset'))
