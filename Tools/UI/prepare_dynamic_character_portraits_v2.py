"""Normalize V2 dynamic character portraits to the KayKit 256px ring contract."""

from __future__ import annotations

import json
import math
from pathlib import Path

from PIL import Image, ImageDraw


PROJECT = Path(__file__).resolve().parents[2]
ROOT = PROJECT / "SourceArt" / "UI" / "KayKitCharacterPortraitsV2"
SOURCE = ROOT / "Keyed"
OUTPUT = ROOT / "Processed"
PREVIEW = ROOT / "contact_sheet.png"
CANVAS = 256
TARGET_RADIUS = 85.0
SAFETY_RADIUS = 88.0
ALPHA_THRESHOLD = 8


def normalize(path: Path) -> dict[str, object]:
    image = Image.open(path).convert("RGBA")
    alpha = image.getchannel("A")
    mask = alpha.point(lambda value: 255 if value > ALPHA_THRESHOLD else 0)
    bbox = mask.getbbox()
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

    def render(test_scale: float) -> tuple[Image.Image, tuple[int, ...], float]:
        resized = image.resize(
            (
                max(1, round(image.width * test_scale)),
                max(1, round(image.height * test_scale)),
            ),
            Image.Resampling.LANCZOS,
        )
        paste_x = round((CANVAS - 1) * 0.5 - center_x * test_scale)
        paste_y = round((CANVAS - 1) * 0.5 - center_y * test_scale)
        result = Image.new("RGBA", (CANVAS, CANVAS), (0, 0, 0, 0))
        result.alpha_composite(resized, (paste_x, paste_y))

        result_alpha = result.getchannel("A")
        result_mask = result_alpha.point(
            lambda value: 255 if value > ALPHA_THRESHOLD else 0
        )
        result_bbox = result_mask.getbbox()
        if result_bbox is None:
            raise RuntimeError(f"Normalization removed content: {path}")
        result_pixels = result_alpha.load()
        result_radius = 0.0
        for result_y in range(CANVAS):
            for result_x in range(CANVAS):
                if result_pixels[result_x, result_y] > ALPHA_THRESHOLD:
                    result_radius = max(
                        result_radius,
                        math.hypot(
                            result_x - (CANVAS - 1) * 0.5,
                            result_y - (CANVAS - 1) * 0.5,
                        ),
                    )
        return result, result_bbox, result_radius

    scale = TARGET_RADIUS / radius
    canvas, out_bbox, max_radius = render(scale)
    # Downsampling can erase a few distant, low-alpha pixels and leave a visibly
    # smaller portrait. Correct once from the measured result so all 19 assets
    # occupy the ring consistently.
    if max_radius < TARGET_RADIUS - 1.0:
        scale *= TARGET_RADIUS / max_radius
        canvas, out_bbox, max_radius = render(scale)
    if max_radius > SAFETY_RADIUS:
        raise RuntimeError(
            f"{path.name}: radius {max_radius:.2f}px exceeds "
            f"{SAFETY_RADIUS:.0f}px safety limit"
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


def build_contact_sheet(rows: list[dict[str, object]]) -> None:
    columns = 5
    row_count = math.ceil(len(rows) / columns)
    margin = 18
    label_height = 38
    tile_height = CANVAS + label_height
    sheet = Image.new(
        "RGBA",
        (
            columns * CANVAS + (columns + 1) * margin,
            row_count * tile_height + (row_count + 1) * margin,
        ),
        (31, 35, 43, 255),
    )
    draw = ImageDraw.Draw(sheet)
    for index, row in enumerate(rows):
        filename = str(row["file"])
        portrait = Image.open(OUTPUT / filename).convert("RGBA")
        x = margin + (index % columns) * (CANVAS + margin)
        y = margin + (index // columns) * (tile_height + margin)
        sheet.alpha_composite(portrait, (x, y))
        label = filename.removeprefix("KK_Face_").removesuffix("_DynamicV2.png")
        draw.text((x + 4, y + CANVAS + 8), label, fill=(235, 238, 244, 255))
    sheet.convert("RGB").save(PREVIEW, optimize=True)


def main() -> None:
    paths = sorted(SOURCE.glob("*.png"))
    if len(paths) != 19:
        raise RuntimeError(f"Expected 19 keyed portraits, found {len(paths)}")
    rows = [normalize(path) for path in paths]
    build_contact_sheet(rows)
    manifest = OUTPUT / "manifest.json"
    manifest.write_text(
        json.dumps(rows, ensure_ascii=False, indent=2),
        encoding="utf-8",
    )
    print(json.dumps(rows, ensure_ascii=False, indent=2))


if __name__ == "__main__":
    main()
