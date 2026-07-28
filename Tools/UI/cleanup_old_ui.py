# -*- coding: utf-8 -*-
"""옮기고 남은 옛 자리를 지운다. 참조가 새 자리를 보는지 먼저 확인한다.

rename_directory 가 옮기기는 했는데 옛 파일을 못 지웠다. 같은 크기 그대로
남아서 SVN 에 올리면 같은 그림이 두 벌 들어간다.

지우기 전에 유닛 데이터가 물고 있는 초상이 **새 자리**를 보는지 본다. 옛
자리를 아직 보고 있으면 지우는 순간 열일곱이 통째로 빈다.

    UnrealEditor-Cmd.exe <uproject> -run=pythonscript -script=cleanup_old_ui.py
"""
import io
import os

import unreal

KK = "/Game/SVN/OutSideAsset/UI/KayKit"
REPORT = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                      "_cleanup_report.txt")
LINES = []


def say(text):
    LINES.append(text)


def portrait_paths():
    """유닛 데이터가 물고 있는 초상의 **경로**."""
    found = {}
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
            found[asset.get_name()] = art.get_path_name()
    return found


before = portrait_paths()
old = {k: v for k, v in before.items() if "/KayKit/" in v}
say("초상 %d개 중 옛 자리를 보는 것: %d개" % (len(before), len(old)))
for name, path in sorted(old.items())[:5]:
    say("   %s -> %s" % (name, path))

if old:
    say("")
    say("옛 자리를 보는 것이 남아 있어 지우지 않는다.")
else:
    gone = 0
    for folder in ("HUD04", "Hire", "Heads", "Fonts"):
        target = "%s/%s" % (KK, folder)
        if not unreal.EditorAssetLibrary.does_directory_exist(target):
            continue
        count = len(unreal.EditorAssetLibrary.list_assets(target,
                                                          recursive=True))
        if unreal.EditorAssetLibrary.delete_directory(target):
            say("지움 %-40s %3d개" % (target, count))
            gone += count
        else:
            say("못 지움 %s" % target)
    say("")
    say("모두 %d개 지웠다" % gone)

io.open(REPORT, "w", encoding="utf-8", newline="\n").write("\n".join(LINES))
