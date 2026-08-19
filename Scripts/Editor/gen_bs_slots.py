# -*- coding: utf-8 -*-
"""bs-slots.json -> RewardBSSlots.inl 코드젠. JSON이 좌표의 단일 원본이다."""
import json
import os


PROJECT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
SRC = os.path.join(PROJECT, "Saved", "DesignAssets", "RewardBS", "bs-slots.json")
OUT = os.path.join(PROJECT, "Source", "P_RDEditor", "UI", "RewardBSSlots.inl")
DECO = {"none": 0, "weak": 1, "medium": 2, "strong": 3}

data = json.load(open(SRC, encoding="utf-8"))
lines = [
    "// Scripts/Editor/gen_bs_slots.py가 bs-slots.json에서 생성.",
    "// 손으로 수정하지 말 것. 좌표 수정은 JSON을 고치고 재생성.",
    "// clang-format off",
    "",
]
entries = []
for stage in data:
    variable = f"GSlotsBS_{stage['stage']}"
    lines += [f"static const FConceptSlot {variable}[] =", "{"]
    for part in stage["parts"]:
        lines.append(
            "\t{{ TEXT(\"{name}\"), {x}.f, {y}.f, {w}.f, {h}.f, {layer}, {deco}, {text} }},".format(
                name=part["name"], x=part["x"], y=part["y"], w=part["w"], h=part["h"],
                layer=part["layer"], deco=DECO[part["decoration"]],
                text="true" if part["runtime_text"] else "false"))
    lines += ["};", ""]
    entries.append((stage["stage"], variable))

lines += ["static const FConceptStage GBSStages[] =", "{"]
for stage, variable in entries:
    lines.append(
        f"\t{{ TEXT(\"bs_bottom_sheet\"), TEXT(\"{stage}\"), {variable}, UE_ARRAY_COUNT({variable}) }},")
lines += ["};", "// clang-format on", ""]

with open(OUT, "w", encoding="utf-8", newline="\n") as stream:
    stream.write("\n".join(lines))
print(f"{len(entries)} stages -> {OUT}")
