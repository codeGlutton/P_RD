# -*- coding: utf-8 -*-
"""Reward BS 파츠용 초록 기준틀/마젠타 키 템플릿 생성.

각 템플릿은 기능 파츠 하나만 담는다. 가능한 파츠는 검수된 C03 원자 파츠를
같은 비율로 넣어 ImageGen이 재해석하지 않고 리페인트하도록 한다.
"""
import os

from PIL import Image, ImageDraw, ImageFont


PROJECT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
C03 = os.path.join(PROJECT, "Saved", "DesignAssets", "RewardC03Refine", "GeneratedParts")
C03_FALLBACK = os.path.join(PROJECT, "Saved", "DesignAssets", "RewardC03Parts")
CONCEPT = os.path.join(PROJECT, "Saved", "DesignAssets", "RewardConceptArt_10", "concept03.png")
BURST = os.path.join(PROJECT, "Saved", "DesignAssets", "RewardWireframeV8_20260816",
                     "GeneratedPartsV2", "reward_v8_chest_burst_512x512_v2.png")
ROOT = os.path.join(PROJECT, "Saved", "DesignAssets", "RewardBS")
OUT = os.path.join(ROOT, "Templates")
RAW = os.path.join(ROOT, "RawGenerations")
GENERATED = os.path.join(ROOT, "GeneratedParts")
COMPARISONS = os.path.join(ROOT, "Comparisons")
for path in (OUT, RAW, GENERATED, COMPARISONS):
    os.makedirs(path, exist_ok=True)


# part -> (W, H, source kind, material swatch)
MANIFEST = {
    "battle_backdrop": (1536, 170, "battle_crop", False),
    "sheet_background": (1472, 694, "board_interior", True),
    "sheet_frame": (1472, 694, "assembled_frame", False),
    "title_plate": (360, 84, "title_plate", False),
    "stage_tab": (160, 58, "stage_tab", False),
    "step_track": (760, 22, "step_bar_track", False),
    "step_fill": (744, 12, "step_bar_fill", True),
    "step_coin_active": (76, 76, "step_coin_active", False),
    "step_coin_inactive": (64, 64, "step_coin_inactive", False),
    "cta_button": (360, 88, "cta_plate", False),
    "parchment_window": (500, 282, "parch_window", False),
    "xp_track": (330, 40, "track_plate", False),
    "xp_fill": (314, 24, "track_fill", True),
    "card_blank": (280, 320, "card_blank", False),
    "selection_glow": (292, 332, "selection_glow", False),
    "chest_burst": (420, 360, "chest_burst", False),
}

SOURCE_FILES = {
    "board_interior": "c03_board_interior_180x320.png",
    "title_plate": "c03_title_plate_566x136.png",
    "stage_tab": "c03_stage_tab_196x48.png",
    "step_bar_track": "c03_step_bar_track_690x22.png",
    "step_bar_fill": "c03_step_bar_fill_400x16.png",
    "step_coin_active": "c03_step_coin_active_92x92.png",
    "step_coin_inactive": "c03_step_coin_inactive_64x64.png",
    "cta_plate": "c03_cta_plate_400x94.png",
    "parch_window": "c03_parch_window_446x286.png",
    "track_plate": "c03_track_plate_322x44.png",
    "track_fill": "c03_track_fill_300x28.png",
    "card_blank": "c03_card_blank_290x326.png",
    "selection_glow": "c03_selection_glow_302x338.png",
}

MAGENTA = (255, 0, 255, 255)
GREEN = (0, 255, 0, 255)
GRAY = (118, 118, 122, 255)
FRAME = 12
MARGIN = 92
MAX_LONG = 1400.0

try:
    FONT = ImageFont.truetype("C:/Windows/Fonts/consolab.ttf", 28)
except OSError:
    FONT = ImageFont.load_default()


def cover(source, size):
    target_w, target_h = size
    scale = max(target_w / source.width, target_h / source.height)
    resized = source.resize((round(source.width * scale), round(source.height * scale)), Image.LANCZOS)
    left = (resized.width - target_w) // 2
    top = (resized.height - target_h) // 2
    return resized.crop((left, top, left + target_w, top + target_h))


