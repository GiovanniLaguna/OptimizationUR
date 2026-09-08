import unreal

eal = unreal.EditorAssetLibrary

def check_map(map_path):
    print(f"=== Checking Map: {map_path} ===")
    world = unreal.EditorLevelLibrary.load_level(map_path)
    if not world:
        print(f"Failed to load {map_path}")
        return
    editor_world = unreal.EditorLevelLibrary.get_editor_world()
    ws = editor_world.get_world_settings()
    gm = ws.get_editor_property("default_game_mode")
    print(f"World: {editor_world.get_name()}, Default GameMode: {gm}")

check_map("/Game/Variant_WildGuns/LVL_WildGuns_MainMenu")
check_map("/Game/Variant_WildGuns/LVL_WildGuns")
