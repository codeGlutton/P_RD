# -*- coding: utf-8 -*-
"""C03 파츠 정제용 레퍼런스 내장 템플릿 생성.

초록 기준틀 안에 '현재 조립에 쓰이는 실제 파츠'를 마젠타 바탕 위에 넣는다.
Codex ImageGen은 이 템플릿을 편집 입력으로 받아 같은 디자인을 고해상으로
리페인트만 한다 — 자유 생성이 아니므로 없는 테두리를 발명할 수 없다.

사용법: python Scripts/Editor/make_c03_refine_templates.py
입력:   Saved/DesignAssets/RewardC03Parts/c03_<파츠>.png
출력:   Saved/DesignAssets/RewardC03Refine/Templates/refine_<파츠>.png
"""
import os

from PIL import Image, ImageDraw, ImageFont

PROJECT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
SRC = os.path.join(PROJECT, "Saved", "DesignAssets", "RewardC03Parts")
OUT = os.path.join(PROJECT, "Saved", "DesignAssets", "RewardC03Refine", "Templates")
os.makedirs(OUT, exist_ok=True)

# 파츠명 -> (W, H, 스와치 여부). 스와치 = 테두리 개념이 없는 재질 조각.
MANIFEST = {
    "board_interior": (180, 320, True),
    "rail_h": (110, 42, False),
    "rail_v_left": (44, 260, False),
    "rail_v_right": (44, 260, False),
    "corner_tl": (92, 92, False),
    "corner_tr": (92, 92, False),
    "corner_bl": (92, 92, False),
    "corner_br": (92, 92, False),
    "title_plate": (566, 136, False),
    "stage_tab": (196, 48, False),
    "cta_plate": (400, 94, False),
    "parch_window": (446, 286, False),
    "card_blank": (290, 326, False),
    "track_plate": (322, 44, False),
    "track_fill": (300, 28, True),
    "step_bar_track": (690, 22, False),
    "step_bar_fill": (400, 16, True),
    "step_coin_active": (92, 92, False),
    "step_coin_inactive": (64, 64, False),
}

MAGENTA = (255, 0, 255)
GREEN = (0, 255, 0)
GRAY = (118, 118, 122)
FRAME = 10
MARGIN = 110
BOX_LONG = 1200.0

try:
    FONT = ImageFont.truetype("C:/Windows/Fonts/consolab.ttf", 30)
except OSError:
    FONT = ImageFont.load_default()

for part, (tw, th, swatch) in MANIFEST.items():
    src_path = os.path.join(SRC, f"c03_{part}_{tw}x{th}.png")
    ref = Image.open(src_path).convert("RGBA")
    scale = BOX_LONG / max(tw, th)
    bw, bh = max(64, round(tw * scale)), max(64, round(th * scale))
    canvas_w = bw + 2 * (FRAME + MARGIN)
    canvas_h = bh + 2 * (FRAME + MARGIN) + 70

    im = Image.new("RGB", (canvas_w, canvas_h), GRAY)
    d = ImageDraw.Draw(im)
    x0, y0 = MARGIN + FRAME, MARGIN + FRAME
    d.rectangle((x0 - FRAME, y0 - FRAME, x0 + bw + FRAME - 1, y0 + bh + FRAME - 1), fill=GREEN)
    d.rectangle((x0, y0, x0 + bw - 1, y0 + bh - 1), fill=MAGENTA)
    # 레퍼런스를 마젠타 위에 알파 합성 (투명 영역은 마젠타가 남는다)
    ref_up = ref.resize((bw, bh), Image.LANCZOS)
    im.paste(ref_up, (x0, y0), ref_up)
    tag = "MATERIAL SWATCH - NO BORDERS, NO EDGES" if swatch else "REPAINT EXACTLY - DO NOT ADD DECORATION"
    d.text((MARGIN, y0 + bh + FRAME + 14), f"{part}  {tw}x{th}  {tag}",
           fill=(40, 40, 44), font=FONT)
    im.save(os.path.join(OUT, f"refine_{part}.png"))
    print(f"refine_{part}.png  box {bw}x{bh}")

print(f"\n{len(MANIFEST)} templates -> {OUT}")
