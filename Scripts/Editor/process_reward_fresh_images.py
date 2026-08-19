from __future__ import annotations

import shutil
import sys
from pathlib import Path

from PIL import Image


PROJECT = Path(r"D:\UnrealProjects\P_RD_develop_20260816")
RAW_DIR = PROJECT / "Saved" / "DesignAssets" / "RewardFreshGenerated_20260817" / "Raw"
FINAL_DIR = PROJECT / "Saved" / "DesignAssets" / "RewardFreshGenerated_20260817" / "Generated"
ARCHIVE_DIR = Path(r"F:\코덱스이미지생성폴더")


def resize_shell(source: Path, output_name: str) -> Path:
    RAW_DIR.mkdir(parents=True, exist_ok=True)
    FINAL_DIR.mkdir(parents=True, exist_ok=True)
    ARCHIVE_DIR.mkdir(parents=True, exist_ok=True)

    raw_target = RAW_DIR / f"{Path(output_name).stem}_raw.png"
    shutil.copy2(source, raw_target)

    with Image.open(source) as image:
        image = image.convert("RGBA")
        image = image.resize((1536, 864), Image.Resampling.LANCZOS)
        output = FINAL_DIR / output_name
        image.save(output, optimize=True)

    archive = ARCHIVE_DIR / f"reward_fresh_20260817_{output_name}"
    shutil.copy2(output, archive)
    print(output)
    print(archive)
    return output


def chroma_key(image: Image.Image) -> Image.Image:
    rgba = image.convert("RGBA")
    pixels = []
    for red, green, blue, _ in rgba.getdata():
        # Generated chroma backgrounds contain painterly pink variation rather than
        # one exact RGB value. Magenta is the only region where both red and blue
        # dominate green; gold, cyan, parchment, iron and red gems each fail at
        # least one side of that test and therefore remain opaque.
        magenta_strength = min(red - green, blue - green)
        if min(red, blue) < 86 or magenta_strength <= 20:
            alpha = 255
        elif magenta_strength >= 86:
            alpha = 0
        else:
            alpha = round((86 - magenta_strength) * 255 / 66)
        pixels.append((red, green, blue, alpha))
    rgba.putdata(pixels)
    return rgba


def save_final(image: Image.Image, output_name: str) -> Path:
    FINAL_DIR.mkdir(parents=True, exist_ok=True)
    ARCHIVE_DIR.mkdir(parents=True, exist_ok=True)
    output = FINAL_DIR / output_name
    image.save(output, optimize=True)
    archive = ARCHIVE_DIR / f"reward_fresh_20260817_{output_name}"
    shutil.copy2(output, archive)
    print(output)
    print(archive)
    return output


def process_overlay(source: Path, output_name: str, width: int, height: int) -> Path:
    RAW_DIR.mkdir(parents=True, exist_ok=True)
    raw_target = RAW_DIR / f"{Path(output_name).stem}_raw.png"
    shutil.copy2(source, raw_target)
    keyed = chroma_key(Image.open(source))
    bounds = keyed.getbbox()
    if bounds:
        keyed = keyed.crop(bounds)
    keyed = keyed.resize((width, height), Image.Resampling.LANCZOS)
    return save_final(keyed, output_name)


def process_chest_sheet(source: Path, suffix: str) -> None:
    RAW_DIR.mkdir(parents=True, exist_ok=True)
    shutil.copy2(source, RAW_DIR / f"reward_fresh_chest_sheet_{suffix}_raw.png")
    sheet = chroma_key(Image.open(source))
    cell_width = sheet.width / 3.0
    for index, name in enumerate(("closed", "half_open", "open")):
        left = round(index * cell_width)
        right = round((index + 1) * cell_width)
        cell = sheet.crop((left, 0, right, sheet.height))
        bounds = cell.getbbox()
        if bounds:
            cell = cell.crop(bounds)
        target = Image.new("RGBA", (520, 420), (0, 0, 0, 0))
        scale = min(480 / cell.width, 390 / cell.height)
        resized = cell.resize(
            (max(1, round(cell.width * scale)), max(1, round(cell.height * scale))),
            Image.Resampling.LANCZOS,
        )
        target.alpha_composite(resized, ((520 - resized.width) // 2, 420 - resized.height))
        save_final(target, f"reward_fresh_chest_{name}_520x420_{suffix}.png")


def crop_asset(source: Path, output_name: str, box: tuple[int, int, int, int], size: tuple[int, int]) -> None:
    with Image.open(source) as image:
        cropped = image.convert("RGBA").crop(box)
        cropped = cropped.resize(size, Image.Resampling.LANCZOS)
        save_final(cropped, output_name)


if __name__ == "__main__":
    mode = sys.argv[1]
    if mode == "shell" and len(sys.argv) == 4:
        resize_shell(Path(sys.argv[2]), sys.argv[3])
    elif mode == "overlay" and len(sys.argv) == 6:
        process_overlay(Path(sys.argv[2]), sys.argv[3], int(sys.argv[4]), int(sys.argv[5]))
    elif mode == "chest" and len(sys.argv) == 4:
        process_chest_sheet(Path(sys.argv[2]), sys.argv[3])
    elif mode == "crop" and len(sys.argv) == 9:
        crop_asset(
            Path(sys.argv[2]), sys.argv[3],
            tuple(int(value) for value in sys.argv[4:8]),
            (int(sys.argv[8].split("x")[0]), int(sys.argv[8].split("x")[1])),
        )
    else:
        raise SystemExit(
            "usage: process_reward_fresh_images.py shell SOURCE OUTPUT | "
            "overlay SOURCE OUTPUT W H | chest SOURCE SUFFIX | "
            "crop SOURCE OUTPUT L T R B WxH"
        )
