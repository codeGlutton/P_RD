# -*- coding: utf-8 -*-
"""reward-c03-wireframe.json -> RewardC03Slots.inl 코드젠. 좌표는 JSON만이 원본."""
import json
import os

PROJECT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
SRC = os.path.join(PROJECT, "Saved", "DesignAssets", "RewardChosenC03", "reward-c03-wireframe.json")
OUT = os.path.join(PROJECT, "Source", "P_RDEditor", "UI", "RewardC03Slots.inl")
DECO = {"none": 0, "weak": 1, "medium": 2, "strong": 3}

data = json.load(open(SRC, encoding="utf-8"))
lines = [
    "// Scripts/Editor/gen_c03_slots.py 가 reward-c03-wireframe.json 에서 생성.",
    "// 손으로 수정하지 말 것 — 좌표 수정은 JSON 을 고치고 재생성.",
    "// clang-format off",
    "",
]
entries = []
for e in data:
    var = f"GSlotsC03_{e['stage']}"
    lines.append(f"static const FConceptSlot {var}[] =")
    lines.append("{")
    for p in e["parts"]:
        lines.append(
            "\t{{ TEXT(\"{name}\"), {x}.f, {y}.f, {w}.f, {h}.f, {layer}, {deco}, {text} }},".format(
                name=p["name"], x=p["x"], y=p["y"], w=p["w"], h=p["h"],
                layer=p["layer"], deco=DECO[p["decoration"]],
                text="true" if p["runtime_text"] else "false"))
    lines.append("};")
    lines.append("")
    entries.append((e["stage"], var))

lines.append("static const FConceptStage GC03Stages[] =")
lines.append("{")
for stage, var in entries:
    lines.append(f"\t{{ TEXT(\"c03_battle_modal\"), TEXT(\"{stage}\"), {var}, UE_ARRAY_COUNT({var}) }},")
lines.append("};")
lines.append("// clang-format on")
lines.append("")

with open(OUT, "w", encoding="utf-8", newline="\n") as f:
    f.write("\n".join(lines))
print(f"{len(entries)} stages -> {OUT}")
