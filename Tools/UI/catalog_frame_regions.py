"""Measure what layout each frame asset actually dictates, and write a catalogue.

프레임은 '있다/없다'가 아니라 '어떤 칸을 강제하는가'로 골라야 한다. 그림에 박힌
분할선을 재서, 그 프레임을 쓰면 콘텐츠를 어디에 둘 수 있는지 비율로 적어 둔다.
비율이라 어떤 크기로 늘려 붙여도 그대로 쓸 수 있다.

Run with plain python:
    python Tools/UI/catalog_frame_regions.py
"""

from __future__ import annotations

import json
from pathlib import Path

AUDIT = Path("D:/UnrealProjects/P_RD_develop_20260803/Saved/LegacyAudit")
DUMP = AUDIT / "FrameDump"
INDEX_PATH = AUDIT / "frame_assets.json"
OUT_JSON = AUDIT / "frame_regions.json"
OUT_MD = Path("D:/UnrealProjects/P_RD_develop_20260803/Tools/UI/FRAME_CATALOG.md")

PROFILE_W, PROFILE_H = 480, 270
MIN_CELL = 0.07          # 이보다 좁은 칸은 장식 틈이지 콘텐츠 칸이 아니다


def bands(values: list[float], min_gap: int) -> list[tuple[int, int]]:
    lo, hi = min(values), max(values)
    if hi - lo < 4.0:
        return []
    threshold = lo + (hi - lo) * 0.42
    out, start = [], None
    for index, value in enumerate(values):
        if value < threshold and start is None:
            start = index
        elif value >= threshold and start is not None:
            if index - start >= 2:
                out.append((start, index))
            start = None
    if start is not None:
        out.append((start, len(values)))
    merged = []
    for band in out:
        if merged and band[0] - merged[-1][1] < min_gap:
            merged[-1] = (merged[-1][0], band[1])
        else:
            merged.append(band)
    return merged


def spans(divider_bands, total) -> list[tuple[float, float]]:
    starts = [0] + [band[1] for band in divider_bands]
    ends = [band[0] for band in divider_bands] + [total]
    result = []
    for a, b in zip(starts, ends):
        if (b - a) / total >= MIN_CELL:
            result.append((round(a / total, 3), round(b / total, 3)))
    return result


def classify(aspect, columns, rows) -> str:
    if aspect >= 3.4:
        return "가로 띠 (제목판/스탯 스트립/행)"
    if aspect >= 2.0 and len(rows) <= 1:
        return "버튼/행 판"
    if 0.9 <= aspect <= 1.1:
        return "정사각 슬롯 (아이콘 홀더)"
    if aspect <= 0.8:
        return "세로 기둥 (명단/파티 열)"
    if len(columns) >= 3:
        return f"{len(columns)}열 전면 판"
    if len(columns) == 2:
        return "2열 전면 판"
    if len(rows) >= 2:
        return f"{len(rows)}단 판"
    return "단일 판"


def main() -> None:
    from PIL import Image

    index = json.loads(INDEX_PATH.read_text(encoding="utf-8"))
    results = []
    for entry in index:
        if not entry.get("png"):
            continue
        path = DUMP / entry["png"]
        if not path.is_file():
            continue
        with Image.open(path) as source:
            image = source.convert("RGB").resize((PROFILE_W, PROFILE_H), Image.LANCZOS)
        pixels = image.load()

        col = []
        for x in range(PROFILE_W):
            total = 0
            for y in range(int(PROFILE_H * 0.22), int(PROFILE_H * 0.88), 2):
                r, g, b = pixels[x, y]
                total += (r * 299 + g * 587 + b * 114) // 1000
            col.append(total / len(range(int(PROFILE_H * 0.22), int(PROFILE_H * 0.88), 2)))
        row = []
        for y in range(PROFILE_H):
            total = 0
            for x in range(int(PROFILE_W * 0.06), int(PROFILE_W * 0.94), 2):
                r, g, b = pixels[x, y]
                total += (r * 299 + g * 587 + b * 114) // 1000
            row.append(total / len(range(int(PROFILE_W * 0.06), int(PROFILE_W * 0.94), 2)))

        columns = spans(bands(col, 6), PROFILE_W)
        rows = spans(bands(row, 6), PROFILE_H)
        aspect = entry["aspect"] or 1.0
        results.append({
            "name": entry["name"], "asset": entry["asset"], "folder": entry["folder"],
            "size": entry["size"], "aspect": aspect,
            "columns": columns, "rows": rows,
            "kind": classify(aspect, columns, rows),
        })

    OUT_JSON.write_text(json.dumps(results, ensure_ascii=False, indent=2), encoding="utf-8")

    def fmt(spans_list):
        return " | ".join(f"{a:.2f}~{b:.2f}" for a, b in spans_list) or "-"

    groups: dict[str, list[dict]] = {}
    for item in results:
        groups.setdefault(item["kind"], []).append(item)

    lines = [
        "# 프레임 에셋 카탈로그 — 각 프레임이 강제하는 칸",
        "",
        "`Tools/UI/catalog_frame_regions.py` 가 그림의 밝기 프로파일에서 분할선을 찾아",
        "**콘텐츠를 놓을 수 있는 칸을 비율(0~1)로** 적은 것이다. 비율이라 어떤 크기로",
        "늘려 붙여도 그대로 환산해 쓸 수 있다.",
        "",
        "> 프레임을 고를 때는 **화면에 필요한 칸 수**를 먼저 정하고 그 칸을 가진 프레임을",
        "> 골라야 한다. 반대로 하면(프레임 먼저, 배치 나중) 지금처럼 섹션이 분할선을 밟는다.",
        "",
    ]
    order = ["3열 전면 판", "4열 전면 판", "2열 전면 판", "단일 판", "2단 판", "3단 판",
             "세로 기둥 (명단/파티 열)", "가로 띠 (제목판/스탯 스트립/행)",
             "버튼/행 판", "정사각 슬롯 (아이콘 홀더)"]
    for kind in order + [k for k in groups if k not in order]:
        items = groups.get(kind)
        if not items:
            continue
        lines.append(f"## {kind}  ({len(items)}종)")
        lines.append("")
        lines.append("| 에셋 | 폴더 | 크기 | 비율 | 세로 칸 (x) | 가로 칸 (y) |")
        lines.append("|---|---|---|---|---|---|")
        for item in sorted(items, key=lambda x: -x["size"][0] * x["size"][1]):
            lines.append(
                f"| `{item['name']}` | {item['folder']} | {item['size'][0]}×{item['size'][1]} "
                f"| {item['aspect']:.2f} | {fmt(item['columns'])} | {fmt(item['rows'])} |")
        lines.append("")

    OUT_MD.write_text("\n".join(lines), encoding="utf-8")
    print(f"analysed {len(results)} frames -> {OUT_MD}")
    for kind in order:
        if groups.get(kind):
            print(f"  {kind}: {len(groups[kind])}")


if __name__ == "__main__":
    main()
