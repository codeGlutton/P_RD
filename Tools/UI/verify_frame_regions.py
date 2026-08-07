"""Verify every placed section sits inside a cell the frame actually painted.

이전 검증은 '화면 밖으로 나갔는가'와 '위젯끼리 겹치는가'만 봤다. 배경 그림이
칸을 정해 놓았다는 사실을 안 봐서, 섹션 238개 중 236개가 분할선을 밟은 걸 놓쳤다.
여기서는 frame_registry 의 실측 칸을 기준으로 본다.

Run with plain python:
    python Tools/UI/verify_frame_regions.py
"""

from __future__ import annotations

import json
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from frame_registry import FRAMES, PAD, SCREEN_FRAME, column, window  # noqa: E402

AUDIT = Path("D:/UnrealProjects/P_RD_develop_20260803/Saved/LegacyAudit")
SPEC_PATH = AUDIT / "variants_render_spec.json"
OUT_PATH = AUDIT / "frame_region_report.txt"

# 배치 칸으로 다루는 위젯들. 칩/글자는 이 안에 들어가므로 섹션만 보면 된다.
SECTION_PREFIXES = ("Section", "Card", "Tab", "TopStrip")
TOLERANCE = 2.0


def inside(outer, rect, tol=TOLERANCE) -> bool:
    return (rect[0] >= outer[0] - tol and rect[1] >= outer[1] - tol
            and rect[0] + rect[2] <= outer[0] + outer[2] + tol
            and rect[1] + rect[3] <= outer[1] + outer[3] + tol)


def main() -> None:
    spec = json.loads(SPEC_PATH.read_text(encoding="utf-8"))
    lines, checked, bad = [], 0, 0

    for asset_name in sorted(spec):
        _, screen, variant = asset_name.split("_", 2)
        frame = FRAMES[SCREEN_FRAME[screen]]
        # 미니멀 안은 프레임을 걷고 평면 배경을 쓴다. 그래도 같은 x 격자를 지키는지 본다.
        flat = any(w["name"].startswith("BaseFlat") for w in spec[asset_name])

        if frame["kind"] == "cols":
            zones = [column(frame, index, 0.0) for index in range(len(frame["cols"]))]
        else:
            zones = [window(frame, 0.0)]

        problems = []
        for widget in spec[asset_name]:
            if not widget["name"].startswith(SECTION_PREFIXES):
                continue
            rect = widget["rect"]
            if rect[2] >= 1918 and rect[3] >= 1078:
                continue
            checked += 1
            if not any(inside(zone, rect) for zone in zones):
                bad += 1
                problems.append(
                    f"    {widget['name']} ({rect[0]:.0f},{rect[1]:.0f},"
                    f"{rect[2]:.0f}x{rect[3]:.0f})")
        if problems:
            lines.append(f"  {asset_name}{'  [flat]' if flat else ''}")
            lines.extend(problems)

    header = [
        "=== 프레임 칸 정렬 검증 ===",
        f"검사한 섹션 {checked}개 / 칸을 벗어난 것 {bad}개",
        "",
    ]
    OUT_PATH.write_text("\n".join(header + lines), encoding="utf-8")
    print("\n".join(header[:2]))
    if lines:
        print("\n".join(lines[:30]))
    else:
        print("모든 섹션이 프레임이 정한 칸 안에 있습니다.")


if __name__ == "__main__":
    main()
