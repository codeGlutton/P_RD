# -*- coding: utf-8 -*-
"""속성 초기값 표(CCT_Attribute)에 어떤 줄이 있나 적어 낸다."""
import io
import unreal
OUT = r"D:\UnrealProjects\P_RD_develop_20260726\Tools\UI\_curve_dump.txt"
L = []
t = unreal.EditorAssetLibrary.load_asset("/Game/BP/Curve/CCT_Attribute")
names = unreal.DataTableFunctionLibrary.get_data_table_row_names(t) \
    if hasattr(unreal, "DataTableFunctionLibrary") else []
try:
    names = list(t.get_editor_property("row_map").keys())
except Exception:
    pass
if not names:
    try:
        names = unreal.CurveTableFunctionLibrary.get_curve_table_row_names(t)
    except Exception:
        names = []
L.append("CCT_Attribute 줄 %d개" % len(names))
for n in sorted(str(x) for x in names):
    L.append("   %s" % n)
# 스폰 데이터의 KeyName 도 같이
L.append("")
for path in sorted(unreal.EditorAssetLibrary.list_assets(
        "/Game/BP/DataAsset/Unit/PlayerUnit", recursive=True)):
    a = unreal.EditorAssetLibrary.load_asset(path.split(".")[0])
    if a is None:
        continue
    try:
        key = a.get_key_name()
    except Exception:
        key = "(못 읽음)"
    L.append("%-28s KeyName=%s  model=%s" % (
        a.get_name(), key, a.get_editor_property("m_model_class")))
io.open(OUT, "w", encoding="utf-8", newline="\n").write("\n".join(L))
