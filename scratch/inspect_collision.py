import unreal

walker_class = unreal.load_class(None, "/Script/Project_URO.WildGunsWalkerNPC")
if walker_class:
    print("Found WildGunsWalkerNPC class:", walker_class)
    cdo = unreal.get_default_object(walker_class)
    capsule = cdo.get_editor_property("capsule_component")
    if capsule:
        print("Capsule profile name:", capsule.get_collision_profile_name())
        print("Capsule collision enabled:", capsule.get_collision_enabled())
        print("Capsule response to Visibility:", capsule.get_collision_response_to_channel(unreal.CollisionChannel.ECC_VISIBILITY))
        print("Capsule response to Camera:", capsule.get_collision_response_to_channel(unreal.CollisionChannel.ECC_CAMERA))
        print("Capsule response to Pawn:", capsule.get_collision_response_to_channel(unreal.CollisionChannel.ECC_PAWN))
mesh = cdo.get_editor_property("mesh")
if mesh:
    print("Mesh profile name:", mesh.get_collision_profile_name())
    print("Mesh collision enabled:", mesh.get_collision_enabled())
    print("Mesh response to Visibility:", mesh.get_collision_response_to_channel(unreal.CollisionChannel.ECC_VISIBILITY))
