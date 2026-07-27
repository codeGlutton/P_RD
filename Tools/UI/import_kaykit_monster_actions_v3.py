"""Import and verify the personality-first KayKit monster action art."""

import os

import unreal


SOURCE = os.path.join(
    unreal.Paths.project_dir(),
    "SourceArt",
    "UI",
    "KayKitMonsterPortraitsV3",
    "Processed",
)
PACKAGE = "/Game/SVN/OutSideAsset/UI/KayKit"
EXPECTED = {
    "KK_Face_Enemy_Eagle_ActionV3.png",
    "KK_Face_Enemy_Golem_ActionV3.png",
    "KK_Face_Enemy_Leshy_ActionV3.png",
    "KK_Face_Enemy_Mushroom_ActionV3.png",
    "KK_Face_Enemy_Necromancer_ActionV3.png",
    "KK_Face_Enemy_SkeletonGolem_ActionV3.png",
    "KK_Face_Enemy_SkeletonMinionMelee_ActionV3.png",
    "KK_Face_Enemy_SkeletonMinionRanged_ActionV3.png",
    "KK_Face_Enemy_Slime_ActionV3.png",
    "KK_Face_Enemy_Spider_ActionV3.png",
    "KK_Face_Enemy_Werewolf_ActionV3.png",
}


files = {
    name for name in os.listdir(SOURCE) if name.lower().endswith("_actionv3.png")
}
if files != EXPECTED:
    raise RuntimeError(
        "monster action set mismatch; missing={}, extra={}".format(
            sorted(EXPECTED - files),
            sorted(files - EXPECTED),
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
        raise RuntimeError(
            "{} imported at {}, expected (256, 256)".format(stem, size)
        )

unreal.log(
    "[KayKit Monster Actions V3] imported and verified {} textures".format(
        len(files)
    )
)
