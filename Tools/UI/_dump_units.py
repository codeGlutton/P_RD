# -*- coding: utf-8 -*-
"""용병 데이터에셋에 무엇이 들어 있나 적어 낸다."""
import io, os
import unreal

OUT = r"D:\UnrealProjects\P_RD_develop_20260726\Tools\UI\_unit_dump.txt"
L = []
for path in sorted(unreal.EditorAssetLibrary.list_assets(
        "/Game/BP/DataAsset/Unit/PlayerUnit", recursive=True)):
    a = unreal.EditorAssetLibrary.load_asset(path.split(".")[0])
    if a is None:
        continue
    L.append("== %s" % a.get_name())
    for p in a.get_editor_property_names() if hasattr(a, "get_editor_property_names") else []:
        pass
    for key in ("m_display_name", "m_attribute_init_datas", "m_attribute_data",
                "m_skill_datas", "m_portrait", "m_view_class", "m_job_type",
                "m_attribute_set_datas", "m_max_hp"):
        try:
            v = a.get_editor_property(key)
        except Exception:
            continue
        L.append("   %-24s %s" % (key, v))
io.open(OUT, "w", encoding="utf-8", newline="\n").write("\n".join(L))
