"""Assign V2 KayKit portraits to gameplay unit spawn data assets."""

import unreal


KK = "/Game/SVN/OutSideAsset/UI/KayKit"
ASSIGNMENTS = {
    "/Game/BP/DataAsset/Unit/EnemyUnit/DA_TestEnemyUnit":
        KK + "/KK_Face_Enemy_Necromancer_DynamicV2",
    "/Game/BP/DataAsset/Unit/EnemyUnit/Stage1/DA_EagleUnit":
        KK + "/KK_Face_Enemy_Eagle_DynamicV2",
    "/Game/BP/DataAsset/Unit/EnemyUnit/Stage1/DA_GolemUnit":
        KK + "/KK_Face_Enemy_Golem_DynamicV2",
    "/Game/BP/DataAsset/Unit/EnemyUnit/Stage1/DA_LeshyUnit":
        KK + "/KK_Face_Enemy_Leshy_DynamicV2",
    "/Game/BP/DataAsset/Unit/EnemyUnit/Stage1/DA_MushroomUnit":
        KK + "/KK_Face_Enemy_Mushroom_DynamicV2",
    "/Game/BP/DataAsset/Unit/EnemyUnit/Stage1/DA_SlimeUnit":
        KK + "/KK_Face_Enemy_Slime_DynamicV2",
    "/Game/BP/DataAsset/Unit/EnemyUnit/Stage1/DA_SpiderUnit":
        KK + "/KK_Face_Enemy_Spider_DynamicV2",
    "/Game/BP/DataAsset/Unit/EnemyUnit/Stage1/DA_WerewolfUnit":
        KK + "/KK_Face_Enemy_Werewolf_DynamicV2",
    "/Game/BP/DataAsset/Unit/EnemyUnit/Stage3/DA_SkeletonGolem":
        KK + "/KK_Face_Enemy_SkeletonGolem_DynamicV2",
    "/Game/BP/DataAsset/Unit/EnemyUnit/Stage3/DA_SkeletonMinionMeleeUnit":
        KK + "/KK_Face_Enemy_SkeletonMinionMelee_DynamicV2",
    "/Game/BP/DataAsset/Unit/EnemyUnit/Stage3/DA_SkeletonMinionRangedUnit":
        KK + "/KK_Face_Enemy_SkeletonMinionRanged_DynamicV2",
    # Keep the currently running test player last: an open game process may
    # lock this one package, but that must not prevent all enemy assignments.
    "/Game/BP/DataAsset/Unit/PlayerUnit/DA_TestPlayerUnit":
        KK + "/KK_Face_Knight_DynamicV2",
}


for data_path, texture_path in ASSIGNMENTS.items():
    data = unreal.EditorAssetLibrary.load_asset(data_path)
    texture = unreal.EditorAssetLibrary.load_asset(texture_path)
    if data is None:
        raise RuntimeError("unit data missing: " + data_path)
    if texture is None:
        raise RuntimeError("portrait texture missing: " + texture_path)
    data.modify(True)
    data.set_editor_property("m_portrait", texture)
    saved = unreal.EditorAssetLibrary.save_loaded_asset(data, False)
    if not saved:
        raise RuntimeError("portrait asset did not save: " + data_path)
    assigned = data.get_editor_property("m_portrait")
    if assigned is None or assigned.get_path_name().split(".", 1)[0] != texture_path:
        raise RuntimeError("portrait assignment did not stick: " + data_path)

unreal.log(
    "[KayKit Dynamic Portraits V2] assigned {} gameplay unit portraits".format(
        len(ASSIGNMENTS)
    )
)
