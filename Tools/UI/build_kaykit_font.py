"""Import Noto Sans KR and build the composite Font the KayKit HUD draws with.

Why
---
The mock-ups are set in a rounded gothic; the implementation was still drawing
in NotoSerifKR, and typography is one of the strongest signals of whether a
screen reads as a game UI or as a document. A serif under low-poly chibi
characters reads as the wrong game.

The Windows copy of Noto Sans KR is a *variable* font, which Unreal imports as
a single default instance with no Bold. Tools/.. instances it into static
Regular (wght 400) and Bold (wght 700) first; this script imports those two
faces and assembles the composite font.

A FontFace cannot be assigned to a Slate font info on its own -- that drops the
fallback chain, which for Korean means missing glyphs. What a widget needs is a
Font asset in Runtime cache mode carrying a composite font with named
typefaces, so `typeface_font_name` can switch weight.

Art lives in SVN, so the faces land under /Game/SVN/OutSideAsset/UI/KayKit.
Noto Sans KR is SIL OFL, so shipping it with the game is fine.

Run through Tools/RunEditorPython.ps1 so a failure is actually reported.
"""
import os

import unreal

SOURCE_DIR = os.path.join(
    unreal.Paths.project_dir(), "SourceArt", "UI", "Fonts", "NotoSansKR")
FACE_DIR = "/Game/SVN/OutSideAsset/UI/KayKit/Fonts/NotoSansKR"
FONT_DIR = "/Game/SVN/OutSideAsset/UI/KayKit/Fonts"
FONT_NAME = "F_HUD_NotoSansKR"

FACES = (("Regular", "NotoSansKR-Regular.ttf"),
         ("Bold", "NotoSansKR-Bold.ttf"))


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
        asset_path = "{}/{}".format(FACE_DIR, stem)
        if unreal.EditorAssetLibrary.load_asset(asset_path) is None:
            raise RuntimeError("face did not import: {}".format(asset_path))
        paths.append(asset_path)
    unreal.log("[Font] imported {} faces".format(len(paths)))
    return paths


def entry(name, path):
    """One typeface entry, as struct text.

    Python does not expose UTypeface, FTypefaceEntry or FFontData, so the
    composite font cannot be assembled object by object. FCompositeFont does
    expose import_text, and Unreal's own struct text format resolves the asset
    reference for us -- which is why this builds a string instead.
    """
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

    # Runtime cache lets Slate rasterise from the faces on demand; Offline
    # would expect a pre-baked page texture, which 11172 Hangul glyphs is not.
    font.set_editor_property("font_cache_type", unreal.FontCacheType.RUNTIME)

    composite = font.get_editor_property("composite_font")
    composite.import_text("(DefaultTypeface=(Fonts=({})))".format(
        ",".join(entry(name, path)
                 for (name, _), path in zip(FACES, paths))))
    font.set_editor_property("composite_font", composite)
    unreal.EditorAssetLibrary.save_loaded_asset(font, only_if_is_dirty=False)

    # Read it back off disk: import_text silently keeps an empty Fonts list if
    # the reference does not resolve, and an empty typeface renders as the
    # default sans -- exactly the state this asset exists to leave behind.
    check = unreal.EditorAssetLibrary.load_asset(asset_path)
    text = check.get_editor_property("composite_font").export_text()
    for name, _ in FACES:
        if '"{}"'.format(name) not in text:
            raise RuntimeError("typeface {!r} did not survive".format(name))
    if "NotoSansKR" not in text:
        raise RuntimeError("no NotoSansKR face landed in the composite font")
    unreal.log("[Font] built {} (Regular + Bold)".format(asset_path))


build()
