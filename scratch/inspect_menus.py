import unreal

def inspect_level_bp(level_path):
    print(f"=== Inspecting Level: {level_path} ===")
    unreal.EditorLevelLibrary.load_level(level_path)
    actors = unreal.EditorLevelLibrary.get_all_level_actors()
    print(f"Actors count: {len(actors)}")
    for a in actors:
        print(f"Actor: {a.get_name()} ({a.get_class().get_name()})")

inspect_level_bp("/Game/Variant_WildGuns/LVL_WildGuns_MainMenu")
inspect_level_bp("/Game/Variant_TwinStick/LVL_MainMenu")
