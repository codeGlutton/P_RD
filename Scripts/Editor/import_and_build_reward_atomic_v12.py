"""Import validated Reward Atomic V12 parts, rebuild the WBP, and verify it."""

import os
import unreal


PROJECT_DIR = unreal.Paths.project_dir()
SOURCE_DIR = os.path.join(
    PROJECT_DIR, "Saved", "DesignAssets", "RewardAtomicV12", "GeneratedParts"
)
DESTINATION = "/Game/UI/ResultBoards/GeneratedArt/AtomicV12"
PARTS = {
    "reward_v12_exp_row_plate_1054x132.png": "T_R12_ExpRowPlate",
    "reward_v12_exp_portrait_ring_116x116.png": "T_R12_ExpPortraitRing",
    "reward_v12_exp_level_window_96x58.png": "T_R12_ExpLevelWindow",
    "reward_v12_exp_progress_track_516x22.png": "T_R12_ExpProgressTrack",
    "reward_v12_exp_xp_badge_120x64.png": "T_R12_ExpXpBadge",
    "reward_v12_gold_panel_plate_800x228.png": "T_R12_GoldPanelPlate",
    "reward_v12_gold_coin_ring_168x168.png": "T_R12_GoldCoinRing",
    "reward_v12_gold_amount_window_300x160.png": "T_R12_GoldAmountWindow",
    "reward_v12_card_frame_240x230.png": "T_R12_CardFrame",
    "reward_v12_card_name_plate_240x70.png": "T_R12_CardNamePlate",
    "reward_v12_card_selected_overlay_240x300.png": "T_R12_CardSelectedOverlay",
}


tasks = []
for filename, asset_name in PARTS.items():
    source = os.path.join(SOURCE_DIR, filename)
    if not os.path.isfile(source):
        raise RuntimeError("Missing validated V12 reward part: " + source)
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
        raise RuntimeError("V12 reward part import failed: " + object_path)
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

unreal.log(f"RD_REWARD_ATOMIC_V12_IMPORT success count={len(PARTS)}")
for command in (
    "RD.Editor.BuildRewardSettlement",
    "RD.Editor.VerifyRewardSettlement",
):
    unreal.log("RD_REWARD_ATOMIC_V12 execute " + command)
    unreal.SystemLibrary.execute_console_command(None, command)

unreal.SystemLibrary.execute_console_command(None, "QUIT_EDITOR")
