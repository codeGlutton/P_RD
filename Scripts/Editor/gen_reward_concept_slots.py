# -*- coding: utf-8 -*-
"""reward-flow-slots.json -> RewardConceptSlots.inl 코드젠.

시안 02/03/06의 슬롯 좌표를 C++ 정적 테이블로 변환한다. 좌표를 손으로
옮기다 어긋나는 사고를 막기 위해 항상 이 스크립트로만 재생성한다.

사용법: python Scripts/Editor/gen_reward_concept_slots.py [slots.json 경로]
"""
import json
import os
import sys

DEFAULT_SRC = (r"C:\Users\2009e\.codex\visualizations\2026\08\15"
               r"\01a00649-56ee-7693-b057-d255eb583395\reward-flow-slots.json")
PROJECT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
OUT = os.path.join(PROJECT, "Source", "P_RDEditor", "UI", "RewardConceptSlots.inl")

CONCEPTS = ("02_bottom_sheet", "03_left_progress_rail", "06_loot_table")
STAGES = ("experience", "chest", "gold", "artifact")
DECO = {"none": 0, "weak": 1, "medium": 2, "strong": 3}

src = sys.argv[1] if len(sys.argv) > 1 else DEFAULT_SRC
data = json.load(open(src, encoding="utf-8"))

lines = [
    "// Scripts/Editor/gen_reward_concept_slots.py 가 reward-flow-slots.json 에서",
    "// 생성한 파일. 손으로 수정하지 말 것 — 좌표 수정은 JSON 을 고치고 재생성.",
    "// clang-format off",
    "",
]
stage_entries = []
for concept in CONCEPTS:
    for stage in STAGES:
        entry = next(e for e in data
                     if e["concept"] == concept and e["stage"] == stage)
        var = f"GSlots_{concept.split('_')[0]}_{stage}"
        lines.append(f"static const FConceptSlot {var}[] =")
        lines.append("{")
        for p in entry["parts"]:
            lines.append(
                "\t{{ TEXT(\"{name}\"), {x:.0f}.f, {y:.0f}.f, {w:.0f}.f, {h:.0f}.f, {layer}, {deco}, {text} }},".format(
                    name=p["name"], x=p["x"], y=p["y"], w=p["w"], h=p["h"],
                    layer=p["layer"], deco=DECO[p["decoration"]],
                    text="true" if p["runtime_text"] else "false"))
        lines.append("};")
        lines.append("")
        stage_entries.append((concept, stage, var))

lines.append("static const FConceptStage GConceptStages[] =")
lines.append("{")
for concept, stage, var in stage_entries:
    lines.append(f"\t{{ TEXT(\"{concept}\"), TEXT(\"{stage}\"), {var}, UE_ARRAY_COUNT({var}) }},")
lines.append("};")
lines.append("// clang-format on")
lines.append("")

with open(OUT, "w", encoding="utf-8", newline="\n") as f:
    f.write("\n".join(lines))
print(f"{len(stage_entries)} stages -> {OUT}")
