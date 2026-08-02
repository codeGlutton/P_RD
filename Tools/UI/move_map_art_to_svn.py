"""지도 아트 텍스처를 git 폴더에서 SVN 관리 폴더로 옮긴다.

이 프로젝트는 아트 원본을 SVN(Content/SVN 정션)에 두고, 코드/WBP는 git에 둔다.
임포트할 때는 /Game/UI/Art/RunFlow 에 넣었으므로 정본 위치로 옮기고 참조를 고친다.

rename 계열 API는 "코드/INI 를 찾아 바꿔야 할 수 있다"는 확인 창을 띄우는데
(에셋 이름이 C++ 소스에 있으면 항상 뜬다), 무인 실행에서는 Cancel 로 처리되어
이동이 실패한다. 그래서 복제 -> 참조 재지정 -> 원본 삭제로 처리한다.
"""

import unreal

SRC_DIR = "/Game/UI/Art/RunFlow"
DST_DIR = "/Game/SVN/OutSideAsset/AICreation/UI/RunFlow"

MOVE_ASSETS = [
    "T_StageMap_Scroll_Flat",
    "T_StageMap_PopupBackground",
    "T_StageMap_Legend_V2",
    "T_MapUI_ButtonPlate_V1",
    "T_MapNode_Monster_V2",
    "T_MapNode_Elite_V2",
    "T_MapNode_Boss_V2",
    "T_MapNode_Shop_V2",
    "T_MapNode_Treasure_V2",
    "T_MapNode_Rest_V2",
    "T_MapNode_RingSelected_V2",
    "T_MapPath_Solid_V2",
    "T_MapPath_Locked_V2",
]

# 더 이상 참조되지 않는 중간 산출물(원근 굽힌 지도/책상 배경/아이콘 아틀라스)
DELETE_ASSETS = [
    "T_StageMap_Scroll_Final",
    "T_StageMap_Desk_Final",
    "T_StageMap_RoomIcons_Final",
]

NODE_ICON_PROPS = {
    "m_icon_monster_texture": "T_MapNode_Monster_V2",
    "m_icon_elite_texture": "T_MapNode_Elite_V2",
    "m_icon_boss_texture": "T_MapNode_Boss_V2",
    "m_icon_shop_texture": "T_MapNode_Shop_V2",
    "m_icon_treasure_texture": "T_MapNode_Treasure_V2",
}
LINE_PROPS = {
    "m_solid_texture": "T_MapPath_Solid_V2",
    "m_dashed_texture": "T_MapPath_Locked_V2",
}


def repoint_blueprint(asset_path: str, props: dict) -> None:
    bp_class = unreal.EditorAssetLibrary.load_blueprint_class(asset_path)
    if bp_class is None:
        raise RuntimeError(f"블루프린트 없음: {asset_path}")
    cdo = unreal.get_default_object(bp_class)
    for prop, name in props.items():
        texture = unreal.load_asset(f"{DST_DIR}/{name}")
        if texture is None:
            raise RuntimeError(f"새 텍스처 로드 실패: {DST_DIR}/{name}")
        cdo.set_editor_property(prop, texture)
    if not unreal.EditorAssetLibrary.save_asset(asset_path, only_if_is_dirty=False):
        raise RuntimeError(f"저장 실패: {asset_path}")
    unreal.log(f"RD_MOVE 참조 재지정 {asset_path}")


def repoint_perspective_material() -> None:
    """M_MapPerspective 의 기본 텍스처 파라미터를 새 경로로 돌린다(머티리얼은 git 유지)."""
    material = unreal.load_asset("/Game/UI/Art/RunFlow/M_MapPerspective")
    if material is None:
        return
    new_texture = unreal.load_asset(f"{DST_DIR}/T_StageMap_Scroll_Flat")

    # 표현식 배열 접근 경로는 엔진 버전마다 달라 몇 가지를 시도한다.
    expressions = []
    for getter in (
        lambda: material.get_editor_property("expression_collection").get_editor_property("expressions"),
        lambda: material.get_editor_property("expressions"),
    ):
        try:
            expressions = list(getter() or [])
            if expressions:
                break
        except Exception as error:  # noqa: BLE001
            unreal.log(f"RD_MOVE 표현식 접근 실패: {error}")

    changed = False
    for expression in expressions:
        if isinstance(expression, unreal.MaterialExpressionTextureObjectParameter):
            expression.set_editor_property("texture", new_texture)
            changed = True
    if not changed:
        unreal.log_warning("RD_MOVE M_MapPerspective 텍스처 파라미터를 찾지 못했다")
    if changed:
        unreal.MaterialEditingLibrary.recompile_material(material)
        unreal.EditorAssetLibrary.save_loaded_asset(material, only_if_is_dirty=False)
        unreal.log("RD_MOVE 참조 재지정 M_MapPerspective")


def main() -> None:
    if not unreal.EditorAssetLibrary.does_directory_exist(DST_DIR):
        unreal.EditorAssetLibrary.make_directory(DST_DIR)

    # 1) 복제
    for name in MOVE_ASSETS:
        src = f"{SRC_DIR}/{name}"
        dst = f"{DST_DIR}/{name}"
        if unreal.EditorAssetLibrary.does_asset_exist(dst):
            unreal.log(f"RD_MOVE 이미 있음 {dst}")
            continue
        if not unreal.EditorAssetLibrary.does_asset_exist(src):
            raise RuntimeError(f"원본 없음: {src}")
        if unreal.EditorAssetLibrary.duplicate_asset(src, dst) is None:
            raise RuntimeError(f"복제 실패: {src} -> {dst}")
        unreal.log(f"RD_MOVE 복제 {src} -> {dst}")
    unreal.EditorAssetLibrary.save_directory(DST_DIR, only_if_is_dirty=False, recursive=False)

    # 2) 참조 재지정(C++ 경로는 소스에서 이미 바꿨다)
    repoint_blueprint("/Game/UI/WBP_FrontendMapNode", NODE_ICON_PROPS)
    repoint_blueprint("/Game/UI/WBP_FrontendMapLine", LINE_PROPS)
    repoint_perspective_material()

    # 3) 원본 삭제 — 남은 참조가 있으면 지우지 않고 알린다.
    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    for name in MOVE_ASSETS + DELETE_ASSETS:
        src = f"{SRC_DIR}/{name}"
        if not unreal.EditorAssetLibrary.does_asset_exist(src):
            continue
        referencers = registry.get_referencers(
            unreal.Name(f"{SRC_DIR}/{name}"), unreal.AssetRegistryDependencyOptions()
        ) or []
        if referencers:
            unreal.log_warning(
                f"RD_MOVE 참조가 남아 삭제 보류 {src}: {[str(r) for r in referencers]}"
            )
            continue
        if not unreal.EditorAssetLibrary.delete_asset(src):
            raise RuntimeError(f"삭제 실패: {src}")
        unreal.log(f"RD_MOVE 삭제 {src}")

    unreal.EditorAssetLibrary.save_directory("/Game/UI", only_if_is_dirty=True, recursive=True)
    unreal.log("RD_MOVE done")


main()
