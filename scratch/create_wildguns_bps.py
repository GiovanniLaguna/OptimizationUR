import unreal

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
factory = unreal.BlueprintFactory()

def create_blueprint(parent_class_path, package_path, asset_name):
    full_path = f"{package_path}/{asset_name}"
    if unreal.EditorAssetLibrary.does_asset_exist(full_path):
        print(f"Asset already exists: {full_path}")
        return unreal.EditorAssetLibrary.load_asset(full_path)
    
    parent_cls = unreal.load_class(None, parent_class_path)
    if not parent_cls:
        print(f"Failed to find parent class: {parent_class_path}")
        return None
    
    factory.set_editor_property("parent_class", parent_cls)
    bp = asset_tools.create_asset(asset_name, package_path, unreal.Blueprint, factory)
    if bp:
        print(f"Successfully created Blueprint: {full_path}")
        unreal.EditorAssetLibrary.save_asset(full_path)
    return bp

# Create directory
package_dir = "/Game/Variant_WildGuns/Blueprints"
unreal.EditorAssetLibrary.make_directory(package_dir)

bp_walker = create_blueprint("/Script/Project_URO.WildGunsWalkerNPC", package_dir, "BP_WildGunsWalker")
bp_cover = create_blueprint("/Script/Project_URO.WildGunsCoverNPC", package_dir, "BP_WildGunsCover")
bp_fg = create_blueprint("/Script/Project_URO.WildGunsForegroundNPC", package_dir, "BP_WildGunsForeground")
bp_gm = create_blueprint("/Script/Project_URO.WildGunsGameMode", package_dir, "BP_WildGunsGameMode")

# Test validation
print("Finished creating Blueprints.")
