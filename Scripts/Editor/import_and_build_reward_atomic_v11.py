"""Import V11 per-wireframe-slot reward parts, rebuild, and verify."""

import os
import unreal


PROJECT_DIR = unreal.Paths.project_dir()
SOURCE_DIR = os.path.join(
    PROJECT_DIR,
    "Saved",
    "DesignAssets",
    "RewardAtomicV11_20260817",
    "GeneratedPartsV1",
)
DESTINATION = "/Game/UI/ResultBoards/GeneratedArt/AtomicV11"
PARTS = {
    "reward_v11_modal_background_1536x864.png": "T_R11_ModalBackground",
    "reward_v11_modal_outer_frame_1536x864.png": "T_R11_ModalOuterFrame",
    "reward_v11_parchment_sheet_1352x498.png": "T_R11_ParchmentSheet",
    "reward_v11_header_plate_720x141.png": "T_R11_HeaderPlate",
    "reward_v11_step_plate_380x62.png": "T_R11_StepPlate",
    "reward_v11_cta_plate_404x70.png": "T_R11_CtaPlate",
    "reward_v11_exp_portrait_235x132.png": "T_R11_ExpPortrait",
    "reward_v11_exp_level_110x132.png": "T_R11_ExpLevel",
    "reward_v11_exp_progress_525x132.png": "T_R11_ExpProgress",
    "reward_v11_exp_xp_140x132.png": "T_R11_ExpXp",
    "reward_v11_exp_tail_44x132.png": "T_R11_ExpTail",
    "reward_v11_gold_coin_220x228.png": "T_R11_GoldCoin",
    "reward_v11_gold_label_280x228.png": "T_R11_GoldLabel",
    "reward_v11_gold_amount_300x228.png": "T_R11_GoldAmount",
    "reward_v11_card_frame_240x230.png": "T_R11_CardFrame",
    "reward_v11_card_backing_168x164.png": "T_R11_CardBacking",
    "reward_v11_card_name_240x70.png": "T_R11_CardName",
    "reward_v11_card_footer_240x19.png": "T_R11_CardFooter",
}


tasks = []
for filename, asset_name in PARTS.items():
    source = os.path.join(SOURCE_DIR, filename)
    if not os.path.isfile(source):
        raise RuntimeError("Missing V11 reward part: " + source)
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
        raise RuntimeError("V11 reward part import failed: " + object_path)
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

unreal.log(f"RD_REWARD_ATOMIC_V11_IMPORT success count={len(PARTS)}")
for command in (
    "RD.Editor.BuildRewardSettlement",
    "RD.Editor.VerifyRewardSettlement",
):
    unreal.log("RD_REWARD_ATOMIC_V11 execute " + command)
    unreal.SystemLibrary.execute_console_command(None, command)

unreal.SystemLibrary.execute_console_command(None, "QUIT_EDITOR")
