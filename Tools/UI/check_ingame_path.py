"""What does it take to see a layout in the running game?

Reports the combat game mode's current HUD class and whether the layout
classes can stand in for it, so the answer is read off the assets rather than
assumed.
"""
import unreal

GAME_MODE = "/Game/BP/GameMode/BP_CombatGameMode"
LAYOUT = "/Game/UI/CombatLayouts/WBP_CombatLayout_01_ClassicCRPG"

blueprint = unreal.EditorAssetLibrary.load_asset(GAME_MODE)
if blueprint is None:
    unreal.log_error("[InGame] {} 없음".format(GAME_MODE))
else:
    cdo = unreal.get_default_object(blueprint.generated_class())
    hud = cdo.get_editor_property("mHUDClass")
    unreal.log("[InGame] BP_CombatGameMode.mHUDClass = {}".format(
        hud.get_name() if hud else "None"))
    unreal.log("[InGame] parent = {}".format(
        blueprint.generated_class().get_super_class().get_name()))

layout = unreal.EditorAssetLibrary.load_asset(LAYOUT)
generated = layout.generated_class() if layout else None
if generated:
    unreal.log("[InGame] layout class = {} (RDUserWidget 파생={})".format(
        generated.get_name(),
        unreal.MathLibrary.class_is_child_of(generated, unreal.RDUserWidget)))
