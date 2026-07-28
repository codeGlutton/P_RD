# -*- coding: utf-8 -*-
"""`/Game/BP/UI` 열일곱 장을 `/Game/UI` 로 옮긴다.

회의(0728)에서 UI 는 `Content/UI` 로 통일하기로 했다. `BP/UI` 와 `UI` 가 둘 다
있으면 다음 사람이 어느 쪽을 보는지 매번 물어야 한다.

## 왜 에디터 안에서 옮기나

손으로 옮기면 참조가 끊긴다. `WBP_CharacterSelect` 가 `WBP_CharacterCard` 를
물고 있고, 게임모드 블루프린트도 이 열일곱을 물고 있다. `rename_asset` 은
옮기면서 그 참조를 고쳐 준다.

**ini 와 C++ 의 문자열 경로는 못 고친다.** 그건 사람이 따로 고쳐야 한다 --
이 스크립트는 남은 것을 찾아서 적어 낸다.

    UnrealEditor-Cmd.exe <uproject> -run=pythonscript -script=move_bp_ui.py
"""
import io
import os

import unreal

SRC = "/Game/BP/UI"
DEST = "/Game/UI"

REPORT = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                      "_move_bp_ui.txt")
LINES = []


def say(text):
    LINES.append(text)


assets = unreal.EditorAssetLibrary.list_assets(SRC, recursive=False)
say("옮길 것 %d개" % len(assets))

moved = 0
for path in assets:
    name = path.split(".")[0].rsplit("/", 1)[-1]
    src = "%s/%s" % (SRC, name)
    dest = "%s/%s" % (DEST, name)
    if unreal.EditorAssetLibrary.does_asset_exist(dest):
        say("이미 있음, 건너뜀: %s" % dest)
        continue
    if unreal.EditorAssetLibrary.rename_asset(src, dest):
        moved += 1
    else:
        say("못 옮김: %s" % src)

say("%d개 옮겼다" % moved)

# 리다이렉터를 정리한다. 안 하면 옛 경로가 유령으로 남는다.
redirectors = [unreal.EditorAssetLibrary.load_asset(p.split(".")[0])
               for p in unreal.EditorAssetLibrary.list_assets(
                   "/Game", recursive=True)
               if p.endswith("ObjectRedirector")]
if redirectors:
    unreal.AssetToolsHelpers.get_asset_tools().fixup_referencers(
        redirectors, False)
    say("리다이렉터 %d개 정리" % len(redirectors))

# 옛 자리가 비었나. 안 비면 무엇이 남았는지 적는다.
left = unreal.EditorAssetLibrary.list_assets(SRC, recursive=True)
if left:
    say("")
    say("옛 자리에 %d개 남음:" % len(left))
    for p in left[:20]:
        say("   %s" % p)
else:
    unreal.EditorAssetLibrary.delete_directory(SRC)
    say("옛 자리를 지웠다")

io.open(REPORT, "w", encoding="utf-8", newline="\n").write("\n".join(LINES))
