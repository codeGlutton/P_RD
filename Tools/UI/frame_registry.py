"""Measured frame regions + per-screen frame assignment. Single source of truth.

칸 값은 catalog_frame_regions.py 가 그림에서 잰 비율을 1920x1080 으로 환산한 것이다.
빌더와 검증기가 같은 값을 봐야 "섹션이 분할선을 밟았다" 같은 문제가 다시 안 생긴다.
"""

UIROOT = "/Game/SVN/OutSideAsset/AICreation/UI"
W, H = 1920.0, 1080.0
PAD = 20.0

# kind:
#   cols   - 그림에 세로 분할선이 있어 열이 정해진 판
#   window - 가운데 창 하나만 뚫린 판/스크림
FRAMES = {
    "base3": dict(
        texture=f"{UIROOT}/Marchbound/MonsterTab/T_MT_BaseFrame", kind="cols",
        cols=[(38.0, 499.0), (518.0, 1075.0), (1094.0, 1882.0)], rows=(130.0, 1015.0),
        # 그림에 제목판이 이미 있다(가운데가 아니라 왼쪽 치우침). 판을 덧그리지 말고
        # 이 자리에 글자만 얹는다.
        titlePlate=(335.0, 67.0, 643.0, 125.0)),
    "titleBg": dict(
        texture=f"{UIROOT}/Title/T_title_bg", kind="window",
        window=(346.0, 518.0, 1248.0, 497.0), art=(0.0, 0.0, 1920.0, 520.0)),
    "setScrim": dict(
        texture=f"{UIROOT}/Settings/T_set_scrim", kind="window",
        # 옛 스크림 그림에서 잰 창이었다. 그 그림은 안 쓰고 한 줄 받침을 늘려
        # 판으로 삼는데, 그 받침은 위아래 나무가 33px 이라 680 안에 줄을 넣으면
        # 한 줄이 44px 로 눌린다(받침 원본은 140px 짜리다). 줄이 제 높이를
        # 갖도록 창을 키우고 가운데로 맞춘다.
        window=(300.0, 140.0, 1320.0, 800.0),
        inner=f"{UIROOT}/Settings/T_set_panel_frame", innerAspect=1.35),
    "summaryWide": dict(
        texture=f"{UIROOT}/Marchbound/Defeat/T_MB_Defeat_BattleSummary", kind="window",
        window=(173.0, 205.0, 1574.0, 670.0)),
    "detailPanel": dict(
        texture=f"{UIROOT}/Marchbound/Common/T_MB_GenericDetailPanel", kind="window",
        # 상단 6~21% 는 그림에 제목 띠가 박혀 있다. 콘텐츠는 그 아래부터.
        window=(0.10, 0.24, 0.80, 0.68), relative=True, titleBand=(0.06, 0.21)),
    "mapScrim": dict(
        texture=f"{UIROOT}/WorldMap/T_wm_panel_scrim", kind="window",
        window=(326.0, 151.0, 1268.0, 724.0),
        strip=f"{UIROOT}/Map/T_StageMap_Background_Parchment", stripAspect=0.563),
}

# 화면 -> 프레임. FRAME_ASSIGNMENT.md 의 배정표와 같다.
SCREEN_FRAME = {
    "Title": "titleBg",
    "Settings": "setScrim",
    "SkillDetail": "base3",
    "EnemyDetail": "base3",
    "ArtifactDetail": "summaryWide",
    "EnemySummary": "detailPanel",
    "MercenarySummary": "detailPanel",
    "MercenaryTab": "base3",
    "MonsterTab": "base3",
    "Map": "mapScrim",
}

# base3 를 네 화면이 나눠 쓴다. 배경이 같으면 어느 화면인지 헷갈리므로 색으로 가른다.
SCREEN_TINT = {
    "SkillDetail": (0.95, 0.62, 0.22),
    "EnemyDetail": (0.92, 0.32, 0.28),
    "MercenaryTab": (0.35, 0.78, 0.92),
    "MonsterTab": (0.62, 0.85, 0.40),
    "ArtifactDetail": (0.80, 0.55, 0.95),
    "Map": (0.90, 0.78, 0.45),
}

SCREEN_TITLES = {
    "Title": "타이틀", "Settings": "설정", "SkillDetail": "스킬 상세",
    "EnemyDetail": "적 상세", "ArtifactDetail": "아티팩트 상세",
    "EnemySummary": "적 요약", "MercenarySummary": "용병 요약",
    "MercenaryTab": "용병탭", "MonsterTab": "몬스터탭", "Map": "지도",
}

VARIANTS = [("v01", "안전안"), ("v02", "재배치"), ("v03", "야간"),
            ("v04", "미니멀"), ("v05", "실험")]


