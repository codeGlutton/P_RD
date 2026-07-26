"""Import the KayKit UI parts into the SVN mount as textures.

Art lives in SVN, not git: Content/SVN is a junction to the SVN working copy,
so these land under /Game/SVN/OutSideAsset/UI/KayKit/.

Mipmaps stay on. The parts are authored at 4x the size they draw at, and
minified art without mips shimmers -- that cost several build-and-look cycles
on the previous art line.
"""
import os
import unreal

SOURCE = r"D:/UnrealProjects/P_RD_develop/Tools/UI/KayKitUIKit/Processed"
PACKAGE = "/Game/SVN/OutSideAsset/UI/KayKit"

files = sorted(f for f in os.listdir(SOURCE) if f.lower().endswith(".png"))
if not files:
    raise RuntimeError("no PNGs under {}".format(SOURCE))

tasks = []
for name in files:
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", os.path.join(SOURCE, name))
    task.set_editor_property("destination_path", PACKAGE)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", True)
    tasks.append(task)

unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)

# Read back from disk. An import that silently did nothing looks exactly like
# one that worked until the widget draws a white box.
imported, missing = [], []
for name in files:
    stem = os.path.splitext(name)[0]
    path = "{}/{}".format(PACKAGE, stem)
    texture = unreal.EditorAssetLibrary.load_asset(path)
    if texture is None:
        missing.append(stem)
        continue
    texture.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
    texture.set_editor_property("compression_settings",
                                unreal.TextureCompressionSettings.TC_EDITOR_ICON)
    texture.set_editor_property("mip_gen_settings",
                                unreal.TextureMipGenSettings.TMGS_SIMPLE_AVERAGE)
    texture.set_editor_property("srgb", True)
    unreal.EditorAssetLibrary.save_loaded_asset(texture, False)
    imported.append(stem)

unreal.log("[KayKit] imported {}/{}".format(len(imported), len(files)))
if missing:
    raise RuntimeError("import failed:\n  " + "\n  ".join(missing))

# Confirm the sizes survived: a resize on import would break the grid.
EXPECT = {
    "KK_HFrame_Corner_T": (256, 256), "KK_HFrame_Link_T": (128, 128),
    "KK_LFrame_Corner_T": (128, 128), "KK_LFrame_Link_T": (64, 64),
    "KK_Select_Corner": (128, 128), "KK_Select_Link_H": (64, 64),
    "KK_Fill_Stone": (512, 512), "KK_Bar_Link": (64, 128),
    "KK_Ring_Portrait": (512, 512), "KK_Icon_Move": (256, 256),
}
for stem, want in EXPECT.items():
    texture = unreal.EditorAssetLibrary.load_asset("{}/{}".format(PACKAGE, stem))
    got = (texture.blueprint_get_size_x(), texture.blueprint_get_size_y())
    if got != want:
        raise RuntimeError("{}: {} (기대 {})".format(stem, got, want))
unreal.log("[KayKit] 격자 크기 확인 완료")
