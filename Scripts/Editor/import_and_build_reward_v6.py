"""Import the V6 modular reward parts, rebuild the WBP, and verify it."""

import os
import unreal


PROJECT_DIR = unreal.Paths.project_dir()
SOURCE_DIR = os.path.join(
    PROJECT_DIR,
    "Saved",
    "DesignAssets",
    "RewardWireframeV6_20260816",
    "GeneratedPartsV1",
)
DESTINATION = "/Game/UI/ResultBoards/GeneratedArt/V6"

IMPORTS = {
    "reward_outer_shell_1536x864_v1.png": "T_RS_V6_OuterShell",
    "reward_header_plate_1040x160_v1.png": "T_RS_V6_HeaderPlate",
    "reward_step_badge_800x160_v1.png": "T_RS_V6_StepBadge",
    "reward_content_panel_1280x640_v1.png": "T_RS_V6_ContentPanel",
    "reward_gold_panel_1216x320_v1.png": "T_RS_V6_GoldPanel",
    "reward_artifact_card_640x960_v1.png": "T_RS_V6_ArtifactCard",
    "reward_primary_button_800x160_v1.png": "T_RS_V6_PrimaryButton",
}


tasks = []
for filename, destination_name in IMPORTS.items():
    source = os.path.join(SOURCE_DIR, filename)
    if not os.path.isfile(source):
        raise RuntimeError("Missing V6 reward part: " + source)

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
        raise RuntimeError("V6 reward texture import failed: " + object_path)

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

unreal.log(f"RD_REWARD_V6_IMPORT success count={len(IMPORTS)}")

for command in (
    "RD.Editor.BuildRewardSettlement",
    "RD.Editor.VerifyRewardSettlement",
):
    unreal.log("RD_REWARD_V6 execute " + command)
    unreal.SystemLibrary.execute_console_command(None, command)

unreal.SystemLibrary.execute_console_command(None, "QUIT_EDITOR")