def source_image(kind, size):
    tw, th = size
    if kind == "battle_crop":
        concept = Image.open(CONCEPT).convert("RGBA")
        # 좌하단 패널의 UI가 없는 좌측 전장 영역만 사용한다.
        crop = concept.crop((0, concept.height // 2, 240, concept.height // 2 + 170))
        return cover(crop, size)
    if kind == "assembled_frame":
        canvas = Image.new("RGBA", size, MAGENTA)
        rail_h = Image.open(os.path.join(C03, "c03_rail_h_110x42.png")).convert("RGBA")
        rail_l = Image.open(os.path.join(C03, "c03_rail_v_left_44x260.png")).convert("RGBA")
        rail_r = Image.open(os.path.join(C03, "c03_rail_v_right_44x260.png")).convert("RGBA")
        corners = {
            "tl": Image.open(os.path.join(C03, "c03_corner_tl_92x92.png")).convert("RGBA"),
            "tr": Image.open(os.path.join(C03, "c03_corner_tr_92x92.png")).convert("RGBA"),
            "bl": Image.open(os.path.join(C03, "c03_corner_bl_92x92.png")).convert("RGBA"),
            "br": Image.open(os.path.join(C03, "c03_corner_br_92x92.png")).convert("RGBA"),
        }
        rail = 52
        corner = 104
        canvas.alpha_composite(rail_h.resize((tw, rail), Image.LANCZOS), (0, 0))
        canvas.alpha_composite(rail_h.resize((tw, rail), Image.LANCZOS), (0, th - rail))
        canvas.alpha_composite(rail_l.resize((rail, th), Image.LANCZOS), (0, 0))
        canvas.alpha_composite(rail_r.resize((rail, th), Image.LANCZOS), (tw - rail, 0))
        canvas.alpha_composite(corners["tl"].resize((corner, corner), Image.LANCZOS), (0, 0))
        canvas.alpha_composite(corners["tr"].resize((corner, corner), Image.LANCZOS), (tw - corner, 0))
        canvas.alpha_composite(corners["bl"].resize((corner, corner), Image.LANCZOS), (0, th - corner))
        canvas.alpha_composite(corners["br"].resize((corner, corner), Image.LANCZOS), (tw - corner, th - corner))
        return canvas
    if kind == "board_interior":
        source = Image.open(os.path.join(C03, SOURCE_FILES[kind])).convert("RGBA")
        # C03 결과의 투명 키 보정점/외곽 장식은 제외하고 중앙 재질만 취한다.
        inset_x = max(2, source.width // 8)
        inset_y = max(2, source.height // 8)
        source = source.crop((inset_x, inset_y, source.width - inset_x, source.height - inset_y))
        return cover(source, size)
    if kind == "chest_burst":
        return cover(Image.open(BURST).convert("RGBA"), size)
    source_path = os.path.join(C03, SOURCE_FILES[kind])
    if not os.path.isfile(source_path):
        source_path = os.path.join(C03_FALLBACK, SOURCE_FILES[kind])
    source = Image.open(source_path).convert("RGBA")
    return source.resize(size, Image.LANCZOS)


for part, (tw, th, kind, swatch) in MANIFEST.items():
    scale = MAX_LONG / max(tw, th)
    bw, bh = max(80, round(tw * scale)), max(80, round(th * scale))
    canvas = Image.new("RGBA", (bw + 2 * (FRAME + MARGIN), bh + 2 * (FRAME + MARGIN) + 68), GRAY)
    draw = ImageDraw.Draw(canvas)
    x0, y0 = MARGIN + FRAME, MARGIN + FRAME
    draw.rectangle((x0 - FRAME, y0 - FRAME, x0 + bw + FRAME - 1, y0 + bh + FRAME - 1), fill=GREEN)
    draw.rectangle((x0, y0, x0 + bw - 1, y0 + bh - 1), fill=MAGENTA)
    ref = source_image(kind, (tw, th)).resize((bw, bh), Image.LANCZOS)
    canvas.alpha_composite(ref, (x0, y0))
    if swatch or part == "battle_backdrop":
        # C03 후처리의 알파 실측 게이트용 키 보정점. 최종 크기에서 1px 미만이다.
        marker = max(8, round(min(bw, bh) * 0.035))
        draw.rectangle((x0, y0, x0 + marker, y0 + marker), fill=MAGENTA)
        draw.rectangle((x0 + bw - marker - 1, y0 + bh - marker - 1,
                        x0 + bw - 1, y0 + bh - 1), fill=MAGENTA)
    tag = "MATERIAL SWATCH - NO BORDERS, NO EDGES" if swatch else "ONE FUNCTIONAL PART - REPAINT ONLY"
    draw.text((MARGIN, y0 + bh + FRAME + 12), f"{part}  {tw}x{th}  {tag}", fill=(32, 32, 36), font=FONT)
    canvas.convert("RGB").save(os.path.join(OUT, f"guide_{part}.png"))
    print(f"guide_{part}.png  box={bw}x{bh}")

print(f"{len(MANIFEST)} BS templates -> {OUT}")