# ── 조립식 전면 틀 ──────────────────────────────────────────────────────
#
# 통짜 프레임 그림을 쓰면 열 경계가 그림에 박혀 있어, 열 비율을 바꾸거나 열을
# 늘리려면 그림을 새로 받아야 한다. 16:9 가 아닌 폰에서는 나무도 늘어난다.
#
# 그래서 바깥 틀 한 장과 기둥 한 장으로 나눠 두고(split_frame_parts.py), 열은
# **여기서 숫자로** 정한다. 그림은 안 건드리고 비율만 고치면 된다.
COMPOSED = dict(
    outer="T_KitA_Frame_Outer",
    divider="T_KitA_Frame_Divider",
    bar=45.0,            # 바깥 틀 나무 두께. 콘텐츠는 이 안쪽부터
    dividerWidth=58.0,   # 기둥 그림 폭. 기둥은 이 폭 그대로 놓는다
)

# 화면 -> 열 너비 비율. 합이 1 이면 된다. 열 개수는 여기서 정해진다.
# 열 비율. 열마다 **무엇이 들어가는지**가 정한다.
#
# 상세창은 셋을 이렇게 쓴다.
#     왼쪽   그림 · 이름 밑 한 줄 · 설명   -- 문장이라 폭이 필요하다
#     가운데 수치 칩 다섯                  -- 칩은 좁아도 된다. 세로로 쌓는다
#     오른쪽 사거리·영향 격자 · 차단 규칙  -- 격자 둘이 나란히 들어가야 한다
#
# 전에는 0.24/0.29/0.47 이었다. 왼쪽이 좁아 설명이 두세 글자씩 갈라졌고,
# 가운데는 칩을 2+2+1 로 놓아 마지막 하나가 외따로 남았다.
SCREEN_COLUMNS = {
    "SkillDetail": (0.32, 0.22, 0.46),
    "EnemyDetail": (0.32, 0.22, 0.46),
    "MonsterTab": (0.30, 0.26, 0.44),
    "MercenaryTab": (0.30, 0.26, 0.44),
    # 아티팩트는 칩이 없다. 가운데와 오른쪽을 이어 붙여 효과 목록에 준다 --
    # 런타임이 가운데 열 판과 기둥 하나를 접는다.
    "ArtifactDetail": (0.32, 0.22, 0.46),
}


def compose(weights, size=(W, H), pad=PAD):
    """열 비율을 실제 사각형과 기둥 자리로 바꾼다.

    @return (열 사각형 목록, 기둥 x 목록, 콘텐츠 세로 범위)

    기둥은 열과 열 **사이**에 놓이므로 열 하나당 기둥 하나가 아니라, 열 개수보다
    하나 적다. 열 너비는 기둥이 먹는 폭을 뺀 나머지를 비율대로 나눈 것이다.
    """
    width, height = size
    bar, divider = COMPOSED["bar"], COMPOSED["dividerWidth"]
    inner_x, inner_w = bar, width - bar * 2
    inner_y, inner_h = bar, height - bar * 2

    usable = inner_w - divider * (len(weights) - 1)
    total = sum(weights)

    columns, dividers, cursor = [], [], inner_x
    for index, weight in enumerate(weights):
        span = usable * (weight / total)
        columns.append((cursor + pad, inner_y + pad, span - pad * 2, inner_h - pad * 2))
        cursor += span
        if index < len(weights) - 1:
            dividers.append(cursor)
            cursor += divider
    return columns, dividers, (inner_y, inner_y + inner_h)


def column(frame, index, pad=PAD):
    """열 하나를 콘텐츠 사각형으로. 그림의 분할선 안쪽으로만 준다."""
    x0, x1 = frame["cols"][index]
    y0, y1 = frame["rows"]
    return (x0 + pad, y0 + pad, (x1 - x0) - pad * 2, (y1 - y0) - pad * 2)


def window(frame, pad=PAD, size=None):
    box = frame["window"]
    if frame.get("relative"):
        base = size or (W, H)
        box = (box[0] * base[0], box[1] * base[1], box[2] * base[0], box[3] * base[1])
    return (box[0] + pad, box[1] + pad, box[2] - pad * 2, box[3] - pad * 2)


def stack(rect, weights, gap=16.0):
    """세로 칸을 비율대로 나눈다. 열 안에서 섹션을 쌓을 때 쓴다."""
    x, y, width, height = rect
    total = sum(weights)
    usable = height - gap * (len(weights) - 1)
    out, cursor = [], y
    for weight in weights:
        piece = usable * (weight / total)
        out.append((x, cursor, width, piece))
        cursor += piece + gap
    return out
