"""Import the V8 outer shell, rebuild the reward WBP, and verify its layout."""

import os
import unreal


PROJECT_DIR = unreal.Paths.project_dir()
SOURCE = os.path.join(
    PROJECT_DIR,
    "Saved",
    "DesignAssets",
    "RewardWireframeV8_20260816",
    "GeneratedPartsV1",
    "reward_v8_outer_shell_no_bottom_crystal_1536x864.png",
)
DESTINATION = "/Game/UI/ResultBoards/GeneratedArt/V8"
DESTINATION_NAME = "T_RS_V8_OuterShellNoBottomCrystal"

if not os.path.isfile(SOURCE):
    raise RuntimeError("Missing V8 reward outer shell: " + SOURCE)

task = unreal.AssetImportTask()
task.set_editor_property("filename", SOURCE)
task.set_editor_property("destination_path", DESTINATION)
task.set_editor_property("destination_name", DESTINATION_NAME)
task.set_editor_property("automated", True)
task.set_editor_property("replace_existing", True)
task.set_editor_property("save", False)
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

object_path = f"{DESTINATION}/{DESTINATION_NAME}.{DESTINATION_NAME}"
texture = unreal.EditorAssetLibrary.load_asset(object_path)
if texture is None:
    raise RuntimeError("V8 reward outer shell import failed: " + object_path)

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
unreal.log("RD_REWARD_V8_IMPORT success")

for command in (
    "RD.Editor.BuildRewardSettlement",
    "RD.Editor.VerifyRewardSettlement",
):
    unreal.log("RD_REWARD_V8 execute " + command)
    unreal.SystemLibrary.execute_console_command(None, command)

unreal.SystemLibrary.execute_console_command(None, "QUIT_EDITOR")
