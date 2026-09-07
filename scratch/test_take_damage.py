import unreal

world = unreal.EditorLevelLibrary.get_editor_world()
walker_cls = unreal.load_class(None, "/Script/Project_URO.WildGunsWalkerNPC")
if walker_cls and world:
    spawn_loc = unreal.Vector(1000.0, 0.0, 100.0)
    actor = unreal.EditorLevelLibrary.spawn_actor_from_class(walker_cls, spawn_loc)
    if actor:
        print("Spawned test Walker NPC:", actor.get_name())
        # Apply damage via GameplayStatics
        dmg_dealt = unreal.GameplayStatics.apply_damage(actor, 1.0, None, None, None)
        print("Damage dealt via GameplayStatics:", dmg_dealt)
        print("Is actor hidden?:", actor.is_hidden_ed())
        # Clean up test actor
        unreal.EditorLevelLibrary.destroy_actor(actor)
        print("Test actor successfully tested and destroyed.")
