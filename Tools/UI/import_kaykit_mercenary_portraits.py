"""Import and verify the model-grounded KayKit mercenary portraits."""

import os

import unreal


SOURCE = os.path.join(
    unreal.Paths.project_dir(),
    "SourceArt",
    "UI",
    "KayKitMercenaryPortraitsV1",
    "Processed",
)
PACKAGE = "/Game/SVN/OutSideAsset/UI/KayKit"
EXPECTED = {
    "KK_Face_Barbarian_ActualV1.png",
    "KK_Face_BarbarianLarge_ActualV1.png",
    "KK_Face_Druid_ActualV1.png",
    "KK_Face_Engineer_ActualV1.png",
    "KK_Face_Knight_ActualV1.png",
    "KK_Face_Mage_ActualV1.png",
    "KK_Face_Ranger_ActualV1.png",
    "KK_Face_Rogue_ActualV1.png",
    "KK_Face_RogueHooded_ActualV1.png",
}


files = {
    name
    for name in os.listdir(SOURCE)
    if name.lower().endswith("_actualv1.png")
}
if files != EXPECTED:
    raise RuntimeError(
        "mercenary portrait set mismatch; missing={}, extra={}".format(
            sorted(EXPECTED - files), sorted(files - EXPECTED)
        )
    )

tasks = []
for name in sorted(files):
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", os.path.join(SOURCE, name))
    task.set_editor_property("destination_path", PACKAGE)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", True)
    tasks.append(task)

unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)

for name in sorted(files):
    stem = os.path.splitext(name)[0]
    path = "{}/{}".format(PACKAGE, stem)
    texture = unreal.EditorAssetLibrary.load_asset(path)
    if texture is None:
        raise RuntimeError("import failed: " + path)
    texture.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
    texture.set_editor_property(
        "compression_settings",
        unreal.TextureCompressionSettings.TC_EDITOR_ICON,
    )
    texture.set_editor_property(
        "mip_gen_settings",
        unreal.TextureMipGenSettings.TMGS_SIMPLE_AVERAGE,
    )
    texture.set_editor_property("srgb", True)
    unreal.EditorAssetLibrary.save_loaded_asset(texture, False)

    size = (texture.blueprint_get_size_x(), texture.blueprint_get_size_y())
    if size != (256, 256):
        raise RuntimeError("{} imported at {}, expected (256, 256)".format(stem, size))

unreal.log(
    "[KayKit Mercenaries] imported and verified {} textures".format(len(files))
)
