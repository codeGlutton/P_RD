"""First-pass classification of every UI texture: 아이콘 / 초상화 / 프레임 / 미분류.

왜 셋뿐인가
-----------
처음에 아홉 갈래로 나누고 분류마다 다른 것을 점검하려 했는데, 그게 틀렸다.
**비율은 어느 그림이든 망가지면 안 된다.** "늘려도 장식이 안 뭉개지나" 는 판에만
해당하는 물음이 아니라 전부에 해당한다. 그러니 그 점검을 분류로 나눌 이유가 없다.

남는 물음은 하나다 -- **이 그림을 어떻게 놓아야 하는가.**

    프레임   구멍이 있다. 내용이 그 구멍 안으로 들어간다
    초상화   사람·괴물의 그림. 잘리면 얼굴이 잘리므로 비율을 반드시 지킨다
    아이콘   작은 상징. 늘 같은 크기로 그린다
    미분류   위 셋이 아니거나 애매한 것. 사람이 봐야 한다

애매하면 미분류로 둔다. 틀린 분류는 짐작을 사실처럼 만들어 더 나쁘다.

무엇을 보고 정하나
------------------
이름만 보면 틀린다. ``T_Hire_PartySlot_V11`` 은 이름이 "슬롯" 인데 668x1358 이라
작은 칸이 아니다. 그래서 **잰 값을 먼저** 보고 이름은 거들게만 쓴다.

규칙은 결과 파일에 같이 적는다. 왜 그렇게 골랐는지 보이지 않으면 고칠 수 없다.
"""

import json
import re
from pathlib import Path

ROOT = Path("D:/UnrealProjects/P_RD_develop_20260803")
SOURCE = ROOT / "Tools/UI/mockups/assets.json"
OUT = ROOT / "Tools/UI/mockups/cats.json"
REPORT = ROOT / "Saved/LegacyAudit/classify.txt"

PORTRAIT = re.compile(
    r"(face|portrait|illust|hero|head|action_?v|knight|mage|rogue|archer|druid|"
    r"barbarian|ranger|slime|mushroom|spider|eagle|werewolf|golem|necromancer)",
    re.IGNORECASE)
ICON = re.compile(r"(icon|glyph|symbol|d6_face|dice|pip|dot|marker|badge)",
                  re.IGNORECASE)
# 초상화로 보이기 쉬운데 아닌 것들. 이름에 직업이 들어간 틀·카드가 많다.
NOT_PORTRAIT = re.compile(r"(frame|panel|plate|card|slot|socket|button|btn|bar|"
                          r"board|shell|scrim|banner|row|strip|tray|bg|background)",
                          re.IGNORECASE)

ICON_MAX = 320          # 긴 변이 이보다 크면 아이콘이라 보기 어렵다
SQUARE = 0.25           # 가로세로 차이가 이 비율 안이면 정사각으로 본다


def decide(entry):
    """(분류, 왜) 를 돌려준다. 애매하면 미분류."""
    name = entry["name"]
    width, height = entry["size"]
    holes = len(entry.get("holes") or [])
    longest = max(width, height)
    squarish = abs(width - height) <= longest * SQUARE

    # 1) 구멍이 있으면 프레임이다. 잰 값이라 이름보다 믿을 만하다.
    if holes:
        return "프레임", f"구멍 {holes}개"

    # 2) 사람·괴물 그림. 틀 이름이 섞여 있으면 초상화가 아니다.
    if PORTRAIT.search(name) and not NOT_PORTRAIT.search(name):
        return "초상화", "이름이 인물·몬스터"

    # 3) 작고 정사각에 가까운 상징.
    if ICON.search(name) and not NOT_PORTRAIT.search(name):
        return "아이콘", "이름이 아이콘 계열"
    if squarish and longest <= ICON_MAX:
        return "아이콘", f"정사각 {width}x{height}"

    # 4) 나머지는 사람이 봐야 한다.
    return None, "구멍 없음 · 이름으로 못 가림"


def main():
    data = json.loads(SOURCE.read_text(encoding="utf-8"))
    cats, rows, counts = {}, [], {}
    for entry in data:
        if not entry.get("size"):
            continue
        kind, why = decide(entry)
        if kind:
            cats[entry["name"]] = kind
        label = kind or "미분류"
        counts[label] = counts.get(label, 0) + 1
        rows.append(f"{label:5s}  {entry['name']:46s} "
                    f"{entry['size'][0]:5d}x{entry['size'][1]:<5d} "
                    f"구멍{len(entry.get('holes') or [])} "
                    f"{'9slice' if entry.get('slice') else '늘리지말것':10s}  {why}")

    OUT.write_text(json.dumps(cats, ensure_ascii=False, indent=1), encoding="utf-8")
    header = ["# 첫 분류. 애매한 것은 미분류로 두었다 -- 사람이 고칠 자리다.",
              "# " + " · ".join(f"{k} {v}" for k, v in sorted(counts.items())),
              ""]
    REPORT.write_text("\n".join(header + sorted(rows)) + "\n", encoding="utf-8")
    print(" · ".join(f"{k} {v}" for k, v in sorted(counts.items())))


if __name__ == "__main__":
    main()
