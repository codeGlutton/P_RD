# -*- coding: utf-8 -*-
"""흩어진 UI 화면을 Content/UI 한 곳으로 모은다.

## 왜

같은 화면이 두 폴더에 하나씩 있었다. Content/BP/UI 는 6월에 만든 것들이고
Content/UI 는 그 뒤에 만든 것들인데, 언리얼은 둘을 다르게 취급하지 않는다.
규칙이 갈린 게 아니라 어느 날부터 새 화면을 옆에 만들기 시작해 굳은 것이다.

증거는 캐릭터 선택 화면이다. BP/UI 에 옛 것이, UI/ClassSelect 에 새 것이
있어 폴더만 보고는 어느 쪽이 뜨는지 알 수 없었다.

## 어떻게

rename_asset 이 자리를 옮기고 쪽지(리다이렉터)를 남긴다. 남은 쪽지는
fixup_referencers 가 참조하는 자산을 직접 고쳐 쓰고 치운다 -- 쪽지를 남겨
두면 Content/BP/UI 가 빈 껍데기로 계속 남는다.

코드와 ini 에 글자로 박힌 주소는 이게 못 고친다. 쪽지를 따라가므로 돌기는
하지만 옛 주소가 남아 다음 사람이 헷갈린다. 손으로 고친다.

## 안 옮기는 것

참조가 어디에도 없는 넉 장은 두고 간다. 죽은 것까지 끌고 가면 정리한 게
아니라 옮긴 것뿐이다. 다만 "참조가 없다"가 "안 쓴다"와 같지는 않아서,
지우는 것은 따로 판단할 일이다.

    python (RunEditorPython) move_ui_assets.py
"""
import unreal

SRC = "/Game/BP/UI"

#: 어디로 보낼까. 화면이 하는 일로 묶었다.
MOVES = {
    "Frontend": (
        "WBP_TitleMenu",
        "WBP_FrontendMap",
        "WBP_FrontendMapLine",
        "WBP_FrontendMapNode",
    ),
    "ClassSelect": (
        "WBP_CharacterCard",
        "WBP_CharacterCard_Knight",
        "WBP_CharacterCard_Mage",
        "WBP_CharacterCard_Rogue",
    ),
    "Reward": (
        "WBP_Reward",
        "WBP_RewardRow",
    ),
    "Common": (
        "WBP_FadeInOut",
        "WBP_SettingsPanel",
        "WBP_SkillPanel",
    ),
}

#: 부르는 데가 없어 두고 가는 것들. 지우는 것은 따로 판단한다.
KEEP = (
    "WBP_CombatActionPanel",
    "WBP_Inventory",
    "WBP_LoadingNotify",
    "WBP_CharacterSelect",
)

assets = unreal.EditorAssetLibrary
moved = []

for folder, names in MOVES.items():
    for name in names:
        src = "{}/{}".format(SRC, name)
        dst = "/Game/UI/{}/{}".format(folder, name)
        if not assets.does_asset_exist(src):
            unreal.log_warning("[UI] {} 없음 -- 건너뜀".format(src))
            continue
        if assets.does_asset_exist(dst):
            unreal.log_warning("[UI] {} 이미 있음 -- 건너뜀".format(dst))
            continue
        if not assets.rename_asset(src, dst):
            raise RuntimeError("옮기기 실패: {} -> {}".format(src, dst))
        moved.append(dst)
        unreal.log("[UI] {} -> {}".format(name, folder))

# 남은 쪽지를 치운다. 참조하는 자산을 직접 고쳐 쓰고 쪽지를 지운다.
registry = unreal.AssetRegistryHelpers.get_asset_registry()
registry.scan_paths_synchronous([SRC], True)
stubs = []
for data in registry.get_assets_by_path(SRC, recursive=True):
    if data.asset_class_path.asset_name == "ObjectRedirector":
        stubs.append(data.get_asset())

if stubs:
    unreal.AssetToolsHelpers.get_asset_tools().fixup_referencers(stubs, False, True)
    unreal.log("[UI] 쪽지 {}장 치움".format(len(stubs)))

assets.save_directory("/Game/UI", False, True)
unreal.log("[UI] {}장 옮김. 두고 온 것: {}".format(len(moved), ", ".join(KEEP)))
