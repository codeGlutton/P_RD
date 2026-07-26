"""Normalize keyed ImageGen portraits to the KayKit 256px ring contract."""

from __future__ import annotations

import json
import math
from pathlib import Path

from PIL import Image


PROJECT = Path(__file__).resolve().parents[2]
SOURCE = PROJECT / "SourceArt" / "UI" / "KayKitMercenaryPortraitsV1" / "Keyed"
OUTPUT = (
    PROJECT / "SourceArt" / "UI" / "KayKitMercenaryPortraitsV1" / "Processed"
)
PREVIEW = (
    PROJECT / "SourceArt" / "UI" / "KayKitMercenaryPortraitsV1" / "preview.png"
)
CANVAS = 256
TARGET_RADIUS = 85.0
ALPHA_THRESHOLD = 8


def normalize(path: Path) -> dict[str, object]:
    image = Image.open(path).convert("RGBA")
    alpha = image.getchannel("A")
    bbox = alpha.point(lambda value: 255 if value > ALPHA_THRESHOLD else 0).getbbox()
    if bbox is None:
        raise RuntimeError(f"No opaque content: {path}")

    pixels = alpha.load()
    left, top, right, bottom = bbox
    center_x = (left + right - 1) * 0.5
    center_y = (top + bottom - 1) * 0.5
    radius = 0.0
    for y in range(top, bottom):
        for x in range(left, right):
            if pixels[x, y] > ALPHA_THRESHOLD:
                radius = max(radius, math.hypot(x - center_x, y - center_y))
    if radius <= 0.0:
        raise RuntimeError(f"Invalid alpha radius: {path}")

    scale = TARGET_RADIUS / radius
    resized = image.resize(
        (
            max(1, round(image.width * scale)),
            max(1, round(image.height * scale)),
        ),
        Image.Resampling.LANCZOS,
    )
    resized_center_x = center_x * scale
    resized_center_y = center_y * scale
    paste_x = round((CANVAS - 1) * 0.5 - resized_center_x)
    paste_y = round((CANVAS - 1) * 0.5 - resized_center_y)

    canvas = Image.new("RGBA", (CANVAS, CANVAS), (0, 0, 0, 0))
    canvas.alpha_composite(resized, (paste_x, paste_y))

    out_alpha = canvas.getchannel("A")
    out_bbox = out_alpha.point(
        lambda value: 255 if value > ALPHA_THRESHOLD else 0
    ).getbbox()
    if out_bbox is None:
        raise RuntimeError(f"Normalization removed content: {path}")

    out_pixels = out_alpha.load()
    max_radius = 0.0
    for y in range(CANVAS):
        for x in range(CANVAS):
            if out_pixels[x, y] > ALPHA_THRESHOLD:
                max_radius = max(
                    max_radius,
                    math.hypot(x - (CANVAS - 1) * 0.5, y - (CANVAS - 1) * 0.5),
                )
    if max_radius > 88.0:
        raise RuntimeError(
            f"{path.name}: radius {max_radius:.2f}px exceeds 88px safety limit"
        )

    OUTPUT.mkdir(parents=True, exist_ok=True)
    destination = OUTPUT / path.name
    canvas.save(destination, optimize=True)
    return {
        "file": destination.name,
        "canvas": [CANVAS, CANVAS],
        "alpha_bbox": list(out_bbox),
        "alpha_radius": round(max_radius, 3),
        "source_radius": round(radius, 3),
        "scale": round(scale, 6),
    }


def build_preview(rows: list[dict[str, object]]) -> None:
    columns = 3
    rows_count = math.ceil(len(rows) / columns)
    margin = 24
    tile = CANVAS
    preview = Image.new(
        "RGBA",
        (
            columns * tile + (columns + 1) * margin,
            rows_count * tile + (rows_count + 1) * margin,
        ),
        (31, 35, 43, 255),
    )
    for index, row in enumerate(rows):
        portrait = Image.open(OUTPUT / str(row["file"])).convert("RGBA")
        x = margin + (index % columns) * (tile + margin)
        y = margin + (index // columns) * (tile + margin)
        preview.alpha_composite(portrait, (x, y))
    preview.convert("RGB").save(PREVIEW, optimize=True)


def main() -> None:
    rows = [normalize(path) for path in sorted(SOURCE.glob("*.png"))]
    build_preview(rows)
    manifest = OUTPUT / "manifest.json"
    manifest.write_text(
        json.dumps(rows, ensure_ascii=False, indent=2), encoding="utf-8"
    )
    print(json.dumps(rows, ensure_ascii=False, indent=2))


if __name__ == "__main__":
    main()
