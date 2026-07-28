# -*- coding: utf-8 -*-
"""전투 게임모드가 쓸 HUD 를 정한다.

배치안 스무 개를 하나씩 돌려보던 동안에는 이 설정을 커밋하면 안 됐다. 어느
것을 쓸지 정해지지 않았는데 하나를 골라 두면 남이 받았을 때 이유 없이 그
화면이 뜬다.

이제 시안4 로 정해졌으므로 커밋한다. 다른 것을 잠깐 보고 싶으면 LAYOUT 만
바꿔 돌리고, 되돌릴 때는:
    git checkout -- Content/BP/GameMode/BP_CombatGameMode.uasset
"""
import unreal

GAME_MODE = "/Game/BP/GameMode/BP_CombatGameMode"
LAYOUT = "/Game/UI/CombatLayouts/WBP_CombatHUD04"

blueprint = unreal.EditorAssetLibrary.load_asset(GAME_MODE)
if blueprint is None:
    raise RuntimeError("게임모드를 못 찾음: {}".format(GAME_MODE))

layout = unreal.EditorAssetLibrary.load_asset(LAYOUT)
if layout is None:
    raise RuntimeError("배치안을 못 찾음: {}".format(LAYOUT))
layout_class = layout.generated_class()

cdo = unreal.get_default_object(blueprint.generated_class())
before = cdo.get_editor_property("mHUDClass")
unreal.log("[HUD] before = {}".format(before.get_name() if before else "None"))

cdo.set_editor_property("mHUDClass", layout_class)
unreal.EditorAssetLibrary.save_loaded_asset(blueprint, False)

# Read back from disk: a default that did not stick looks identical to one that
# did until the game runs.
reloaded = unreal.EditorAssetLibrary.load_asset(GAME_MODE)
after = unreal.get_default_object(
    reloaded.generated_class()).get_editor_property("mHUDClass")
unreal.log("[HUD] after  = {}".format(after.get_name() if after else "None"))
if after is None or after.get_name() != layout_class.get_name():
    raise RuntimeError("mHUDClass가 저장되지 않았다")
unreal.log("[HUD] done")
