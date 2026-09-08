import unreal

eal = unreal.EditorAssetLibrary

def inspect_level_actors(map_path):
    print(f"\n==========================================")
    print(f"ACTORS IN: {map_path}")
    print(f"==========================================")
    unreal.EditorLevelLibrary.load_level(map_path)
    actors = unreal.EditorLevelLibrary.get_all_level_actors()
    print(f"Total Actors: {len(actors)}")
    for a in actors:
        cls_name = a.get_class().get_name()
        if not a.get_name().startswith("Default__"):
            print(f" - [{cls_name}] {a.get_name()} (Loc: {a.get_actor_location()})")

inspect_level_actors("/Game/Variant_WildGuns/LVL_WildGuns_MainMenu")
