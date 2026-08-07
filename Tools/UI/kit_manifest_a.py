"""Concept A UI kit — 잘라낸 부품의 이름 · 그리는 방식 · 9-slice 여백.

번호는 사람이 대조표(`Saved/UIKit/ConceptA/_contact.png`)를 보고 붙였다.
자동으로 정하지 않는다 -- 무엇이 단추이고 무엇이 칩인지 기계가 틀리면 그 이름이
배선까지 따라간다.

여백은 `measure_nine_slice.py` 가 잰 값이다. 세로가 짧은 부품은 상하 여백이
높이의 30% 한계에 걸려 있는데, 원래 테두리가 그만큼 두꺼워서 맞는 값이다.

draw:
    box    9-slice. 늘려도 모서리 장식이 안 뭉개진다
    image  통짜. 늘 같은 크기로 그리는 것들(체크박스 · 손잡이 · 칩 테두리)
"""

# (부품번호, 텍스처 이름, 그리는 방식, 여백(좌,상,우,하), 쓰이는 곳)
# 여백은 그림 안쪽 기준이고, 잘라낼 때 남긴 투명 여백은 PAD 에 따로 있다.
PARTS = [
    (0,  "T_KitA_Button_Wide_Normal",    "box",   (57, 65, 57, 65), "아래 단추 줄 · 큰 확인 단추"),
    (4,  "T_KitA_Button_Wide_Pressed",   "box",   (76, 54, 76, 54), "위와 같은 자리의 눌림"),
    (8,  "T_KitA_Button_Wide_Disabled",  "box",   (46, 65, 46, 65), "위와 같은 자리의 못 누름"),
    (1,  "T_KitA_Button_Small_Normal",   "box",   (77, 26, 77, 26), "뒤로 · 되돌리기 같은 작은 단추"),
    (2,  "T_KitA_Checkbox_Off",          "image", (36, 37, 36, 37), "설정 체크 끔"),
    (3,  "T_KitA_Checkbox_On",           "image", (23, 38, 23, 38), "설정 체크 켬"),
    (5,  "T_KitA_Slider_Track",          "box",   (28, 23, 28, 23), "소리 슬라이더 홈"),
    (7,  "T_KitA_Slider_Fill",           "box",   (15, 17, 15, 17), "슬라이더 채운 부분"),
    (6,  "T_KitA_Slider_Knob",           "image", (28, 31, 28, 31), "슬라이더 손잡이"),
    (9,  "T_KitA_StatChip_Ring",         "image", (9,  20, 9,  20), "수치 칩 테두리(AP · 피해 …)"),
    (10, "T_KitA_Row_Plate",             "box",   (62, 56, 62, 56), "설정 한 줄 받침 · 목록 행"),
    (11, "T_KitA_Cell_Normal",           "image", (15, 17, 15, 17), "세그먼트 칸 · 스킬 칸"),
    (12, "T_KitA_Cell_Selected",         "image", (25, 25, 25, 25), "고른 칸"),
    (13, "T_KitA_Cell_Disabled",         "image", (14, 15, 14, 15), "못 고르는 칸"),
    (14, "T_KitA_Portrait_Frame",        "box",   (65, 68, 65, 68), "초상화 · 아이콘 틀"),
    (15, "T_KitA_HPBar_Fill_Red",        "box",   (17, 21, 17, 21), "적 HP"),
    (17, "T_KitA_HPBar_Fill_Green",      "box",   (20, 18, 20, 18), "아군 HP"),
    # 명패는 양끝이 파여 있어 9-slice 가 안 된다. 어디를 잘라도 늘어나는 쪽이
    # 한결같지 않다고 실측이 말한다. 늘리지 말고 비율을 지켜 그린다.
    (16, "T_KitA_Title_Plate",           "image", (51, 50, 51, 50), "화면 제목 명패"),
]

# ── 전면 프레임. 통짜 1920x1080 시안을 split_frame_parts.py 로 나눈 것이다.
#
# 통짜로 쓰면 16:9 가 아닌 폰에서 나무가 늘어나고, 열 비율이 그림에 박혀 두 열·
# 네 열 화면에 못 쓴다. 바깥 틀과 기둥으로 나누면 "틀 한 장 + 기둥 N개를 원하는
# x 에" 가 되어 열 개수도 위치도 코드가 정한다.
#
# 이 둘은 부품 시트가 아니라 원본에서 바로 떠서 **투명 여백이 없다**(pad=0).
FRAME_PARTS = [
    ("T_KitA_Frame_Outer", "box", (91, 98, 91, 98), 0, "화면 바깥 틀. 아무 크기나"),
    ("T_KitA_Frame_Divider", "box", (0, 94, 0, 94), 0, "세로 기둥. 높이만 늘어난다"),
]

