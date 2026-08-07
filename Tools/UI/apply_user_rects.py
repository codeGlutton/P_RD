"""Fold the hand-adjusted regions into the manifest the builders read.

무엇을 하나
-----------
목록 페이지에서 사람이 맞춘 **칸**(글자·아이콘을 놓아도 테두리를 안 밟는 자리)을
``kit_manifest_a.INNER`` 에 넣는다. 배치 빌더는 이미 그 표를 보고 자리를 잡으므로,
표만 갈아 끼우면 화면이 따라온다.

어느 값을 쓰나
--------------
    1. 사람이 고친 것(rects_user.json)   -- 가장 믿을 만하다
    2. 잰 것(assets.json 의 holes)       -- 사람이 안 고친 것
    3. 없으면 표에서 뺀다                -- 짐작하지 않는다. 부르는 쪽이 기본값을 쓴다

칸이 여럿인 그림은 **첫 칸**을 쓴다. 부품 하나에 자리 하나가 KitA 의 규칙이고,
여러 칸이 필요한 그림(옵션 레일 4칸 같은 것)은 배치가 직접 나눠 쓴다.

Run with plain python:
    python Tools/UI/apply_user_rects.py
"""

import json
import re
from pathlib import Path

ROOT = Path("D:/UnrealProjects/P_RD_develop_20260803")
USER = ROOT / "Tools/UI/mockups/rects_user.json"
MEASURED = ROOT / "Tools/UI/mockups/assets.json"
MANIFEST = ROOT / "Tools/UI/kit_manifest_a.py"
REPORT = ROOT / "Saved/LegacyAudit/apply_rects.txt"


def first_rect(holes):
    if not holes:
        return None
    hole = holes[0]
    return hole.get("rect") or hole.get("inner") or hole.get("box")


def main():
    user = json.loads(USER.read_text(encoding="utf-8")) if USER.is_file() else {}
    measured = {a["name"]: a for a in json.loads(MEASURED.read_text(encoding="utf-8"))}

    text = MANIFEST.read_text(encoding="utf-8")
    # 표(INNER) **앞쪽**에서만 이름을 찾는다. 표 자체를 읽으면 지난번에 넣은
    # 이름이 그대로 살아남아, 부품 목록에서 뺀 것이 안 빠진다.
    # KitA 밖에서 빌려 온 판(T_MB_...)도 자리가 필요하므로 T_ 로 넓힌다.
    head = text[:text.index("# ── 부품 안")] if "# ── 부품 안" in text else text
    wanted = sorted(set(re.findall(r'"(T_[A-Za-z0-9_]+)"', head)))

    rows, lines = {}, []
    for name in wanted:
        rect, source = None, None
        if name in user:
            rect, source = first_rect(user[name]), "사람이 맞춤"
        if rect is None and name in measured:
            rect, source = first_rect(measured[name].get("holes")), "잰 값"
        if rect is None:
            lines.append(f"{name:32s} 자리 없음 -- 표에서 뺀다")
            continue
        # 원본 크기도 같이 적는다. 9-slice 부품은 테두리가 원본 픽셀 크기로
        # 그려지므로, 비율만으로는 늘려 놓은 자리의 안쪽을 못 구한다.
        size = measured.get(name, {}).get("size")
        if not size:
            lines.append(f"{name:32s} 원본 크기를 모름 -- 표에서 뺀다")
            continue
        rows[name] = ([round(v, 4) for v in rect], [int(size[0]), int(size[1])])
        lines.append(f"{name:32s} {source:8s} "
                     f"({rect[0]:.3f},{rect[1]:.3f})-({rect[2]:.3f},{rect[3]:.3f})"
                     f"  원본 {size[0]}x{size[1]}")

    body = ['''
# ── 부품 안에 글자·아이콘을 놓아도 되는 자리 ──────────────────────────
#
# 목록 페이지(assets.html)에서 눈으로 맞춘 값이다. 예전에는 `chip * 0.20` 처럼
# 비율로 짐작했고, 재 보니 칩 글자가 링을 밟고 초상화가 사방 11% 나무 위로
# 올라가 있었다.
#
# 값은 **원본 텍스처 기준 비율**이다. 뒤의 두 수는 그 원본 크기다 -- 9-slice
# 부품은 테두리가 원본 픽셀 크기 그대로 그려지므로, 늘려 놓은 자리의 안쪽을
# 구하려면 비율이 아니라 픽셀로 환산해야 한다(kit_brush.inner_rect).
#
# apply_user_rects.py 가 다시 만든다. 손으로 고치지 말 것 -- 페이지에서 고치면
# 여기로 들어온다.
INNER = {''']
    for name in sorted(rows):
        (left, top, right, bottom), (source_w, source_h) = rows[name]
        body.append(f'    "{name}": ({left}, {top}, {right}, {bottom},'
                    f' {source_w}, {source_h}),')
    body.append("}\n\n\ndef inner_ratio(name):\n"
                '    """자리 비율. 없으면 None -- 부르는 쪽이 기본값을 쓴다."""\n'
                "    entry = INNER.get(name)\n"
                "    return entry[:4] if entry is not None else None\n"
                "\n\ndef inner_source(name):\n"
                '    """그 비율을 잰 원본 크기. 9-slice 를 픽셀로 환산할 때 쓴다."""\n'
                "    entry = INNER.get(name)\n"
                "    return entry[4:6] if entry is not None else None\n")

    # 표만 갈아 끼운다. 첫 실행 때는 옛 제목이라 이름으로 찾으면 두 번째
    # 실행에서 못 찾는다(실제로 그랬다). 사이에 낀 것이 INNER 하나뿐이므로
    # 앞뒤 고정된 것을 집는다.
    start = text.index("\n# ──", text.index("PAD_BY_NAME = {"))
    end = text.index("# 부품 시트에 없어서 화면 시안")
    MANIFEST.write_text(text[:start] + "\n".join(body) + "\n\n" + text[end:], encoding="utf-8")

    header = [f"# KitA 부품 {len(wanted)}종 중 자리를 넣은 것 {len(rows)}개",
              f"# 사람이 맞춘 것 {sum(1 for n in rows if n in user)}개", ""]
    REPORT.write_text("\n".join(header + lines) + "\n", encoding="utf-8")
    print("\n".join(header + lines))


if __name__ == "__main__":
    main()
