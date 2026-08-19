"""Import Reward BS parts, build BS boards, then close the editor."""
import os

import unreal


PROJECT_DIR = unreal.Paths.project_dir()
SOURCE_DIR = os.path.join(PROJECT_DIR, "Saved", "DesignAssets", "RewardBS", "GeneratedParts")
DESTINATION = "/Game/UI/ResultBoards/GeneratedArt/BS"
PARTS = {
    "bs_battle_backdrop_1536x170.png": "T_BS_BattleBackdrop",
    "bs_sheet_background_1472x694.png": "T_BS_SheetBackground",
    "bs_sheet_frame_1472x694.png": "T_BS_SheetFrame",
    "bs_title_plate_360x84.png": "T_BS_TitlePlate",
    "bs_stage_tab_160x58.png": "T_BS_StageTab",
    "bs_step_track_760x22.png": "T_BS_StepTrack",
    "bs_step_fill_744x12.png": "T_BS_StepFill",
    "bs_step_coin_active_76x76.png": "T_BS_StepCoinActive",
    "bs_step_coin_inactive_64x64.png": "T_BS_StepCoinInactive",
    "bs_cta_button_360x88.png": "T_BS_CtaButton",
    "bs_parchment_window_500x282.png": "T_BS_ParchmentWindow",
    "bs_xp_track_330x40.png": "T_BS_XpTrack",
    "bs_xp_fill_314x24.png": "T_BS_XpFill",
    "bs_card_blank_280x320.png": "T_BS_CardBlank",
    "bs_selection_glow_292x332.png": "T_BS_SelectionGlow",
    "bs_chest_burst_420x360.png": "T_BS_ChestBurst",
}

tasks = []
for filename, asset_name in PARTS.items():
    source = os.path.join(SOURCE_DIR, filename)
    if not os.path.isfile(source):
        raise RuntimeError("Missing BS part: " + source)
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", source)
    task.set_editor_property("destination_path", DESTINATION)
    task.set_editor_property("destination_name", asset_name)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", False)
    tasks.append(task)

unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
for asset_name in PARTS.values():
    object_path = f"{DESTINATION}/{asset_name}.{asset_name}"
    texture = unreal.EditorAssetLibrary.load_asset(object_path)
    if texture is None:
        raise RuntimeError("BS part import failed: " + object_path)
    texture.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_EDITOR_ICON)
    texture.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
    texture.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
    texture.set_editor_property("never_stream", True)
    texture.set_editor_property("srgb", True)
    texture.modify()
    unreal.EditorAssetLibrary.save_loaded_asset(texture, only_if_is_dirty=False)

unreal.log(f"RD_REWARD_BS_IMPORT success count={len(PARTS)}")
unreal.SystemLibrary.execute_console_command(None, "RD.Editor.BuildRewardBSBoards")
unreal.SystemLibrary.execute_console_command(None, "QUIT_EDITOR")
