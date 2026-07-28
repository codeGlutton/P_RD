"""Import LINE Seed Sans KR and build the KayKit HUD composite font.

LINE Seed Sans KR is distributed by LY Corporation under SIL OFL 1.1.  The
original ZIP, the two unmodified TTF faces, and the OFL text are preserved
under SourceArt/UI/Fonts/LINESeedKR.
"""
import os

import unreal


SOURCE_DIR = os.path.join(
    unreal.Paths.project_dir(), "SourceArt", "UI", "Fonts", "LINESeedKR")
FACE_DIR = "/Game/SVN/OutSideAsset/Fonts/LINESeedKR"
FONT_DIR = "/Game/SVN/OutSideAsset/Fonts"
FONT_NAME = "F_HUD_LINESeedKR"
FACES = (("Regular", "LINESeedKR-Regular.ttf"),
         ("Bold", "LINESeedKR-Bold.ttf"))


def import_faces():
    tasks = []
    for _, filename in FACES:
        path = os.path.join(SOURCE_DIR, filename)
        if not os.path.exists(path):
            raise RuntimeError("font file missing: {}".format(path))
        task = unreal.AssetImportTask()
        task.set_editor_property("filename", path)
        task.set_editor_property("destination_path", FACE_DIR)
        task.set_editor_property("automated", True)
        task.set_editor_property("replace_existing", True)
        task.set_editor_property("save", True)
        tasks.append(task)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)

    paths = []
    for _, filename in FACES:
        stem = os.path.splitext(filename)[0]
        path = "{}/{}".format(FACE_DIR, stem)
        if unreal.EditorAssetLibrary.load_asset(path) is None:
            raise RuntimeError("font face did not import: " + path)
        paths.append(path)
    return paths


def entry(name, path):
    return ('(Name="{}",Font=(FontFaceAsset=FontFace\'"{}.{}"\','
            'LoadingPolicy=LazyLoad))').format(
                name, path, path.rsplit("/", 1)[-1])


def build():
    paths = import_faces()
    asset_path = "{}/{}".format(FONT_DIR, FONT_NAME)
    if unreal.EditorAssetLibrary.does_asset_exist(asset_path):
        unreal.EditorAssetLibrary.delete_asset(asset_path)

    font = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        FONT_NAME, FONT_DIR, unreal.Font, unreal.FontFactory())
    if font is None:
        raise RuntimeError("could not create " + asset_path)
    font.set_editor_property("font_cache_type", unreal.FontCacheType.RUNTIME)

    composite = font.get_editor_property("composite_font")
    composite.import_text("(DefaultTypeface=(Fonts=({})))".format(
        ",".join(entry(name, path)
                 for (name, _), path in zip(FACES, paths))))
    font.set_editor_property("composite_font", composite)
    unreal.EditorAssetLibrary.save_loaded_asset(font, only_if_is_dirty=False)

    check = unreal.EditorAssetLibrary.load_asset(asset_path)
    text = check.get_editor_property("composite_font").export_text()
    for name, _ in FACES:
        if '"{}"'.format(name) not in text:
            raise RuntimeError("typeface {!r} did not survive".format(name))
    if "LINESeedKR" not in text:
        raise RuntimeError("LINE Seed faces are absent from composite font")
    unreal.log("[Font] built {} (Regular + Bold)".format(asset_path))


build()
