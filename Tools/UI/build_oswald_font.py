# -*- coding: utf-8 -*-
"""Oswald 를 HUD 폰트로 넣는다. 한글은 LINE Seed 로 떨어지게 한다.

## 왜 합성 폰트인가

Oswald 에는 **한글 자모가 없다.** 그대로 쓰면 "베기" 나 "턴 종료" 가 네모로
나온다. 폰트를 고른 근거가 숫자 모양이었으니 숫자와 라틴은 Oswald 로 두고,
한글은 지금까지 쓰던 LINE Seed 로 떨어져야 한다.

UE 폰트 자산은 기본 얼굴 하나에 **글자 범위별 대체 얼굴**을 붙일 수 있다.
그래서 한 자산으로 둘을 같이 쓴다 -- 글자마다 어느 폰트를 쓸지 부르는 쪽이
고르게 하면, 새 글자를 놓을 때마다 그 판단을 되풀이해야 한다.

## 가변 폰트를 왜 쪼갰나

구글이 내주는 것은 Oswald[wght].ttf 하나뿐이다. UE 는 가변 축을 골라 쓰지
못하고 기본값(400)만 읽으므로, fonttools 로 400 과 700 을 따로 떠 두었다.
OFL 사본에 Reserved Font Name 이 없어 이름을 그대로 써도 된다.

    UnrealEditor-Cmd.exe <uproject> -run=pythonscript -script=build_oswald_font.py
"""
import os

import unreal

SOURCE_DIR = os.path.join(
    unreal.Paths.project_dir(), "SourceArt", "UI", "Fonts", "Oswald")
SEED_DIR = os.path.join(
    unreal.Paths.project_dir(), "SourceArt", "UI", "Fonts", "LINESeedKR")

FACE_DIR = "/Game/SVN/OutSideAsset/Fonts/Oswald"
SEED_FACE_DIR = "/Game/SVN/OutSideAsset/Fonts/LINESeedKR"
FONT_DIR = "/Game/SVN/OutSideAsset/Fonts"
FONT_NAME = "F_HUD_Oswald"

FACES = (("Regular", "Oswald-Regular.ttf"),
         ("Bold", "Oswald-Bold.ttf"))

#: 한글이 사는 자리. 이 범위는 LINE Seed 로 떨어진다.
#:
#: 완성형만으로는 모자란다 -- 조합형 자모와 호환 자모가 섞여 들어오고,
#: 문장 부호와 전각 문자도 Oswald 에 없다.
KOREAN_RANGES = (
    (0x1100, 0x11FF),   # 한글 자모
    (0x3000, 0x303F),   # CJK 문장 부호
    (0x3130, 0x318F),   # 호환 자모
    (0xAC00, 0xD7A3),   # 완성형
    (0xFF00, 0xFFEF),   # 전각
)


def import_faces(source_dir, faces, dest):
    tasks = []
    for _, filename in faces:
        path = os.path.join(source_dir, filename)
        if not os.path.exists(path):
            raise RuntimeError("폰트 파일이 없다: {}".format(path))
        task = unreal.AssetImportTask()
        task.set_editor_property("filename", path)
        task.set_editor_property("destination_path", dest)
        task.set_editor_property("automated", True)
        task.set_editor_property("replace_existing", True)
        task.set_editor_property("save", True)
        # 팩토리를 박아 넣는다. 안 정해 주면 임포트가 고르는 창을 띄우려 하고,
        # 커맨드릿에는 Slate 가 없어 CurrentApplication.IsValid() 로 죽는다 --
        # 텍스처 임포트에서 겪은 것과 같은 벽이다.
        task.set_editor_property("factory", unreal.FontFileImportFactory())
        tasks.append(task)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)

    paths = []
    for _, filename in faces:
        stem = os.path.splitext(filename)[0]
        path = "{}/{}".format(dest, stem)
        if unreal.EditorAssetLibrary.load_asset(path) is None:
            raise RuntimeError("폰트 얼굴이 안 들어왔다: " + path)
        paths.append(path)
    return paths


def entry(name, path):
    return ('(Name="{}",Font=(FontFaceAsset=FontFace\'"{}.{}"\','
            'LoadingPolicy=LazyLoad))').format(
                name, path, path.rsplit("/", 1)[-1])


def typeface(faces, paths):
    return "(Fonts=({}))".format(
        ",".join(entry(name, path) for (name, _), path in zip(faces, paths)))


def ranges_text():
    return ",".join(
        "(LowerBound=(Type=Inclusive,Value={}),"
        "UpperBound=(Type=Inclusive,Value={}))".format(low, high)
        for low, high in KOREAN_RANGES)


def build():
    oswald = import_faces(SOURCE_DIR, FACES, FACE_DIR)

    # 한글 얼굴은 이미 들어와 있다. 없으면 같이 넣는다.
    seed_faces = (("Regular", "LINESeedKR-Regular.ttf"),
                  ("Bold", "LINESeedKR-Bold.ttf"))
    seed = ["{}/{}".format(SEED_FACE_DIR, os.path.splitext(f)[0])
            for _, f in seed_faces]
    if any(unreal.EditorAssetLibrary.load_asset(p) is None for p in seed):
        seed = import_faces(SEED_DIR, seed_faces, SEED_FACE_DIR)

    asset_path = "{}/{}".format(FONT_DIR, FONT_NAME)
    if unreal.EditorAssetLibrary.does_asset_exist(asset_path):
        unreal.EditorAssetLibrary.delete_asset(asset_path)

    font = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        FONT_NAME, FONT_DIR, unreal.Font, unreal.FontFactory())
    if font is None:
        raise RuntimeError("못 만들었다: " + asset_path)
    font.set_editor_property("font_cache_type", unreal.FontCacheType.RUNTIME)

    composite = font.get_editor_property("composite_font")
    composite.import_text(
        "(DefaultTypeface={},SubTypefaces=((Typeface={},"
        "CharacterRanges=({}))))".format(
            typeface(FACES, oswald), typeface(seed_faces, seed),
            ranges_text()))
    font.set_editor_property("composite_font", composite)
    unreal.EditorAssetLibrary.save_loaded_asset(font, only_if_is_dirty=False)

    # 넣은 것이 살아남았는지 본다. import_text 는 틀린 글을 조용히 버린다 --
    # 확인 없이 넘어가면 한글이 네모로 나오는 것을 게임에서야 알게 된다.
    check = unreal.EditorAssetLibrary.load_asset(asset_path)
    text = check.get_editor_property("composite_font").export_text()
    if "Oswald" not in text:
        raise RuntimeError("Oswald 얼굴이 안 들어갔다")
    if "LINESeedKR" not in text:
        raise RuntimeError("한글 대체 얼굴이 안 들어갔다")
    if "44032" not in text:
        raise RuntimeError("한글 범위가 안 들어갔다 (완성형 시작값)")
    unreal.log("[Font] {} 만들었다 (Oswald + 한글 대체)".format(asset_path))


build()
