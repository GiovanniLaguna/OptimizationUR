import unreal

print("=== Starting Enemy Death & Collision Verification ===")

classes_to_check = [
    ("/Script/Project_URO.WildGunsWalkerNPC", "WildGunsWalkerNPC"),
    ("/Script/Project_URO.WildGunsCoverNPC", "WildGunsCoverNPC"),
    ("/Script/Project_URO.WildGunsForegroundNPC", "WildGunsForegroundNPC"),
    ("/Script/Project_URO.TwinStickNPC", "TwinStickNPC")
]

all_passed = True

for cls_path, name in classes_to_check:
    cls = unreal.load_class(None, cls_path)
    if not cls:
        print(f"[FAIL] Could not load class: {cls_path}")
        all_passed = False
        continue
    
    cdo = unreal.get_default_object(cls)
    capsule = cdo.get_editor_property("capsule_component")
    vis_resp = capsule.get_collision_response_to_channel(unreal.CollisionChannel.ECC_VISIBILITY)
    cam_resp = capsule.get_collision_response_to_channel(unreal.CollisionChannel.ECC_CAMERA)
    
    print(f"[{name}]")
    print(f"  Capsule Visibility Response: {vis_resp}")
    print(f"  Capsule Camera Response:     {cam_resp}")
    
    if vis_resp != unreal.CollisionResponseType.ECR_BLOCK:
        print(f"  [ERROR] {name} Capsule DOES NOT BLOCK VISIBILITY!")
        all_passed = False
    else:
        print(f"  [OK] {name} Capsule Blocks Visibility!")

    if hasattr(cdo, "destruction_proxy_class"):
        proxy = cdo.get_editor_property("destruction_proxy_class")
        print(f"  DestructionProxyClass: {proxy}")
        if not proxy:
            print(f"  [WARNING] {name} DestructionProxyClass is None!")
        else:
            print(f"  [OK] {name} DestructionProxyClass is set to {proxy.get_name()}!")

print("=== Verification Result:", "ALL PASSED" if all_passed else "SOME FAILED", "===")
