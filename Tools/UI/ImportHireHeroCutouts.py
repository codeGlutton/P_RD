"""Import the six versioned mercenary foreground cutouts into Unreal.

Run with UnrealEditor-Cmd after the final transparent PNGs have been written to
SourceArt/UI/MercenaryHire/HeroCutouts.  This script intentionally imports only
the foreground cutouts; the independently generated wide backgrounds are left
untouched.
"""

import os

import unreal


PROJECT_ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
SOURCE_DIR = os.path.join(
    PROJECT_ROOT, "SourceArt", "UI", "MercenaryHire", "HeroCutouts"
)
DESTINATION = "/Game/UI/MercenaryHire/HeroCutouts"
CLASSES = ("Knight", "Mage", "Ranger", "Rogue", "Barbarian", "Druid")


def fail(message: str) -> None:
    unreal.log_error("RD_HIRE_CUTOUT_IMPORT " + message)
    raise RuntimeError(message)


asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
for hero_class in CLASSES:
    asset_name = f"T_HireHeroCutout_{hero_class}_v1"
    filename = os.path.join(SOURCE_DIR, asset_name + ".png")
    if not os.path.isfile(filename):
        fail(f"missing source image: {filename}")

    # UE 5.7 routes AssetImportTask through Interchange by default. In a
    # Python commandlet Interchange attempts to post a Content Browser
    # notification even though Slate is not initialized, which asserts after
    # the first texture. The legacy UTextureFactory path is fully unattended
    # and deterministic for these PNG UI textures.
    factory = unreal.TextureFactory()
    import_data = unreal.AutomatedAssetImportData()
    import_data.set_editor_property("destination_path", DESTINATION)
    import_data.set_editor_property("filenames", [filename])
    import_data.set_editor_property("replace_existing", True)
    import_data.set_editor_property("skip_read_only", True)
    import_data.set_editor_property("factory", factory)
    imported = asset_tools.import_assets_automated(import_data)
    if not imported:
        fail(f"failed to import source image: {filename}")

for hero_class in CLASSES:
    asset_name = f"T_HireHeroCutout_{hero_class}_v1"
    package_path = f"{DESTINATION}/{asset_name}"
    texture = unreal.EditorAssetLibrary.load_asset(package_path)
    if texture is None:
        fail(f"failed to load imported texture: {package_path}")

    texture.set_editor_property("srgb", True)
    texture.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
    texture.set_editor_property(
        "compression_settings", unreal.TextureCompressionSettings.TC_DEFAULT
    )
    texture.set_editor_property("compression_no_alpha", False)
    texture.set_editor_property("never_stream", False)
    texture.modify()
    if not unreal.EditorAssetLibrary.save_loaded_asset(
        texture, only_if_is_dirty=False
    ):
        fail(f"failed to save imported texture: {package_path}")
    unreal.log(f"RD_HIRE_CUTOUT_IMPORT configured {package_path}")

unreal.log("RD_HIRE_CUTOUT_IMPORT success")
