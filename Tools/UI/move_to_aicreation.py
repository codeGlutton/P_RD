# -*- coding: utf-8 -*-
"""쓰는 UI 아트를 AICreation 아래로 옮긴다. 폰트는 따로 뺀다.

## 왜 에디터 안에서 옮기나

파일을 손으로 옮기면 참조가 끊긴다. 유닛 데이터가 초상을 경로로 물고 있어서
(`m_portrait`), 폴더만 바꾸면 그 열일곱이 통째로 빈다.

`rename_directory` 는 옮기면서 참조를 고쳐 준다. 그 뒤에 리다이렉터를 정리해
옛 경로를 없앤다 -- 안 없애면 옛 자리가 유령으로 남아 SVN 에도 올라간다.

## 무엇을 옮기나

지금 쓰는 것만이다. 시안1에서 오려 낸 판(Chrome·Cutouts·Slices)과 안 쓰는
낱개는 옮기지 않는다 -- SVN 에 안 올리고 버리기로 했다.

    UnrealEditor-Cmd.exe <uproject> -run=pythonscript -script=move_to_aicreation.py
"""
import io
import os

import unreal

KK = "/Game/SVN/OutSideAsset/UI/KayKit"
AI = "/Game/SVN/OutSideAsset/AICreation/UI"
FONTS = "/Game/SVN/OutSideAsset/Fonts"

REPORT = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                      "_move_report.txt")
LINES = []


def say(text):
    LINES.append(text)


#: 폴더째 옮길 것. (원래 자리, 갈 자리)
FOLDERS = (
    (KK + "/HUD04", AI + "/HUD04"),
    (KK + "/Hire", AI + "/Hire"),
    (KK + "/Heads", AI + "/Portraits"),
    (KK + "/Fonts", FONTS),
)

#: 낱개로 옮길 것. 굽는 코드가 이름으로 물고 있는 것들이다.
LOOSE = (
    ("KK_Badge_Round", AI + "/Common"),
    ("KK_Bar_Track_Link", AI + "/Common"),
    ("KK_Bar_Link", AI + "/Common"),
)

#: 유닛 데이터가 물고 있는 초상 중 루트에 있는 것. 같이 Portraits 로.
#:
#: 열일곱 중 여섯은 Heads 에, 열하나는 루트에 흩어져 있다. 한 곳에 모아야
#: 다음에 찾을 때 두 군데를 안 뒤진다.
PORTRAIT_ROOT = AI + "/Portraits"


def move_folder(src, dest):
    if not unreal.EditorAssetLibrary.does_directory_exist(src):
        say("건너뜀(없음): %s" % src)
        return 0
    count = len(unreal.EditorAssetLibrary.list_assets(src, recursive=True))
    if not unreal.EditorAssetLibrary.rename_directory(src, dest):
        say("못 옮김: %s -> %s" % (src, dest))
        return 0
    say("옮김 %-46s -> %-46s %3d개" % (src, dest, count))
    return count


def move_asset(name, src_dir, dest_dir):
    src = "%s/%s" % (src_dir, name)
    if not unreal.EditorAssetLibrary.does_asset_exist(src):
        return 0
    dest = "%s/%s" % (dest_dir, name)
    if unreal.EditorAssetLibrary.does_asset_exist(dest):
        say("이미 있음: %s" % dest)
        return 0
    if not unreal.EditorAssetLibrary.rename_asset(src, dest):
        say("못 옮김: %s" % src)
        return 0
    return 1


def used_portraits():
    """유닛 데이터가 물고 있는 초상 이름."""
    found = set()
    for path in unreal.EditorAssetLibrary.list_assets(
            "/Game/BP/DataAsset/Unit", recursive=True):
        asset = unreal.EditorAssetLibrary.load_asset(path.split(".")[0])
        if asset is None:
            continue
        try:
            art = asset.get_editor_property("m_portrait")
        except Exception:
            continue
        if art is not None:
            found.add(art.get_name())
    return sorted(found)


moved = 0
for src, dest in FOLDERS:
    moved += move_folder(src, dest)

for name, dest in LOOSE:
    moved += move_asset(name, KK, dest)

# 루트에 흩어진 초상을 모은다. 폴더를 옮긴 뒤라 Heads 것은 이미 가 있다.
loose_faces = 0
for name in used_portraits():
    loose_faces += move_asset(name, KK, PORTRAIT_ROOT)
say("루트에 있던 초상 %d개를 모았다" % loose_faces)
moved += loose_faces

# 리다이렉터를 정리한다. 안 하면 옛 자리가 유령으로 남아 SVN 에도 올라간다.
unreal.AssetToolsHelpers.get_asset_tools().fixup_referencers(
    [unreal.EditorAssetLibrary.load_asset(p.split(".")[0])
     for p in unreal.EditorAssetLibrary.list_assets(
         "/Game/SVN/OutSideAsset", recursive=True)
     if p.endswith("ObjectRedirector")], False)

say("")
say("모두 %d개 옮겼다" % moved)
io.open(REPORT, "w", encoding="utf-8", newline="\n").write("\n".join(LINES))
