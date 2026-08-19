# -*- coding: utf-8 -*-
"""V12 가이드 템플릿 생성: 파츠별 정비율 마젠타 박스 + 초록 기준틀.

ImageGen이 어떤 해상도로 출력하든, 후처리가 초록 틀을 찾아 그 안쪽만 잘라내므로
파츠 종횡비가 기하학적으로 보장된다.

사용법: python Scripts/Editor/make_v12_guide_templates.py
출력:   Saved/DesignAssets/RewardAtomicV12/GuideTemplates/guide_<파츠명>.png
"""
import os

from PIL import Image, ImageDraw, ImageFont

PROJECT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
OUT = os.path.join(PROJECT, "Saved", "DesignAssets", "RewardAtomicV12", "GuideTemplates")
os.makedirs(OUT, exist_ok=True)

# postprocess_reward_v12_parts.py의 MANIFEST와 동일해야 한다.
MANIFEST = {
    "exp_row_plate": (1054, 132),
    "exp_portrait_ring": (116, 116),
    "exp_level_window": (96, 58),
    "exp_progress_track": (516, 22),
    "exp_xp_badge": (120, 64),
    "gold_panel_plate": (800, 228),
    "gold_coin_ring": (168, 168),
    "gold_amount_window": (300, 160),
    "card_frame": (240, 230),
    "card_name_plate": (240, 70),
    "card_selected_overlay": (240, 300),
    "modal_outer_frame": (1536, 864),
    "modal_background": (1536, 864),
    "header_plate": (720, 141),
    "step_plate": (380, 62),
    "cta_plate": (404, 70),
    "parchment_sheet": (1352, 498),
}

MAGENTA = (255, 0, 255)
GREEN = (0, 255, 0)
GRAY = (118, 118, 122)
FRAME = 10          # 초록 틀 두께
MARGIN = 110        # 틀 밖 회색 여백
BOX_LONG = 1200.0   # 마젠타 박스 긴 변 목표 픽셀

try:
    FONT = ImageFont.truetype("C:/Windows/Fonts/consolab.ttf", 34)
except OSError:
    FONT = ImageFont.load_default()

for part, (tw, th) in MANIFEST.items():
    scale = BOX_LONG / max(tw, th)
    bw, bh = max(64, round(tw * scale)), max(64, round(th * scale))
    cw, ch = bw + 2 * (FRAME + MARGIN), bh + 2 * (FRAME + MARGIN) + 60

    im = Image.new("RGB", (cw, ch), GRAY)
    d = ImageDraw.Draw(im)
    x0, y0 = MARGIN + FRAME, MARGIN + FRAME
    # 초록 기준틀 (박스 바깥쪽)
    d.rectangle((x0 - FRAME, y0 - FRAME, x0 + bw + FRAME - 1, y0 + bh + FRAME - 1), fill=GREEN)
    # 마젠타 작업 영역
    d.rectangle((x0, y0, x0 + bw - 1, y0 + bh - 1), fill=MAGENTA)
    d.text((MARGIN, y0 + bh + FRAME + 14),
           f"{part}  target {tw}x{th}  — paint ONLY inside the green frame",
           fill=(40, 40, 44), font=FONT)
    im.save(os.path.join(OUT, f"guide_{part}.png"))
    print(f"guide_{part}.png  canvas {cw}x{ch}  box {bw}x{bh}")

print(f"\n{len(MANIFEST)} templates -> {OUT}")
