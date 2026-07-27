"""Fit V3 monster action art to a square UI canvas without circular normalization."""

from __future__ import annotations

import json
import math
from pathlib import Path

from PIL import Image, ImageDraw


PROJECT = Path(__file__).resolve().parents[2]
ROOT = PROJECT / "SourceArt" / "UI" / "KayKitMonsterPortraitsV3"
SOURCE = ROOT / "Keyed"
OUTPUT = ROOT / "Processed"
CONTACT_SHEET = ROOT / "contact_sheet.png"
CANVAS = 256
MAX_EXTENT = 228
ALPHA_THRESHOLD = 8


def fit(path: Path) -> dict[str, object]:
    image = Image.open(path).convert("RGBA")
    alpha = image.getchannel("A")
    bbox = alpha.point(
        lambda value: 255 if value > ALPHA_THRESHOLD else 0
    ).getbbox()
    if bbox is None:
        raise RuntimeError(f"No opaque content: {path}")

    cropped = image.crop(bbox)
    scale = min(MAX_EXTENT / cropped.width, MAX_EXTENT / cropped.height)
    resized = cropped.resize(
        (
            max(1, round(cropped.width * scale)),
            max(1, round(cropped.height * scale)),
        ),
        Image.Resampling.LANCZOS,
    )
    x = (CANVAS - resized.width) // 2
    y = (CANVAS - resized.height) // 2
    canvas = Image.new("RGBA", (CANVAS, CANVAS), (0, 0, 0, 0))
    canvas.alpha_composite(resized, (x, y))

    out_bbox = canvas.getchannel("A").point(
        lambda value: 255 if value > ALPHA_THRESHOLD else 0
    ).getbbox()
    if out_bbox is None:
        raise RuntimeError(f"Fitting removed content: {path}")
    left, top, right, bottom = out_bbox
    if min(left, top, CANVAS - right, CANVAS - bottom) < 10:
        raise RuntimeError(f"{path.name}: insufficient transparent padding")

    OUTPUT.mkdir(parents=True, exist_ok=True)
    destination = OUTPUT / path.name
    canvas.save(destination, optimize=True)
    return {
        "file": destination.name,
        "canvas": [CANVAS, CANVAS],
        "alpha_bbox": list(out_bbox),
        "subject_size": [right - left, bottom - top],
        "scale": round(scale, 6),
    }


def build_contact_sheet(rows: list[dict[str, object]]) -> None:
    columns = 4
    row_count = math.ceil(len(rows) / columns)
    margin = 20
    label_height = 36
    tile_height = CANVAS + label_height
    sheet = Image.new(
        "RGBA",
        (
            columns * CANVAS + (columns + 1) * margin,
            row_count * tile_height + (row_count + 1) * margin,
        ),
        (29, 33, 40, 255),
    )
    draw = ImageDraw.Draw(sheet)
    for index, row in enumerate(rows):
        filename = str(row["file"])
        image = Image.open(OUTPUT / filename).convert("RGBA")
        x = margin + (index % columns) * (CANVAS + margin)
        y = margin + (index // columns) * (tile_height + margin)
        sheet.alpha_composite(image, (x, y))
        label = filename.removeprefix("KK_Face_Enemy_").removesuffix(
            "_ActionV3.png"
        )
        draw.text((x + 4, y + CANVAS + 8), label, fill=(238, 240, 244, 255))
    sheet.convert("RGB").save(CONTACT_SHEET, optimize=True)


def main() -> None:
    paths = sorted(SOURCE.glob("*_ActionV3.png"))
    if len(paths) != 11:
        raise RuntimeError(f"Expected 11 keyed monster actions, found {len(paths)}")
    rows = [fit(path) for path in paths]
    build_contact_sheet(rows)
    (OUTPUT / "manifest.json").write_text(
        json.dumps(rows, ensure_ascii=False, indent=2),
        encoding="utf-8",
    )
    print(json.dumps(rows, ensure_ascii=False, indent=2))


if __name__ == "__main__":
    main()
