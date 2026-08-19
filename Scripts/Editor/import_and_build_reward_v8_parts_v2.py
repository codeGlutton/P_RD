"""Import every independently generated V8 reward part, rebuild, and verify."""

import os
import unreal


PROJECT_DIR = unreal.Paths.project_dir()
SOURCE_DIR = os.path.join(
    PROJECT_DIR,
    "Saved",
    "DesignAssets",
    "RewardWireframeV8_20260816",
    "GeneratedPartsV2",
)
DESTINATION = "/Game/UI/ResultBoards/GeneratedArt/V8PartsV2"
PARTS = {
    "reward_v8_header_720x141_v2.png": "T_RS_V8_Header",
    "reward_v8_step_badge_380x62_v2.png": "T_RS_V8_StepBadge",
    "reward_v8_content_panel_1376x518_v2_clean_edgefixed.png": "T_RS_V8_ContentPanel",
    "reward_v8_exp_row_1054x132_v2_clean_edgefixed.png": "T_RS_V8_ExpRow",
    "reward_v8_gold_panel_800x228_v2_clean_edgefixed.png": "T_RS_V8_GoldPanel",
    "reward_v8_artifact_card_240x389_v2.png": "T_RS_V8_ArtifactCard",
    "reward_v8_primary_button_404x70_v2.png": "T_RS_V8_PrimaryButton",
    "reward_v8_chest_closed_380x304_v2.png": "T_RS_V8_ChestClosed",
    "reward_v8_chest_aura_512x512_v2.png": "T_RS_V8_ChestAura",
    "reward_v8_chest_burst_512x512_v2.png": "T_RS_V8_ChestBurst",
}


tasks = []
for filename, asset_name in PARTS.items():
    source = os.path.join(SOURCE_DIR, filename)
    if not os.path.isfile(source):
        raise RuntimeError("Missing V8 V2 reward part: " + source)
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
        raise RuntimeError("V8 V2 reward part import failed: " + object_path)
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

unreal.log("RD_REWARD_V8_PARTS_V2_IMPORT success")
for command in (
    "RD.Editor.BuildRewardSettlement",
    "RD.Editor.VerifyRewardSettlement",
):
    unreal.log("RD_REWARD_V8_PARTS_V2 execute " + command)
    unreal.SystemLibrary.execute_console_command(None, command)

unreal.SystemLibrary.execute_console_command(None, "QUIT_EDITOR")
