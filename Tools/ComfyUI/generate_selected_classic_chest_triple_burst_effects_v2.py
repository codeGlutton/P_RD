#!/usr/bin/env python3
"""Regenerate the selected triple-burst chest with safe framing and localized effects."""

from pathlib import Path

from PIL import Image

import generate_selected_classic_chest_opening_i2v as base


base.PROJECT_OUTPUT = Path(
    r"C:\Users\2009e\.codex\worktrees\a2e1\P_RD_develop"
    r"\Saved\UI\RewardConcept03New\SelectedClassicChest_TripleBurstEffectsV3_20260818"
)
base.INPUT_NAME = "codex_selected_classic_chest_safe_frame_v3.png"
base.LENGTH = 65

base.NEGATIVE_PROMPT = (
    "cropped chest, cut off lid, cut off coins, object touching frame edge, zoom in, camera push in, "
    "camera pan, camera orbit, camera drift, moving viewpoint, rectangular golden background, yellow box, "
    "yellow panel, bright rectangle, full-screen gold wash, full-frame glow, flat color card, empty chest, "
    "one or two coins, weak motion, slow lid, gentle opening, morphing chest, duplicated chest, extra chest, "
    "deformed chest, extra metal bands, changing design, people, hands, creature, text, caption, logo, "
    "watermark, fire, smoke cloud, permanent overexposure, low quality, temporal flicker"
)

base.VARIANTS = {
    "A": {
        "slug": "triple_burst_effects_safe_frame_v3",
        "seed": 826182404,
        "prompt": (
            "A four-second stylized low-poly fantasy game jackpot animation using this exact wooden and "
            "antique-brass treasure chest. Preserve its exact design, proportions, materials and three-quarter "
            "front view. Keep the complete chest, the fully raised lid, every major coin burst and all effects "
            "inside the central eighty percent safe area for the entire clip, with generous empty margin above, "
            "below and on both sides. Never crop the lid or base. Static centered camera, absolutely no zoom. "
            "The background remains uniform near-black matching the input edges. Never create a rectangular "
            "gold or yellow panel. All light is localized in soft circular glows directly behind the chest. "
            "For the first half-second the chest compresses and rattles with rapidly building pressure. Then the "
            "large lock snaps and the heavy lid SLAMS fully open in a few frames with a violent upward recoil, "
            "one strong impact shake and a quick scale overshoot. The interior is visibly packed to the brim with "
            "a mound of gold coins. At the opening impact a circular gold shockwave ring expands behind the chest, "
            "short radial speed lines flash outward, and a dense first blast of coins punches toward the viewer. "
            "Two fast secondary coin blasts follow like a three-hit arcade combo. Each hit has its own smaller "
            "circular shockwave, star-shaped sparks and crisp recoil. Several hero coins rush toward the camera, "
            "grow dramatically larger with readable perspective and motion blur, then arc down. Dozens of smaller "
            "coins spray high and sideways but stay within the safe frame, then rain around the chest. Finish with "
            "the complete open chest fully visible, still overflowing with a large gold pile and coins scattered "
            "around its base. Exaggerated punchy premium game reward feedback, strong anticipation and release, "
            "no text."
        ),
    },
}


def prepare_safe_input() -> Path:
    if not base.SOURCE_CHEST.is_file():
        raise FileNotFoundError(base.SOURCE_CHEST)
    source = Image.open(base.SOURCE_CHEST).convert("RGB")
    source = source.resize((500, 333), Image.Resampling.LANCZOS)
    target = Image.new("RGB", (base.WIDTH, base.HEIGHT), (3, 3, 4))
    target.paste(source, ((base.WIDTH - source.width) // 2, 88))

    input_dir = base.COMFY_ROOT / "input"
    input_dir.mkdir(parents=True, exist_ok=True)
    input_path = input_dir / base.INPUT_NAME
    target.save(input_path, quality=96)
    base.PROJECT_OUTPUT.mkdir(parents=True, exist_ok=True)
    target.save(base.PROJECT_OUTPUT / "00_SelectedChest_SafeFrameV3_Input.png",
                quality=96)
    return input_path


base.prepare_input = prepare_safe_input


if __name__ == "__main__":
    raise SystemExit(base.main())
