from pathlib import Path

from PIL import Image, ImageDraw, ImageEnhance


ROOT = Path(__file__).resolve().parents[2]
SOURCE_DIR = ROOT / "Content" / "SourceArt" / "Dice"
OUTPUT_DIR = SOURCE_DIR / "Generated"

PAPER_TEXTURE = SOURCE_DIR / "Paper001_1K-JPG" / "Paper001_1K-JPG_Color.jpg"


def build_base_face() -> Image.Image:
    base = Image.open(PAPER_TEXTURE).convert("RGB")
    width, height = base.size
    crop_size = min(width, height)
    left = (width - crop_size) // 2
    top = (height - crop_size) // 2
    base = base.crop((left, top, left + crop_size, top + crop_size))
    base = base.resize((512, 512), Image.Resampling.LANCZOS)

    base = ImageEnhance.Color(base).enhance(0.22)
    base = ImageEnhance.Brightness(base).enhance(1.18)
    base = ImageEnhance.Contrast(base).enhance(0.80)

    overlay = Image.new("RGBA", base.size, (232, 244, 238, 132))
    base = Image.alpha_composite(base.convert("RGBA"), overlay)

    draw = ImageDraw.Draw(base)
    draw.rounded_rectangle(
        (18, 18, 494, 494),
        radius=44,
        outline=(116, 164, 154, 255),
        width=14,
    )

    return base


def main() -> None:
    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
    build_base_face().save(OUTPUT_DIR / "T_DiceFace_Base.png")


if __name__ == "__main__":
    main()
