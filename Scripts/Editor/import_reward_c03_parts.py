"""Import concept03 parts, then rebuild static boards and the functional runtime WBP."""

import os
import unreal


PROJECT_DIR = unreal.Paths.project_dir()
SOURCE_DIR = os.path.join(PROJECT_DIR, "Saved", "DesignAssets", "RewardC03Parts")
DESTINATION = "/Game/UI/ResultBoards/GeneratedArt/C03"
PARTS = {
    "c03_board_interior_180x320.png": "T_C03_BoardInterior",
    "c03_rail_h_110x42.png": "T_C03_RailH",
    "c03_rail_v_left_44x260.png": "T_C03_RailVLeft",
    "c03_rail_v_right_44x260.png": "T_C03_RailVRight",
    "c03_corner_tl_92x92.png": "T_C03_CornerTL",
    "c03_corner_tr_92x92.png": "T_C03_CornerTR",
    "c03_corner_bl_92x92.png": "T_C03_CornerBL",
    "c03_corner_br_92x92.png": "T_C03_CornerBR",
    "c03_title_plate_566x136.png": "T_C03_TitlePlate",
    "c03_stage_tab_196x48.png": "T_C03_StageTab",
    "c03_step_coin_active_92x92.png": "T_C03_StepCoinActive",
    "c03_step_coin_inactive_64x64.png": "T_C03_StepCoinInactive",
    "c03_step_bar_track_690x22.png": "T_C03_StepBarTrack",
    "c03_step_bar_fill_400x16.png": "T_C03_StepBarFill",
    "c03_cta_plate_400x94.png": "T_C03_CtaPlate",
    "c03_parch_window_446x286.png": "T_C03_ParchWindow",
    "c03_xp_badge_156x156.png": "T_C03_XpBadge",
    "c03_track_plate_322x44.png": "T_C03_TrackPlate",
    "c03_track_fill_300x28.png": "T_C03_TrackFill",
    "c03_card_blank_290x326.png": "T_C03_CardBlank",
    "c03_selection_glow_302x338.png": "T_C03_SelectionGlow",
    "c03_chest_visual_344x324.png": "T_C03_ChestVisual",
    "c03_gold_coin_visual_300x300.png": "T_C03_GoldCoinVisual",
    "c03_gold_coin_visual_generated_512x512.png": "T_C03_GoldCoinVisualGenerated",
    "c03_chest_visual_soft_344x324.png": "T_C03_ChestVisualSoft",
    "c03_gold_coin_visual_soft_300x300.png": "T_C03_GoldCoinVisualSoft",
}


tasks = []
for filename, asset_name in PARTS.items():
    source = os.path.join(SOURCE_DIR, filename)
    if not os.path.isfile(source):
        raise RuntimeError("Missing c03 part: " + source)
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
        raise RuntimeError("c03 part import failed: " + object_path)
    texture.set_editor_property(
        "compression_settings", unreal.TextureCompressionSettings.TC_EDITOR_ICON
    )
    texture.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
    texture.set_editor_property(
        "mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS
    )
    texture.set_editor_property("never_stream", True)
    texture.set_editor_property("srgb", True)
    texture.modify()
    unreal.EditorAssetLibrary.save_loaded_asset(texture, only_if_is_dirty=False)

unreal.log(f"RD_REWARD_C03_IMPORT success count={len(PARTS)}")
unreal.SystemLibrary.execute_console_command(None, "RD.Editor.BuildRewardC03Boards")
unreal.SystemLibrary.execute_console_command(None, "RD.Editor.BuildRewardSettlement")
unreal.SystemLibrary.execute_console_command(None, "RD.Editor.VerifyRewardSettlement")
unreal.SystemLibrary.execute_console_command(None, "QUIT_EDITOR")
