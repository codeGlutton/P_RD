"""Import the ImageGen V2 KayKit portraits and skill icons.

The generated originals and their chroma-keyed, normalized derivatives live
under SourceArt so the import can be repeated deterministically.  V2 names are
kept separate from the approved V1 set until the HUD comparison is accepted.
"""
import os

import unreal


SOURCE = os.path.join(
    unreal.Paths.project_dir(), "SourceArt", "UI", "KayKitGeneratedV2",
    "Processed")
PACKAGE = "/Game/SVN/OutSideAsset/UI/KayKit"


def png_size(path):
    with open(path, "rb") as handle:
        head = handle.read(24)
    if len(head) < 24 or head[1:4] != b"PNG":
        return None
    return (int.from_bytes(head[16:20], "big"),
            int.from_bytes(head[20:24], "big"))


files = sorted(
    name for name in os.listdir(SOURCE)
    if name.lower().endswith("_v2.png"))
if len(files) != 7:
    raise RuntimeError("expected 7 V2 PNGs, found {} under {}".format(
        len(files), SOURCE))

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

for name in files:
    stem = os.path.splitext(name)[0]
    path = "{}/{}".format(PACKAGE, stem)
    texture = unreal.EditorAssetLibrary.load_asset(path)
    if texture is None:
        raise RuntimeError("import failed: " + path)
    texture.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
    texture.set_editor_property(
        "compression_settings",
        unreal.TextureCompressionSettings.TC_EDITOR_ICON)
    texture.set_editor_property(
        "mip_gen_settings",
        unreal.TextureMipGenSettings.TMGS_SIMPLE_AVERAGE)
    texture.set_editor_property("srgb", True)
    unreal.EditorAssetLibrary.save_loaded_asset(texture, False)

    want = png_size(os.path.join(SOURCE, name))
    got = (texture.blueprint_get_size_x(), texture.blueprint_get_size_y())
    if got != want:
        raise RuntimeError("{} imported at {}, expected {}".format(
            stem, got, want))

unreal.log("[KayKit V2] imported and verified {} textures".format(len(files)))
