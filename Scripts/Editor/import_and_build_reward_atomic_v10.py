"""Import V10 single-master atomic reward parts, rebuild, and verify."""

import os
import unreal


PROJECT_DIR = unreal.Paths.project_dir()
SOURCE_DIR = os.path.join(
    PROJECT_DIR,
    "Saved",
    "DesignAssets",
    "RewardAtomicV10_20260817",
    "GeneratedPartsV1",
)
DESTINATION = "/Game/UI/ResultBoards/GeneratedArt/AtomicV10"
PARTS = {
    "reward_v10_modal_background_1536x864.png": "T_R10_ModalBackground",
    "reward_v10_modal_outer_frame_1536x864.png": "T_R10_ModalOuterFrame",
    "reward_v10_body_background_1376x518.png": "T_R10_BodyBackground",
    "reward_v10_body_frame_1376x518.png": "T_R10_BodyFrame",
    "reward_v10_header_background_720x141.png": "T_R10_HeaderBackground",
    "reward_v10_header_frame_720x141.png": "T_R10_HeaderFrame",
    "reward_v10_step_background_380x62.png": "T_R10_StepBackground",
    "reward_v10_step_frame_380x62.png": "T_R10_StepFrame",
    "reward_v10_cta_background_404x70.png": "T_R10_CtaBackground",
    "reward_v10_cta_frame_404x70.png": "T_R10_CtaFrame",
    "reward_v10_exp_portrait_segment_235x132.png": "T_R10_ExpPortraitSegment",
    "reward_v10_exp_level_segment_110x132.png": "T_R10_ExpLevelSegment",
    "reward_v10_exp_progress_segment_525x132.png": "T_R10_ExpProgressSegment",
    "reward_v10_exp_xp_segment_140x132.png": "T_R10_ExpXpSegment",
    "reward_v10_exp_tail_segment_44x132.png": "T_R10_ExpTailSegment",
    "reward_v10_gold_coin_segment_220x228.png": "T_R10_GoldCoinSegment",
    "reward_v10_gold_label_segment_280x228.png": "T_R10_GoldLabelSegment",
    "reward_v10_gold_amount_segment_300x228.png": "T_R10_GoldAmountSegment",
    "reward_v10_card_background_240x389.png": "T_R10_CardBackground",
    "reward_v10_card_art_segment_240x230.png": "T_R10_CardArtSegment",
    "reward_v10_card_info_segment_240x70.png": "T_R10_CardInfoSegment",
    "reward_v10_card_name_segment_240x70.png": "T_R10_CardNameSegment",
    "reward_v10_card_footer_segment_240x19.png": "T_R10_CardFooterSegment",
}


tasks = []
for filename, asset_name in PARTS.items():
    source = os.path.join(SOURCE_DIR, filename)
    if not os.path.isfile(source):
        raise RuntimeError("Missing V10 reward part: " + source)
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
        raise RuntimeError("V10 reward part import failed: " + object_path)
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

unreal.log(f"RD_REWARD_ATOMIC_V10_IMPORT success count={len(PARTS)}")
for command in (
    "RD.Editor.BuildRewardSettlement",
    "RD.Editor.VerifyRewardSettlement",
):
    unreal.log("RD_REWARD_ATOMIC_V10 execute " + command)
    unreal.SystemLibrary.execute_console_command(None, command)

unreal.SystemLibrary.execute_console_command(None, "QUIT_EDITOR")
