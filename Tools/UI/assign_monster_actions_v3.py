"""Assign personality-first V3 action art to every gameplay enemy role."""

import unreal


KK = "/Game/SVN/OutSideAsset/UI/KayKit"
ASSIGNMENTS = {
    "/Game/BP/DataAsset/Unit/EnemyUnit/DA_TestEnemyUnit":
        KK + "/KK_Face_Enemy_Necromancer_ActionV3",
    "/Game/BP/DataAsset/Unit/EnemyUnit/Stage1/DA_EagleUnit":
        KK + "/KK_Face_Enemy_Eagle_ActionV3",
    "/Game/BP/DataAsset/Unit/EnemyUnit/Stage1/DA_GolemUnit":
        KK + "/KK_Face_Enemy_Golem_ActionV3",
    "/Game/BP/DataAsset/Unit/EnemyUnit/Stage1/DA_LeshyUnit":
        KK + "/KK_Face_Enemy_Leshy_ActionV3",
    "/Game/BP/DataAsset/Unit/EnemyUnit/Stage1/DA_MushroomUnit":
        KK + "/KK_Face_Enemy_Mushroom_ActionV3",
    "/Game/BP/DataAsset/Unit/EnemyUnit/Stage1/DA_SlimeUnit":
        KK + "/KK_Face_Enemy_Slime_ActionV3",
    "/Game/BP/DataAsset/Unit/EnemyUnit/Stage1/DA_SpiderUnit":
        KK + "/KK_Face_Enemy_Spider_ActionV3",
    "/Game/BP/DataAsset/Unit/EnemyUnit/Stage1/DA_WerewolfUnit":
        KK + "/KK_Face_Enemy_Werewolf_ActionV3",
    "/Game/BP/DataAsset/Unit/EnemyUnit/Stage3/DA_SkeletonGolem":
        KK + "/KK_Face_Enemy_SkeletonGolem_ActionV3",
    "/Game/BP/DataAsset/Unit/EnemyUnit/Stage3/DA_SkeletonMinionMeleeUnit":
        KK + "/KK_Face_Enemy_SkeletonMinionMelee_ActionV3",
    "/Game/BP/DataAsset/Unit/EnemyUnit/Stage3/DA_SkeletonMinionRangedUnit":
        KK + "/KK_Face_Enemy_SkeletonMinionRanged_ActionV3",
}


for data_path, texture_path in ASSIGNMENTS.items():
    data = unreal.EditorAssetLibrary.load_asset(data_path)
    texture = unreal.EditorAssetLibrary.load_asset(texture_path)
    if data is None:
        raise RuntimeError("unit data missing: " + data_path)
    if texture is None:
        raise RuntimeError("monster action texture missing: " + texture_path)
    data.modify(True)
    data.set_editor_property("m_portrait", texture)
    if not unreal.EditorAssetLibrary.save_loaded_asset(data, False):
        raise RuntimeError("monster portrait asset did not save: " + data_path)
    assigned = data.get_editor_property("m_portrait")
    if assigned is None or assigned.get_path_name().split(".", 1)[0] != texture_path:
        raise RuntimeError("monster portrait assignment did not stick: " + data_path)

unreal.log(
    "[KayKit Monster Actions V3] assigned {} gameplay enemies".format(
        len(ASSIGNMENTS)
    )
)
