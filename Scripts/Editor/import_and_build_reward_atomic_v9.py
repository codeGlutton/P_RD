"""Import the V9 one-function-per-PNG reward atoms, rebuild, and verify."""

import os
import unreal


PROJECT_DIR = unreal.Paths.project_dir()
SOURCE_DIR = os.path.join(
    PROJECT_DIR,
    "Saved",
    "DesignAssets",
    "RewardAtomicV9_20260817",
    "GeneratedPartsV1",
)
DESTINATION = "/Game/UI/ResultBoards/GeneratedArt/AtomicV9"
PARTS = {
    "reward_atomic_modal_background_1536x864_v1.png": "T_RA_ModalBackground",
    "reward_atomic_modal_outer_frame_1536x864_v1.png": "T_RA_ModalOuterFrame",
    "reward_atomic_body_background_1376x518_v1.png": "T_RA_BodyBackground",
    "reward_atomic_body_frame_1376x518_v1.png": "T_RA_BodyFrame",
    "reward_atomic_header_background_720x141_v1.png": "T_RA_HeaderBackground",
    "reward_atomic_header_frame_720x141_v1.png": "T_RA_HeaderFrame",
    "reward_atomic_step_background_380x62_v1.png": "T_RA_StepBackground",
    "reward_atomic_step_frame_380x62_v1.png": "T_RA_StepFrame",
    "reward_atomic_cta_background_404x70_v1.png": "T_RA_CtaBackground",
    "reward_atomic_cta_frame_404x70_v1.png": "T_RA_CtaFrame",
    "reward_atomic_exp_rail_background_1054x132_v1.png": "T_RA_ExpRailBackground",
    "reward_atomic_exp_rail_frame_1054x132_v1.png": "T_RA_ExpRailFrame",
    "reward_atomic_exp_portrait_socket_190x132_v1.png": "T_RA_ExpPortraitSocket",
    "reward_atomic_exp_level_plate_100x76_v1.png": "T_RA_ExpLevelPlate",
    "reward_atomic_exp_progress_track_520x60_v1.png": "T_RA_ExpProgressTrack",
    "reward_atomic_exp_xp_badge_130x92_v1.png": "T_RA_ExpXpBadge",
    "reward_atomic_gold_rail_background_800x228_v1.png": "T_RA_GoldRailBackground",
    "reward_atomic_gold_rail_frame_800x228_v1.png": "T_RA_GoldRailFrame",
    "reward_atomic_gold_coin_socket_180x180_v1.png": "T_RA_GoldCoinSocket",
    "reward_atomic_gold_label_plate_300x150_v1.png": "T_RA_GoldLabelPlate",
    "reward_atomic_gold_amount_plate_280x150_v1.png": "T_RA_GoldAmountPlate",
    "reward_atomic_card_background_240x389_v1.png": "T_RA_CardBackground",
    "reward_atomic_card_outer_frame_240x389_v1.png": "T_RA_CardOuterFrame",
    "reward_atomic_card_art_socket_180x180_v1.png": "T_RA_CardArtSocket",
    "reward_atomic_card_info_panel_180x54_v1.png": "T_RA_CardInfoPanel",
    "reward_atomic_card_name_plate_196x58_v1.png": "T_RA_CardNamePlate",
    "reward_atomic_card_selection_overlay_240x389_v1.png": "T_RA_CardSelectionOverlay",
}


tasks = []
for filename, asset_name in PARTS.items():
    source = os.path.join(SOURCE_DIR, filename)
    if not os.path.isfile(source):
        raise RuntimeError("Missing V9 atomic reward part: " + source)
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
        raise RuntimeError("V9 atomic reward part import failed: " + object_path)
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

unreal.log(f"RD_REWARD_ATOMIC_V9_IMPORT success count={len(PARTS)}")
for command in (
    "RD.Editor.BuildRewardSettlement",
    "RD.Editor.VerifyRewardSettlement",
):
    unreal.log("RD_REWARD_ATOMIC_V9 execute " + command)
    unreal.SystemLibrary.execute_console_command(None, command)

unreal.SystemLibrary.execute_console_command(None, "QUIT_EDITOR")
