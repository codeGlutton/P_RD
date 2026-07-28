# -*- coding: utf-8 -*-
"""여섯 용병의 스탯 표를 합성 표에 도로 물린다.

`CCT_Attribute` 는 합성 커브 표다. 게임은 이것 하나만 읽고, 여기 안 걸린
표는 없는 것과 같다. develop 머지에서 저쪽 `CCT_Attribute` 를 통째로 받으면서
`CT_StartingCrewAttribute` 가 목록에서 빠졌다 -- 그래서 기사 말고는 전부
체력 0/0 으로 나왔다.

    UnrealEditor-Cmd.exe <uproject> -run=pythonscript -script=fix_crew_curve.py
"""
import io
import os

import unreal

OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                   "_crew_curve.txt")
L = []

CCT = "/Game/BP/Curve/CCT_Attribute"
CREW = "/Game/BP/Curve/Player/CT_StartingCrewAttribute"

composite = unreal.EditorAssetLibrary.load_asset(CCT)
crew = unreal.EditorAssetLibrary.load_asset(CREW)

tables = list(composite.get_editor_property("parent_tables"))
L.append("지금 물린 표 %d개" % len(tables))
for t in tables:
    L.append("   %s" % (t.get_name() if t else "None"))

if crew in tables:
    L.append("\n이미 물려 있다")
else:
    tables.append(crew)
    composite.set_editor_property("parent_tables", tables)
    unreal.EditorAssetLibrary.save_asset(CCT)
    L.append("\n%s 를 물렸다 -> %d개" % (crew.get_name(), len(tables)))

io.open(OUT, "w", encoding="utf-8", newline="\n").write("\n".join(L))