# ── 큰 판. KitA 시트 밖에서 빌려 온다 ────────────────────────────────
#
# KitA 의 ``T_KitA_Row_Plate`` 는 526x140 짜리 **한 줄** 그림이다. 속이 비어
# 있고(투명) 나무가 좌우 66px 이라, 열이나 패널처럼 큰 자리에 늘려 놓으면
# 뒤가 그대로 비치고 나무만 두껍게 남는다. 실제로 상세창 세 열이 그랬다.
#
# 대신 용병 화면의 양피지 줄판을 쓴다. 속이 차 있고(양피지색) 테두리가
# 1024 폭에 55px 이라 훨씬 얇다. 아래 표의 마지막 칸은 이 그림이 원래
# 의도한 테두리 두께(px)이고, 그보다 얇게 그리고 싶으면 kit_brush 가
# ImageSize 를 줄여 준다.
PANEL_PARTS = [
    ("T_MB_HireRowNormal", "box", (55, 25, 55, 25), 0,
     "/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Hire",
     "양피지 판. 열·패널 바탕. 속이 차 있다"),
    ("T_MB_HirePartyRowEmpty", "box", (92, 62, 92, 62), 0,
     "/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Hire",
     "위와 같은 결의 두꺼운 판. 큰 자리에"),
]

# 이름 -> 잘라낼 때 남긴 투명 여백. 없으면 8(부품 시트 기본).
PAD_BY_NAME = {name: pad for name, _draw, _margin, pad, _use in FRAME_PARTS}
PAD_BY_NAME.update({name: pad for name, _d, _m, pad, _dir, _use in PANEL_PARTS})

# 이름 -> 텍스처가 있는 폴더. 없으면 KitA 폴더(kit_brush.KIT_DIR).
DIR_BY_NAME = {name: folder for name, _d, _m, _p, folder, _use in PANEL_PARTS}


# ── 부품 안에 글자·아이콘을 놓아도 되는 자리 ──────────────────────────
#
# 목록 페이지(assets.html)에서 눈으로 맞춘 값이다. 예전에는 `chip * 0.20` 처럼
# 비율로 짐작했고, 재 보니 칩 글자가 링을 밟고 초상화가 사방 11% 나무 위로
# 올라가 있었다.
#
# 값은 **원본 텍스처 기준 비율**이다. 뒤의 두 수는 그 원본 크기다 -- 9-slice
# 부품은 테두리가 원본 픽셀 크기 그대로 그려지므로, 늘려 놓은 자리의 안쪽을
# 구하려면 비율이 아니라 픽셀로 환산해야 한다(kit_brush.inner_rect).
#
# apply_user_rects.py 가 다시 만든다. 손으로 고치지 말 것 -- 페이지에서 고치면
# 여기로 들어온다.
INNER = {
    "T_KitA_Button_Small_Normal": (0.1195, 0.2145, 0.8706, 0.7876, 281, 133),
    "T_KitA_Button_Wide_Disabled": (0.0627, 0.2259, 0.9388, 0.7726, 763, 163),
    "T_KitA_Button_Wide_Normal": (0.063, 0.2273, 0.9365, 0.7586, 764, 164),
    "T_KitA_Button_Wide_Pressed": (0.0618, 0.2485, 0.9357, 0.7678, 763, 165),
    "T_KitA_Cell_Disabled": (0.0873, 0.0967, 0.9142, 0.9028, 189, 176),
    "T_KitA_Cell_Normal": (0.0936, 0.0951, 0.9091, 0.9005, 186, 176),
    "T_KitA_Cell_Selected": (0.1862, 0.2045, 0.8138, 0.8011, 188, 176),
    "T_KitA_Checkbox_Off": (0.2206, 0.218, 0.7794, 0.782, 136, 133),
    "T_KitA_Checkbox_On": (0.2787, 0.4052, 0.7131, 0.5948, 122, 116),
    "T_KitA_Frame_Outer": (0.0365, 0.0741, 0.9635, 0.9352, 1920, 1080),
    "T_KitA_Portrait_Frame": (0.1979, 0.1944, 0.8021, 0.8056, 268, 252),
    "T_KitA_Row_Plate": (0.125, 0.2353, 0.875, 0.7647, 526, 140),
    "T_KitA_Slider_Knob": (0.3725, 0.3942, 0.6275, 0.6058, 102, 104),
    "T_KitA_Slider_Track": (0.0521, 0.2857, 0.9479, 0.7143, 596, 88),
    "T_KitA_StatChip_Ring": (0.2745, 0.2829, 0.7255, 0.7237, 153, 152),
    "T_KitA_Title_Plate": (0.125, 0.25, 0.875, 0.8542, 719, 183),
    "T_MB_HirePartyRowEmpty": (0.0481, 0.1103, 0.9534, 0.8621, 1024, 420),
    "T_MB_HireRowNormal": (0.0363, 0.0878, 0.9613, 0.8961, 1024, 288),
}


def inner_ratio(name):
    """자리 비율. 없으면 None -- 부르는 쪽이 기본값을 쓴다."""
    entry = INNER.get(name)
    return entry[:4] if entry is not None else None


def inner_source(name):
    """그 비율을 잰 원본 크기. 9-slice 를 픽셀로 환산할 때 쓴다."""
    entry = INNER.get(name)
    return entry[4:6] if entry is not None else None


# 부품 시트에 없어서 화면 시안(A-③~⑥)에서 받아야 하는 것들.
# 이게 없으면 배치를 옮겨도 바탕이 옛 그림 그대로다.
MISSING = [
    "양피지 큰 판 (열 하나를 채우는 바탕)",
    "어두운 우물 (판 안에 파인 자리)",
    "전체 화면 스크림 (상세창 뒤를 덮는 것)",
    "세로 스크롤 지도 몸통",
]
