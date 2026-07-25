"""Point the combat game mode at a layout variant so it shows up in the game.

Local only -- BP_CombatGameMode decides which HUD the real game uses, so this
must not be committed until a layout is actually chosen. Revert with:
    git checkout -- Content/BP/GameMode/BP_CombatGameMode.uasset

The class name comes from the LAYOUT constant; edit it to try another one.
"""
import unreal

GAME_MODE = "/Game/BP/GameMode/BP_CombatGameMode"
LAYOUT = "/Game/UI/CombatLayouts/WBP_CombatLayout_01_ClassicCRPG"

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
