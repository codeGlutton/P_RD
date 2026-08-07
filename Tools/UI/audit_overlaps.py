"""겹쳐 있어 UMG 에서 집기 힘든 위젯을 찾는다.

왜
--
판을 열어 고치려면 **그 위젯을 눌러 고를 수 있어야** 한다. 같은 자리에 여러
개가 포개져 있으면 늘 맨 위 것만 잡히고, 아래 것은 계층 목록에서 이름으로
찾아 들어가야 한다. 화면 전체를 덮는 빈 캔버스가 서넛 겹쳐 있으면 사실상
마우스로는 아무것도 못 고른다.

무엇을 겹침으로 보나
--------------------
같은 캔버스의 두 위젯이

    * 서로 **거의 같은 자리**(겹치는 넓이가 작은 쪽의 90% 이상)이고
    * 둘 다 눈에 보일 만한 크기다

이면 하나로 센다. 판 전체를 덮는 것(스크림·틀)은 원래 그런 것이므로 따로
표시하고, 그 위에 또 덮개가 몇 겹인지를 센다.

Run with plain python:
    python Tools/UI/audit_overlaps.py
"""

import json
from collections import defaultdict
from pathlib import Path

WORKSPACE = Path("D:/UnrealProjects_WBP_Editor/data/workspace.json")
REPORT = Path("D:/UnrealProjects/P_RD_develop_20260803/Saved/LegacyAudit/overlaps.txt")

# 두 사각이 "같은 자리" 인지는 **합집합 대비 교집합**으로 본다.
#
# 처음에는 "겹치는 넓이가 작은 쪽의 90% 이상" 으로 뒀다. 그러면 화면 전체를
# 덮는 배경이 작은 위젯 전부와 같은 자리로 잡혀, 한 캔버스가 92겹으로 나왔다.
# 작은 것이 큰 것 안에 든 것은 겹침이 아니라 **담긴** 것이다.
SAME = 0.80
TINY = 4.0        # 이보다 작으면 안 본다(자리만 잡아 둔 것)
# 시안 폴더는 안 본다. 안 쓰는 판이라 고칠 일이 없다.
SKIP = "/Game/UI/Concepts"


def area(rect):
    return max(0.0, rect["w"]) * max(0.0, rect["h"])


def overlap(a, b):
    left = max(a["x"], b["x"])
    top = max(a["y"], b["y"])
    right = min(a["x"] + a["w"], b["x"] + b["w"])
    bottom = min(a["y"] + a["h"], b["y"] + b["h"])
    return max(0.0, right - left) * max(0.0, bottom - top)


def same_place(a, b):
    """합집합 대비 교집합. 크기와 자리가 둘 다 비슷해야 높아진다."""
    share = overlap(a, b)
    union = area(a) + area(b) - share
    return share / union if union > 0 else 0.0


def main():
    data = json.loads(WORKSPACE.read_text(encoding="utf-8"))
    lines, worst = [], []

    for document in data.get("documents", []):
        if document.get("sourceKind") != "current-develop-wbp":
            continue
        if document.get("assetPath", "").startswith(SKIP):
            continue
        widgets = [w for w in document.get("widgets", [])
                   if (w.get("rect") or {}).get("w", 0) > TINY
                   and (w.get("rect") or {}).get("h", 0) > TINY]
        if len(widgets) < 2:
            continue

        # 같은 자리끼리 묶는다.
        groups = defaultdict(list)
        for index, widget in enumerate(widgets):
            placed = False
            for key, members in groups.items():
                other = widgets[key]
                if same_place(widget["rect"], other["rect"]) >= SAME:
                    members.append(widget)
                    placed = True
                    break
            if not placed:
                groups[index] = [widget]

        stacks = [(key, members) for key, members in groups.items() if len(members) >= 3]
        if not stacks:
            continue

        asset = document.get("assetPath", "?")
        canvas = document.get("canvasName", "?")
        for key, members in sorted(stacks, key=lambda kv: -len(kv[1])):
            rect = widgets[key]["rect"]
            names = [m.get("name", "?") for m in members]
            worst.append((len(members), asset, canvas, rect, names))

    worst.sort(key=lambda row: -row[0])
    lines.append(f"# 같은 자리에 셋 이상 포개진 곳 {len(worst)}군데")
    lines.append("# UMG 에서 맨 위 것만 잡힌다 -- 아래 것은 계층 목록으로만 고칠 수 있다.")
    lines.append("")
    for count, asset, canvas, rect, names in worst:
        lines.append(f"{count}겹  {asset.rsplit('/', 1)[-1]} / {canvas}"
                     f"  ({rect['x']:.0f},{rect['y']:.0f} {rect['w']:.0f}x{rect['h']:.0f})")
        for name in names:
            lines.append(f"        {name}")

    REPORT.parent.mkdir(parents=True, exist_ok=True)
    REPORT.write_text("\n".join(lines) + "\n", encoding="utf-8")
    print("\n".join(lines[:60]))
    print(f"\n... 전체는 {REPORT}")


if __name__ == "__main__":
    main()
