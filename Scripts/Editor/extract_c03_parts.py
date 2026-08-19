# -*- coding: utf-8 -*-
"""concept03 목업에서 c03 파츠를 크롭·힐링해 1차 아트 세트를 만든다.

소스: 스크래치의 c03_panel{0..3}.png (1536x864로 확대된 패널 4장)
출력: Saved/DesignAssets/RewardC03Parts/  (설계 좌표 규격, 필요한 곳 실제 알파)

베이크된 텍스트·숫자·아이콘은 주변 텍스처 샘플로 지우고 런타임에서 올린다.
선명도가 부족한 파츠는 이후 Codex 재생성(가이드 템플릿 파이프라인)으로 교체한다.
"""
import os

import numpy as np
from PIL import Image, ImageDraw, ImageFilter

PROJECT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
SCRATCH = r"C:\Users\2009e\AppData\Local\Temp\claude\D--UnrealProjects-P-RD-develop\ac6170e6-fd56-41e6-9ad2-eadee642a2f1\scratchpad"
OUT = os.path.join(PROJECT, "Saved", "DesignAssets", "RewardC03Parts")
os.makedirs(OUT, exist_ok=True)

P = [Image.open(os.path.join(SCRATCH, f"c03_panel{i}.png")).convert("RGB") for i in range(4)]

def save(im, name):
    im.save(os.path.join(OUT, name))
    print(f"{name:36s} {im.size[0]}x{im.size[1]}")

def rounded_alpha(im, radius, soft=1.2):
    mask = Image.new("L", im.size, 0)
    ImageDraw.Draw(mask).rounded_rectangle((0, 0, im.size[0] - 1, im.size[1] - 1),
                                           radius=radius, fill=255)
    out = im.convert("RGBA")
    out.putalpha(mask.filter(ImageFilter.GaussianBlur(soft)))
    return out

def circle_alpha(im, soft=1.5):
    mask = Image.new("L", im.size, 0)
    ImageDraw.Draw(mask).ellipse((0, 0, im.size[0] - 1, im.size[1] - 1), fill=255)
    out = im.convert("RGBA")
    out.putalpha(mask.filter(ImageFilter.GaussianBlur(soft)))
    return out

def patch(im, box, sample_box, blur=2.0):
    """box 영역을 sample_box 텍스처의 리사이즈본으로 덮는다 (경계 소프트)."""
    region = im.crop(sample_box).resize((box[2] - box[0], box[3] - box[1]), Image.LANCZOS)
    region = region.filter(ImageFilter.GaussianBlur(blur))
    mask = Image.new("L", region.size, 0)
    ImageDraw.Draw(mask).rounded_rectangle((0, 0, region.size[0] - 1, region.size[1] - 1),
                                           radius=8, fill=255)
    mask = mask.filter(ImageFilter.GaussianBlur(3))
    im.paste(region, (box[0], box[1]), mask)

# ---------- 1) 모달 보드 = 깨끗한 구간만 잘라 조립 (힐링 없음) ----------
# 인테리어 차콜 타일, 수평/수직 레일, 코너 브레이스 4종. 빌더가 9-slice식으로 조립.
save(P[1].crop((160, 340, 340, 660)).convert("RGBA"), "c03_board_interior_180x320.png")
save(P[1].crop((330, 260, 440, 302)).convert("RGBA"), "c03_rail_h_110x42.png")
save(P[1].crop((100, 340, 144, 600)).convert("RGBA"), "c03_rail_v_left_44x260.png")
save(P[1].crop((1380, 340, 1424, 600)).convert("RGBA"), "c03_rail_v_right_44x260.png")
corner_tr = P[1].crop((1332, 258, 1424, 350))
save(rounded_alpha(corner_tr, 8), "c03_corner_tr_92x92.png")
save(rounded_alpha(corner_tr.transpose(Image.FLIP_LEFT_RIGHT), 8), "c03_corner_tl_92x92.png")
save(rounded_alpha(P[1].crop((98, 648, 190, 740)), 8), "c03_corner_bl_92x92.png")
save(rounded_alpha(P[1].crop((1332, 648, 1424, 740)), 8), "c03_corner_br_92x92.png")

# ---------- 2) 타이틀 판 (텍스트는 유지? -> 지운다: 런타임 텍스트) ----------
title = P[1].crop((482, 58, 1048, 194))
patch(title, (110, 30, 456, 108), (60, 26, 110, 110), blur=3)  # "전투 보상" 제거: 좌측 판재 샘플
save(rounded_alpha(title, 22), "c03_title_plate_566x136.png")

# ---------- 3) 스테이지 탭 (텍스트 힐링: 좌우 캡 + 중앙 스트레치) ----------
tab = P[1].crop((118, 272, 314, 320))
left_cap = tab.crop((0, 0, 30, 48))
right_cap = tab.crop((166, 0, 196, 48))
mid = tab.crop((30, 0, 44, 48)).resize((136, 48), Image.LANCZOS)
clean_tab = Image.new("RGB", (196, 48))
clean_tab.paste(left_cap, (0, 0)); clean_tab.paste(mid, (30, 0)); clean_tab.paste(right_cap, (166, 0))
save(rounded_alpha(clean_tab, 12), "c03_stage_tab_196x48.png")

