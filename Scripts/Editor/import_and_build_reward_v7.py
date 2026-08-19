"""Import the transparent V7 reward parts, rebuild the WBP, and verify it."""

import os
import unreal


PROJECT_DIR = unreal.Paths.project_dir()
SOURCE_DIR = os.path.join(
    PROJECT_DIR,
    "Saved",
    "DesignAssets",
    "RewardWireframeV7_20260816",
    "GeneratedPartsV1",
)
DESTINATION = "/Game/UI/ResultBoards/GeneratedArt/V7"

IMPORTS = {
    "reward_v7_outer_shell_1536x864.png": "T_RS_V7_OuterShell",
    "reward_v7_header_720x141.png": "T_RS_V7_Header",
    "reward_v7_step_badge_380x62.png": "T_RS_V7_StepBadge",
    "reward_v7_content_panel_1376x518.png": "T_RS_V7_ContentPanel",
    "reward_v7_gold_panel_640x182.png": "T_RS_V7_GoldPanel",
    "reward_v7_artifact_card_216x350.png": "T_RS_V7_ArtifactCard",
    "reward_v7_primary_button_440x82_v2.png": "T_RS_V7_PrimaryButton",
}


tasks = []
for filename, destination_name in IMPORTS.items():
    source = os.path.join(SOURCE_DIR, filename)
    if not os.path.isfile(source):
        raise RuntimeError("Missing V7 reward part: " + source)

    task = unreal.AssetImportTask()
    task.set_editor_property("filename", source)
    task.set_editor_property("destination_path", DESTINATION)
    task.set_editor_property("destination_name", destination_name)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", False)
    tasks.append(task)

unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)

for destination_name in IMPORTS.values():
    object_path = f"{DESTINATION}/{destination_name}.{destination_name}"
    texture = unreal.EditorAssetLibrary.load_asset(object_path)
    if texture is None:
        raise RuntimeError("V7 reward texture import failed: " + object_path)

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

unreal.log(f"RD_REWARD_V7_IMPORT success count={len(IMPORTS)}")

for command in (
    "RD.Editor.BuildRewardSettlement",
    "RD.Editor.VerifyRewardSettlement",
):
    unreal.log("RD_REWARD_V7 execute " + command)
    unreal.SystemLibrary.execute_console_command(None, command)

unreal.SystemLibrary.execute_console_command(None, "QUIT_EDITOR")
