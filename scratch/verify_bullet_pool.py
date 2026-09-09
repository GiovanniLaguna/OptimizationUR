import unreal

print("=== VERIFYING PLAYER PROJECTILE POOL IN WILDGUNS ===")

# Verify WildGunsGameMode CDO
gm_cls = unreal.load_class(None, "/Script/Project_URO.WildGunsGameMode")
if gm_cls:
    gm_cdo = unreal.get_default_object(gm_cls)
    player_pool = gm_cdo.get_editor_property('player_projectile_pool')
    shotgun_pool = gm_cdo.get_editor_property('shotgun_projectile_pool')
    enemy_pool = gm_cdo.get_editor_property('enemy_projectile_pool')
    print("WildGunsGameMode CDO loaded successfully")
    print(f"  PlayerProjectilePool: {player_pool.get_name() if player_pool else 'None'}")
    print(f"    defaultSize: {player_pool.get_editor_property('default_size') if player_pool else 'None'}")
    print(f"  ShotgunProjectilePool: {shotgun_pool.get_name() if shotgun_pool else 'None'}")
    print(f"  EnemyProjectilePool: {enemy_pool.get_name() if enemy_pool else 'None'}")
else:
    print("Failed to load WildGunsGameMode class")

# Verify BP_WildGunsGameMode Blueprint if present
bp_gm = unreal.EditorAssetLibrary.load_asset('/Game/Variant_WildGuns/Blueprints/BP_WildGunsGameMode')
if bp_gm:
    gen_cls = bp_gm.generated_class()
    bp_cdo = unreal.get_default_object(gen_cls)
    bp_player_pool = bp_cdo.get_editor_property('player_projectile_pool')
    print("BP_WildGunsGameMode CDO loaded successfully")
    print(f"  BP PlayerProjectilePool: {bp_player_pool.get_name() if bp_player_pool else 'None'}")
    print(f"    defaultSize: {bp_player_pool.get_editor_property('default_size') if bp_player_pool else 'None'}")

print("=== VERIFYING BULLET POOL COMPLETE ===")
