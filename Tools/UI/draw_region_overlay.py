"""Draw the frame's painted cells against the placed sections, as a PNG.

빨강 = 그림에 박힌 분할선 / 초록 = 그림이 정해 놓은 칸 / 파랑 = 내가 얹은 섹션.
파랑이 초록 안에 들어가야 맞는 배치다.
"""

from __future__ import annotations

import json
from pathlib import Path

PROJECT = Path("D:/UnrealProjects/P_RD_develop_20260803")
EDITOR = Path("D:/UnrealProjects_WBP_Editor")
SPEC_PATH = PROJECT / "Saved/LegacyAudit/variants_render_spec.json"
WORKSPACE_PATH = EDITOR / "data/workspace.json"
OUT_DIR = PROJECT / "Saved/LegacyAudit"

TARGETS = ["WBP_SkillDetail_v01", "WBP_EnemyDetail_v02"]
SECTION_NAMES = ("Section", "TopStrip", "Card")

# verify_frame_regions.py 가 측정한 값
FRAME_CELLS = {
    "T_MT_BaseFrame": [(31, 125, 466, 896), (522, 125, 554, 896), (1105, 125, 786, 896)],
}
FRAME_DIVIDERS = {
    "T_MT_BaseFrame": {"vertical": [(0, 31), (497, 522), (1076, 1105), (1891, 1920)],
                       "horizontal": [(0, 125), (1021, 1080)]},
}


def png_for(asset_path: str) -> Path | None:
    workspace = json.loads(WORKSPACE_PATH.read_text(encoding="utf-8"))
    for document in workspace.get("documents", []):
        for widget in document.get("widgets", []):
            resource, image = widget.get("resourcePath"), widget.get("image")
            if resource and image and resource.split(".")[0] == asset_path:
                return EDITOR / image
    return None


def main() -> None:
    from PIL import Image, ImageDraw

    spec = json.loads(SPEC_PATH.read_text(encoding="utf-8"))
    for asset_name in TARGETS:
        widgets = spec.get(asset_name)
        if not widgets:
            continue
        base = next((w for w in widgets
                     if w["class"] == "Image" and w["name"].startswith("BaseFrame")), None)
        key = base["texture"].split("/")[-1].split(".")[0]
        png = png_for(base["texture"].split(".")[0])
        if png is None:
            continue

        with Image.open(png) as source:
            canvas = source.convert("RGB").resize((1920, 1080), Image.LANCZOS)
        canvas = Image.blend(canvas, Image.new("RGB", canvas.size, (12, 14, 20)), 0.35)
        draw = ImageDraw.Draw(canvas, "RGBA")

        for band in FRAME_DIVIDERS[key]["vertical"]:
            draw.rectangle([band[0], 0, band[1], 1080], fill=(220, 60, 60, 110))
        for band in FRAME_DIVIDERS[key]["horizontal"]:
            draw.rectangle([0, band[0], 1920, band[1]], fill=(220, 60, 60, 110))
        for cell in FRAME_CELLS[key]:
            draw.rectangle([cell[0], cell[1], cell[0] + cell[2], cell[1] + cell[3]],
                           outline=(80, 235, 120), width=6)

        for widget in widgets:
            if not widget["name"].startswith(SECTION_NAMES):
                continue
            x, y, w, h = widget["rect"]
            if w >= 1918 and h >= 1078:
                continue
            draw.rectangle([x, y, x + w, y + h], outline=(90, 190, 255), width=5)

        out = OUT_DIR / f"region_overlay_{asset_name}.png"
        canvas.save(out)
        print(f"wrote {out}")


if __name__ == "__main__":
    main()