# ---------- 4) 단계 코인 (번호 힐링: 숫자 없는 세로 슬라이스를 얼굴 전체로) ----------
# 활성: 패널0의 1번 코인 (cx~583, cy~268, 글로우 포함)
active = P[0].crop((537, 222, 629, 314))
face = P[0].crop((570, 246, 596, 254)).resize((44, 44), Image.LANCZOS).filter(ImageFilter.GaussianBlur(5))
m = Image.new("L", (44, 44), 0); ImageDraw.Draw(m).ellipse((0, 0, 43, 43), fill=255)
active.paste(face, (24, 24), m.filter(ImageFilter.GaussianBlur(2)))
save(circle_alpha(active), "c03_step_coin_active_92x92.png")
# 비활성: 패널0의 3번 코인 (cx~941, cy~268)
inactive = P[0].crop((909, 236, 973, 300))
face_i = P[0].crop((916, 252, 926, 284)).resize((34, 34), Image.LANCZOS).filter(ImageFilter.GaussianBlur(5))
m2 = Image.new("L", (34, 34), 0); ImageDraw.Draw(m2).ellipse((0, 0, 33, 33), fill=255)
inactive.paste(face_i, (15, 15), m2.filter(ImageFilter.GaussianBlur(2)))
save(circle_alpha(inactive), "c03_step_coin_inactive_64x64.png")

# ---------- 5) 단계 바 (트랙 + 청록 필, 타일용 — 코인 글로우가 안 닿는 구간만 샘플) ----------
track = P[0].crop((980, 262, 1040, 278)).resize((690, 22), Image.LANCZOS)
save(rounded_alpha(track, 10), "c03_step_bar_track_690x22.png")
fill = P[0].crop((640, 263, 700, 277)).resize((400, 16), Image.LANCZOS)
save(rounded_alpha(fill, 8), "c03_step_bar_fill_400x16.png")

# ---------- 6) CTA 판 (텍스트 힐링) ----------
cta = P[1].crop((568, 700, 968, 794))
lc = cta.crop((0, 0, 56, 94)); rc = cta.crop((344, 0, 400, 94))
mc = cta.crop((60, 0, 82, 94)).resize((288, 94), Image.LANCZOS)
clean_cta = Image.new("RGB", (400, 94))
clean_cta.paste(lc, (0, 0)); clean_cta.paste(mc, (56, 0)); clean_cta.paste(rc, (344, 0))
save(rounded_alpha(clean_cta, 16), "c03_cta_plate_400x94.png")

# ---------- 7) 양피지 창 (문구 힐링, 9-slice 공용) ----------
window = P[1].crop((824, 376, 1270, 662))
patch(window, (60, 100, 386, 190), (60, 40, 386, 90), blur=3)  # 문구 제거
save(rounded_alpha(window, 14), "c03_parch_window_446x286.png")

# ---------- 8) XP 육각 배지 (육각 폴리곤 마스크) ----------
badge = P[0].crop((952, 384, 1108, 540))
w, h = badge.size
hexpts = [(w * .5, 0), (w * .96, h * .25), (w * .96, h * .75), (w * .5, h - 1),
          (w * .04, h * .75), (w * .04, h * .25)]
hm = Image.new("L", badge.size, 0)
ImageDraw.Draw(hm).polygon(hexpts, fill=255)
badge_a = badge.convert("RGBA"); badge_a.putalpha(hm.filter(ImageFilter.GaussianBlur(1.5)))
save(badge_a, "c03_xp_badge_156x156.png")

# ---------- 9) 경험치 트랙 (빈 트랙 + 필) ----------
tr = P[0].crop((386, 398, 708, 442))
empty_right = tr.crop((200, 0, 322, 44))
trk = Image.new("RGB", (322, 44))
trk.paste(empty_right.resize((322, 44), Image.LANCZOS), (0, 0))
lcap = empty_right.crop((100, 0, 122, 44)).transpose(Image.FLIP_LEFT_RIGHT)
trk.paste(lcap, (0, 0))
save(rounded_alpha(trk, 14), "c03_track_plate_322x44.png")
tfill = P[0].crop((398, 406, 516, 434)).resize((300, 28), Image.LANCZOS)
save(rounded_alpha(tfill, 12), "c03_track_fill_300x28.png")

# ---------- 10) 아티팩트 카드 (내부 전체를 좌측 깨끗한 양피지 띠로 재구성) ----------
card = P[3].crop((262, 346, 552, 672))
clean_strip = card.crop((32, 108, 74, 292))  # 성배 왼쪽의 깨끗한 양피지 (글로우 배제)
interior = clean_strip.resize((250, 292), Image.LANCZOS).filter(ImageFilter.GaussianBlur(2))
im_mask = Image.new("L", interior.size, 0)
ImageDraw.Draw(im_mask).rounded_rectangle((0, 0, 249, 291), radius=6, fill=255)
card.paste(interior, (20, 18), im_mask.filter(ImageFilter.GaussianBlur(1)))
save(rounded_alpha(card, 16), "c03_card_blank_290x326.png")

# ---------- 11) 선택 발광 (절차 생성 - 시안 청록) ----------
glow = Image.new("RGBA", (302, 338), (0, 0, 0, 0))
gd = ImageDraw.Draw(glow)
for i, (alpha, wd) in enumerate([(60, 14), (110, 10), (200, 6), (255, 3)]):
    gd.rounded_rectangle((14 - wd // 2 + i * 2, 14 - wd // 2 + i * 2,
                          288 + wd // 2 - i * 2, 324 + wd // 2 - i * 2),
                         radius=18, outline=(80, 220, 255, alpha), width=wd)
glow = glow.filter(ImageFilter.GaussianBlur(3))
save(glow, "c03_selection_glow_302x338.png")

# ---------- 12) 상자·골드 비주얼 (차콜 위 불투명 크롭 - 보드와 동톤) ----------
save(P[1].crop((368, 340, 712, 664)).convert("RGBA"), "c03_chest_visual_344x324.png")
save(P[2].crop((400, 380, 700, 680)).convert("RGBA"), "c03_gold_coin_visual_300x300.png")

print("\ndone ->", OUT)
