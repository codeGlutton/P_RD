"""Import the generated combat-tone skill-card frame as a UI texture."""

from pathlib import Path

import unreal


SOURCE_PATH = (
    Path(unreal.Paths.project_dir())
    / "시안4_7"
    / "스킬프레임"
    / "skill_card_frame_v03_combat.png"
)
DESTINATION_PATH = "/Game/UI/Art/Combat"
ASSET_NAME = "T_SkillCard_Frame_Combat"
ASSET_PATH = f"{DESTINATION_PATH}/{ASSET_NAME}"
EXPECTED_SIZE = (1217, 1292)


def main() -> None:
    if not SOURCE_PATH.is_file():
        raise RuntimeError(f"Missing combat skill-card frame: {SOURCE_PATH}")

    task = unreal.AssetImportTask()
    task.filename = str(SOURCE_PATH)
    task.destination_path = DESTINATION_PATH
    task.destination_name = ASSET_NAME
    task.automated = True
    task.replace_existing = True
    task.replace_existing_settings = False
    task.save = False
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

    texture = unreal.load_asset(ASSET_PATH)
    if not isinstance(texture, unreal.Texture2D):
        raise RuntimeError(f"Imported asset is not a Texture2D: {ASSET_PATH}")

    imported_size = (
        texture.blueprint_get_size_x(),
        texture.blueprint_get_size_y(),
    )
    if imported_size != EXPECTED_SIZE:
        raise RuntimeError(
            f"Unexpected frame dimensions {imported_size}; "
            f"expected {EXPECTED_SIZE}"
        )

    texture.modify()
    texture.set_editor_property(
        "compression_settings", unreal.TextureCompressionSettings.TC_EDITOR_ICON
    )
    texture.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
    texture.set_editor_property(
        "mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS
    )
    texture.set_editor_property("srgb", True)
    texture.set_editor_property("never_stream", True)

    if not unreal.EditorAssetLibrary.save_loaded_asset(
        texture, only_if_is_dirty=False
    ):
        raise RuntimeError(f"Could not save imported texture: {ASSET_PATH}")

    unreal.log(
        "RD_SKILL_CARD_FRAME_SYNC "
        f"source={SOURCE_PATH} asset={ASSET_PATH} "
        f"size={imported_size[0]}x{imported_size[1]} "
        f"compression={texture.get_editor_property('compression_settings')} "
        f"group={texture.get_editor_property('lod_group')} "
        f"mips={texture.get_editor_property('mip_gen_settings')} "
        f"srgb={texture.get_editor_property('srgb')}"
    )


main()
