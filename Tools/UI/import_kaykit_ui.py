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
SLICE_SOURCE = r"D:/UnrealProjects/P_RD_develop/Tools/UI/KayKitUIKit/Slices"
PACKAGE = "/Game/SVN/OutSideAsset/UI/KayKit"
SLICE_PACKAGE = PACKAGE + "/Slices"

files = sorted(f for f in os.listdir(SOURCE) if f.lower().endswith(".png"))
if not files:
    raise RuntimeError("no PNGs under {}".format(SOURCE))

#: 시안에서 통째로 뜬 9슬라이스 판. 조각 조립을 대신한다.
slice_files = sorted(f for f in os.listdir(SLICE_SOURCE)
                     if f.lower().endswith(".png"))     if os.path.isdir(SLICE_SOURCE) else []

tasks = []
for name in slice_files:
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", os.path.join(SLICE_SOURCE, name))
    task.set_editor_property("destination_path", SLICE_PACKAGE)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", True)
    tasks.append(task)
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

# 임포트가 크기를 바꿨는지 확인한다. 이 검사의 목적은 "임포트 중 리사이즈"를
# 잡는 것이지 아트 치수를 못 박는 게 아니다. 기대값을 손으로 적어 두었더니
# 아트를 의도적으로 자를 때마다 임포터가 막았다 -- 원본 PNG에서 읽는다.
def png_size(path):
    """PNG 머리 24바이트에서 폭·높이. 실패하면 None."""
    with open(path, "rb") as handle:
        head = handle.read(24)
    if len(head) < 24 or head[1:4] != b"PNG":
        return None
    return (int.from_bytes(head[16:20], "big"),
            int.from_bytes(head[20:24], "big"))


for name in slice_files:
    stem = os.path.splitext(name)[0]
    if unreal.EditorAssetLibrary.load_asset(
            "{}/{}".format(SLICE_PACKAGE, stem)) is None:
        raise RuntimeError("슬라이스 임포트 실패: {}".format(stem))
unreal.log("[KayKit] 슬라이스 {}장".format(len(slice_files)))

checked = 0
for stem in imported:
    want = png_size(os.path.join(SOURCE, stem + ".png"))
    if want is None:
        continue
    texture = unreal.EditorAssetLibrary.load_asset("{}/{}".format(PACKAGE, stem))
    got = (texture.blueprint_get_size_x(), texture.blueprint_get_size_y())
    if got != want:
        raise RuntimeError("{}: {} (원본 {})".format(stem, got, want))
    checked += 1
unreal.log("[KayKit] 크기 대조 {}장 통과".format(checked))
