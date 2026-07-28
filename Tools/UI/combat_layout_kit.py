"""Shared parts for building the combat HUD layout variants.

Ten layouts have to be comparable, so they share one set of components and
one palette; only the arrangement differs. Anything a layout does not want it
simply does not create -- UCombatLayoutHUDWidget finds widgets by name and
skips the ones that are missing.

Art lives in SVN, not git: Content/SVN is a junction to the SVN working copy,
so the textures are addressed as /Game/SVN/OutSideAsset/UI/CombatHUD/... .
"""
import unreal

PACKAGE_PATH = "/Game/UI/CombatLayouts"
ART = "/Game/SVN/OutSideAsset/UI/CombatHUD"
C04 = ART + "/Concept04"
KK = "/Game/SVN/OutSideAsset/UI/KayKit"
SLICE = KK + "/Slices"
CHROME = KK + "/Chrome"

#: 시안에서 통째로 오려 낸 HUD 껍데기.
#:
#: 조각을 만들어 이어 붙이거나 9슬라이스로 늘리는 대신, 시안의 판 묶음을
#: 그대로 한 장씩 놓는다. 스킬 카드 여섯 장이 한 장, 아군 세 줄이 한 장이다.
#: 늘리지 않으므로 몰딩도 비례도 시안 그 자체다 -- 우리가 맞출 것은 위치뿐이다.
#:
#: 값은 (텍스처, 시안 1672 화면에서의 x, y, 폭, 높이). 설계 1920 좌표는
#: chrome() 이 환산한다.
#: 마지막 값은 앵커 -- 이 껍데기가 어느 모서리에 붙어 사는가.
#:
#: 모바일은 해상도가 제각각이라 한 캔버스에 절대 좌표로 박으면 반드시 잘린다.
#: 실제로 창을 끌어 보니 세로가 짧으면 아래 판이 화면 밖으로 나가고, 세로가
#: 길면 UI 배율이 커져 스킬 줄 오른쪽이 잘렸다.
#:
#: 각 껍데기에 제 구역을 준다. 왼쪽 것은 왼쪽 가장자리에, 오른쪽 것은 오른쪽
#: 가장자리에 붙어 화면이 넓어지면 같이 벌어지고 좁아지면 같이 모인다.
CHROME_PARTS = {
    "round":     ("KK_Chrome_Round",     13,  10, 248, 126, "tl"),
    "turn":      ("KK_Chrome_TurnRow",   492, 11, 689, 124, "tc"),
    "objective": ("KK_Chrome_Objective", 1297, 11, 362, 124, "tr"),
    "party":     ("KK_Chrome_Party",     14,  589, 443, 330, "bl"),
    "skills":    ("KK_Chrome_Skills",    466, 616, 884, 307, "bc"),
    "enemy":     ("KK_Chrome_Enemy",     1357, 617, 308, 199, "br"),
    "endturn":   ("KK_Chrome_EndTurn",   1357, 824, 308, 100, "br"),
}

#: 껍데기에 뚫린 구멍의 자리. 조각 안 좌표이고, 내용은 여기에 앉는다.
#:
#: 손으로 적은 값이 아니라 조각에서 어두운 구멍을 찾아 읽은 것이다 -- 초상
#: 자리도 카드 칸도 배지 자리도 그림이 이미 알고 있다.
CHROME_HOLES = {
    "turn_portrait": [(26, 18), (162, 19), (301, 18), (442, 19), (581, 19)],
    "turn_portrait_size": (81, 84),
    "party_portrait": [(31, 16), (27, 123), (27, 231)],
    "party_portrait_size": (68, 72),
    "party_row_y": [5, 115, 223],
    "party_row_h": 107,
    "skill_card_x": [4, 148, 305, 453, 593, 740],
    "skill_card_y": 20,
    "skill_card_size": (127, 269),
    "skill_badge_y": 16,
    "skill_badge_size": (29, 31),
    "enemy_portrait": (33, 30),
    "enemy_portrait_size": (79, 83),
}

#: 시안은 1672 화면, 배치는 1920 캔버스에서 짠다.
CHROME_SCALE = 1920.0 / 1672.0


#: 시안에서 뽑은 선택 테두리. 대상보다 크게 그려져 바깥을 감싼다.
#:
#: 처음엔 내가 금색 사각형을 그려 넣었는데, 시안의 테두리는 모서리에 금속
#: 물림쇠가 있고 두께도 변에 따라 다르다. 그린 사각형으로는 그 맛이 안 난다.
#: 시안의 선택 상태 조각에서 금색만 뽑아 부품으로 만들었다.
#:
#: 값은 (텍스처, 조각 폭, 조각 높이).
CHROME_SELECT = {
    "party": ("KK_Chrome_SelectParty", 445, 120),
    "skill": ("KK_Chrome_SelectSkill", 152, 306),
    "turn":  ("KK_Chrome_SelectTurn", 136, 128),
}


def chrome_select(blueprint, name, parent, kind, w, h, parent_size):
    """선택 테두리를 대상 한가운데에 겹쳐 놓는다.

    테두리 조각이 대상보다 크므로 가운데를 맞추고 바깥으로 넘치게 둔다 --
    시안이 그렇게 그려져 있다. 런타임이 이 위젯 하나만 켜고 끄면 된다.
    """
    texture, sw, sh = CHROME_SELECT[kind]
    k = CHROME_SCALE
    aw, ah = sw * k, sh * k
    return image(blueprint, name, parent, (w - aw) / 2.0, (h - ah) / 2.0,
                 aw, ah, parent_size, z_order=Z_MARKER,
                 texture="{}/{}".format(CHROME, texture), tint=WHITE)


def chrome(blueprint, root, key, name=None):
    """시안에서 오려 낸 껍데기 한 장을 제자리에 놓는다.

    돌려주는 값은 (그 껍데기를 덮는 캔버스 이름, 캔버스 크기, 배율).
    내용은 그 캔버스 안에 조각 좌표 * 배율 로 놓으면 시안과 같은 자리에 온다.
    """
    texture, sx, sy, sw, sh, anchor = CHROME_PARTS[key]
    k = CHROME_SCALE
    x, y = sx * k, sy * k
    w, h = sw * k, sh * k
    holder = name or ("Chrome_" + key.capitalize())
    add(blueprint, "CanvasPanel", holder, root)
    # 자기 구역 모서리에 붙인다. 전부 좌상단으로 놓았더니 화면이 넓어져도
    # 오른쪽 판들이 왼쪽에 남아 가운데가 텅 비었다.
    place(blueprint, holder, x, y, w, h, anchor, None, Z_FILL)
    image(blueprint, holder + "_Art", holder, 0, 0, w, h, (w, h),
          z_order=Z_FILL, texture="{}/{}".format(CHROME, texture), tint=WHITE)
    return holder, (w, h), k

#: 시안에서 통째로 뜬 9슬라이스 판.
#:
#: 조각을 만들어 이어 붙이는 방식으로는 손그림 품질에 못 닿았다 -- 직선 구간의
#: 진행 방향 요동이 0.02~0.25 인데 시안은 9.67~12.50 이다. 몰딩이 없어 길게 뽑은
#: 플라스틱 막대로 읽힌다. 시안은 이미 완성 품질이므로 그걸 부품으로 만든다.
#:
#: 값은 (텍스처, 이미지 크기, BOX 여백 x, BOX 여백 y). 여백은 프레임 두께를
#: 이미지 크기로 나눈 것이고, 그만큼이 모서리로 남고 나머지가 늘어난다.
SLICES = {
    "party":      ("Slice_Card_Wood", (248, 109), 0.0806, 0.1835),
    "party_lead": ("Slice_Card_Wood", (248, 109), 0.0806, 0.1835),
    "command":    ("Slice_Card_Stone", (162, 308), 0.1358, 0.0714),
    "turn":       ("Slice_Card_Wood", (248, 109), 0.0806, 0.1835),
    "enemy":      ("Slice_Panel_Red", (307, 198), 0.0717, 0.1111),
    "info":       ("Slice_Plate_Parch", (248, 109), 0.0806, 0.1835),
    "action":     ("Slice_Button", (319, 100), 0.0564, 0.1800),
}

CANVAS_W, CANVAS_H = 1920.0, 1080.0
MARGIN = 20.0

# ─── grid and tokens ──────────────────────────────────────────────────────────
#
# One number runs the whole kit: UNIT. Every part's canvas is a multiple of it
# and every joint lands on a multiple of it, so assembling a panel is addition
# rather than measurement. The old art line needed CORNER_RATIO, BAND_MIN and a
# nine-slice margin of 0.312253, all of them read off the drawing after the
# fact -- and a mistake in the last one made the selection frame swallow a card.

#: Design pixels per grid unit. Parts are drawn at 4x this and drawn down.
UNIT = 32.0

#: Frame weights. `corner` and `link` are the sizes the pieces draw at; `band`
#: is how much of the link is moulding, which is what content has to clear.
#: The band ratio is a property of the drawn moulding, read once off the master
#: -- but nothing depends on it lining up, because the pieces were cut from one
#: continuous drawing and provably share a cross-section.
#: `rail`은 직선 구간을 화면에 그릴 두께다. 시안 실측: 레일은 ~10px 얇은
#: 선이고 모서리 브래킷만 덩어리다. 원판의 띠를 그대로 그리면 레일이
#: 모서리만큼 두꺼워져 테두리가 화면의 주인공이 된다 -- 4차 반영본이
#: 정확히 그랬다.
#: 레일 두께는 시안 실측에서 나온다. 예전 값(18/10)은 내 격자에서 고른
#: 것이라 화면에 7px로 그려졌고, 시안은 12px였다 -- 테두리가 얇아 카드가
#: 큼직하게 안 읽히는 주된 원인이었다. 아트가 위젯 폭의 약 70%를 그리므로
#: 12 / 0.7 = 17 로 역산한다.
HEAVY = {"corner": 2 * UNIT, "link": UNIT, "rail": 26.0,
         "band": UNIT * 85.0 / 128.0, "prefix": "KK_HFrame"}
LIGHT = {"corner": UNIT, "link": UNIT / 2.0, "rail": 17.0,
         "band": UNIT * 34.0 / 128.0, "prefix": "KK_LFrame"}

#: 금속 프레임. 나무와 단면 두께가 같게 그려져서 치수는 그대로 쓰고 접두사만
#: 바뀐다 -- 잘라낸 조각의 이음면 두께를 재서 확인했다(둘 다 128 / 64).
METAL = dict(HEAVY, prefix="KK_MFrame")
METAL_LIGHT = dict(LIGHT, prefix="KK_MLFrame")

#: 어느 역할이 어느 프레임을 쓰는가.
#:
#: 지금까지는 전 패널이 같은 나무 테두리였고, 역할 구분이 면 색에만 있었다.
#: 목업은 스킬 카드를 금속으로, 아군 카드를 나무로 그린다 -- 테두리가 먼저
#: 읽히므로 여기서 갈라야 형태만 봐도 무엇인지 안다.
FRAME_FAMILY = {
    "party": ("wood", "wood"),
    "party_lead": ("wood", "wood"),
    "info": ("wood", "wood"),
    "command": ("metal", "metal"),
    "enemy": ("metal", "metal"),
    "turn": ("metal", "metal"),
    "action": ("button", "button"),
}
FRAME_SETS = {
    "wood": (HEAVY, LIGHT),
    "metal": (METAL, METAL_LIGHT),
}

#: 시안 파일을 픽셀로 재서 잡은 프레임 보정색.
#:
#: 시안의 파티 프레임은 (109,67,28) 어두운 호두색인데 4차 나무 원판은 밝은
#: 주황으로 나왔다 -- 색상은 같고 밝기·채도만 높아 곱색으로 내려앉힌다.
#: 금속은 시안 (118,120,123)보다 어두워 1을 넘는 값으로 올린다. 임시 보정이고
#: 다음 원판 발주에 이 실측값이 목표로 들어간다.
#: 흰색은 아래 팔레트에서 정의된다. 여기서 이름으로 부르면 정의 전 참조라
#: 모듈이 통째로 안 읽힌다 -- 실제로 그렇게 죽었다.
FRAME_TINTS = {
    "wood": unreal.LinearColor(1.0, 1.0, 1.0, 1.0),
    "metal": unreal.LinearColor(1.0, 1.0, 1.0, 1.0),
}

SELECT_CORNER = UNIT       # KK_Select_Corner draws at 1U
SELECT_LINK = UNIT / 2.0   # KK_Select_Link_* draw at 0.5U
SELECT_BLEED = 4.0         # how far the marker sits outside its panel

FILL_TILE = 4 * UNIT       # KK_Fill_* are 4U squares
RING_HOLE_RATIO = 11.0 / 16.0
GEM = UNIT                 # KK_Gem_* draw at 1U
ICON = 2 * UNIT            # KK_Icon_* are 2U, glyph inside the middle 3/4
BAR_TILE = UNIT / 2.0      # KK_Bar_* are 0.5U wide, 1U tall
VEIL_TILE = UNIT

# KayKit palette. Bright matte, no oxidised bronze -- the characters are light
# low-poly chibi and the old dark-fantasy HUD read as a different game.
WHITE = unreal.LinearColor(1.0, 1.0, 1.0, 1.0)
TEXT_COLOR = unreal.LinearColor(0.99, 0.97, 0.93, 1.0)
TEXT_DIM = unreal.LinearColor(0.86, 0.84, 0.80, 1.0)
GOLD = unreal.LinearColor(0.91, 0.72, 0.29, 1.0)          # #E8B84B
#: 양피지·주황처럼 밝은 판 위에 얹는 글씨. 흰색은 씻겨서 안 읽힌다.
TEXT_ON_LIGHT = unreal.LinearColor(0.22, 0.13, 0.07, 1.0)
INK = unreal.LinearColor(0.13, 0.14, 0.17, 0.92)
#: 바 곱색. 바 원화는 중립 회색(158)이라 이 값이 곧 화면 색을 정한다.
#:
#: 원화가 초록이던 시절에는 이 곱색이 그 초록에 맞춰져 있었다. 원화를 중립으로
#: 바꾸자(적 바가 빨강 곱색과 곱해져 올리브로 나오던 문제) 같은 곱색이 아군 바를
#: 청록으로, 적 바를 분홍으로 만들었다. 시안 색을 재서 역산했다 --
#: 아군 (149,197,46), 적 (153,50,34).
HP_GREEN = unreal.LinearColor(0.884, 1.616, 0.066, 1.0)
HP_RED = unreal.LinearColor(0.929, 0.079, 0.035, 1.0)
#: 빈 자리(트랙). 시안은 어두운 갈색 홈이다 -- 실측 (107,75,42).
#:
#: 트랙 원화도 중립으로 바꾸면서 흰색 곱색을 그대로 뒀더니 밝기 213 이 그대로
#: 나가 빈 자리가 흰 막대가 됐다. 채움만 보고 트랙을 안 본 것이다.
HP_TRACK = unreal.LinearColor(0.213, 0.101, 0.031, 1.0)
#: 못 쓰는 카드를 덮는 막. 시안은 회색으로 죽이지 검게 지우지 않는다 --
#: 아이콘이 읽혀야 왜 못 쓰는지 알 수 있다.
#: 선택 표시용 금빛 덮개. 오려 낸 테두리가 그 판 크기에 안 맞을 때 쓴다.
SELECT_VEIL = unreal.LinearColor(1.0, 0.78, 0.22, 0.30)
DISABLED_VEIL = unreal.LinearColor(0.06, 0.06, 0.07, 0.50)
STONE = unreal.LinearColor(0.76, 0.78, 0.81, 1.0)         # #C3C7CE
STONE_DIM = unreal.LinearColor(0.43, 0.45, 0.49, 1.0)     # #6E747E
AP_ON = WHITE
AP_OFF = unreal.LinearColor(0.45, 0.47, 0.51, 0.9)
EMPTY_SOCKET = unreal.LinearColor(0.30, 0.32, 0.36, 1.0)
SELECT_TINT = WHITE        # the marker is already gold in the art

#: 수치마다 색을 준다. 전부 흰 계열이면 위계가 글자 크기로만 갈리고, 훑을 때
#: 무엇이 HP이고 무엇이 행동력인지 형태를 읽어야 알 수 있다.
HP_TEXT = unreal.LinearColor(1.00, 0.86, 0.84, 1.0)
AP_TEXT = unreal.LinearColor(1.00, 0.88, 0.52, 1.0)
DAMAGE_TEXT = unreal.LinearColor(0.98, 0.95, 0.88, 1.0)
COOLDOWN_TEXT = unreal.LinearColor(1.00, 0.80, 0.45, 1.0)

#: 판 안쪽 위에 얹는 밝은 선과 아래에 까는 어두운 선. 이 두 줄이 "깎아 만든
#: 판"과 "색칠한 사각형"을 가른다. 목업의 면은 전부 이 층을 갖고 있다.
RIM_LIGHT = unreal.LinearColor(1.0, 0.97, 0.90, 0.35)
RIM_DARK = unreal.LinearColor(0.0, 0.0, 0.0, 0.50)
CARD_SHADOW = unreal.LinearColor(0.0, 0.0, 0.0, 0.45)

#: 숫자를 얹는 배지. 밝은 원에 진한 숫자여야 작은 크기에서 숫자가 산다.
BADGE_FACE = unreal.LinearColor(1.00, 0.96, 0.90, 1.0)
#: 시안 배지는 어두운 갈색 원에 흰 숫자다. 진한 글씨로 칠했더니 원판도
#: 어두워 숫자가 통째로 묻혔다 -- 실측에서 아주밝은 픽셀 0개였다.
BADGE_TEXT = unreal.LinearColor(1.0, 0.98, 0.94, 1.0)

#: Portraits we actually have art for. A slot with none keeps an empty socket
#: rather than borrowing another unit's face -- the runtime overwrites it as
#: soon as the model supplies one.
#: 초상은 얼굴만 잘라 낸 판을 쓴다.
#:
#: 전신 렌더를 그대로 넣었더니 큰 원 안에 사람이 콩알만 하게 들어갔다 --
#: 캡처를 시안과 견주는 검사에서 턴 칸이 가장 나쁘게 나온 이유가 그것이다.
#: 시안 초상은 얼굴과 어깨가 원을 꽉 채운다.
HEADS = "/Game/SVN/OutSideAsset/UI/KayKit/Heads"

PARTY_PORTRAITS = (
    HEADS + "/KK_Face_Knight_HeadV2",
    HEADS + "/KK_Face_Ranger_HeadV2",
    HEADS + "/KK_Face_Mage_HeadV2",
)

#: 턴 순서 칸의 기본 얼굴. 게임플레이가 유닛 초상화를 주면 덮어쓴다.
#: 지금은 미리보기 장면(기사 - 독수리 - 궁수 - 독수리 - 마법사)에 맞춘다.
TURN_PORTRAITS = (
    HEADS + "/KK_Face_Knight_HeadV2",
    HEADS + "/KK_Face_Enemy_Eagle_HeadV2",
    HEADS + "/KK_Face_Ranger_HeadV2",
    HEADS + "/KK_Face_Enemy_Eagle_HeadV2",
    HEADS + "/KK_Face_Mage_HeadV2",
    None,
)

#: Which painted glyph each command slot shows. Slot 0 is 이동; the rest follow
#: the mock skill order.
COMMAND_ICONS = (
    KK + "/KK_Icon_Move",
    KK + "/KK_Icon_BasicAttack",
    KK + "/KK_Icon_ShieldBash",
    KK + "/KK_Icon_PinSlash",
    KK + "/KK_Icon_Breakthrough",
    KK + "/KK_Icon_Riposte",
)


#: 8x256 세로 그라데이션. 알파 램프라 아트라인과 무관하게 쓰인다.
#: 위가 밝고 아래가 어두운 조명은 KayKit도 같으므로 그대로 가져다 쓴다.
SHADE = C04 + "/T_C04_Shade"
SHADE_SIZE = (8.0, 256.0)

#: 역할마다 다른 면과 색조.
#:
#: 처음엔 모든 패널이 KK_Fill_Stone 하나를 썼고, 그 Stone은 값 범위가
#: 132~148뿐이라 사실상 단색이었다. 화면 전체가 같은 회색 판으로 보였다.
#: 면을 나누고 색조를 주면 어느 판이 무엇인지 형태 전에 색으로 읽힌다.
SURFACES = {
    "party":      ("Wood", WHITE),
    "party_lead": ("Wood_Active", WHITE),
    "command":    ("Stone_Skill", WHITE),
    "enemy":      ("Stone_Enemy", WHITE),
    "turn":       ("Wood", WHITE),
    "info":       ("Parchment", WHITE),
    "action":     ("ButtonWood", WHITE),
}


#: 그리기 층. 캔버스 자식 순서에만 기대면 부품이 많은 카드에서 프레임이
#: 사라진다 -- 아군 카드와 커맨드 카드가 정확히 그렇게 됐다.
Z_SHADOW, Z_FILL, Z_CONTENT, Z_FRAME, Z_OVERLAY, Z_MARKER = (
    -10, 0, 10, 20, 30, 40)


def snap(value, step):
    """Round a length up to the grid so a frame lands on whole pieces."""
    import math
    return max(step, math.ceil(value / step) * step)


helper = unreal.MCPythonHelper
_LOADED = {}


def art(path):
    """Load a piece of art once, and fail loudly if it is not there.

    A missing brush resource does not error at runtime -- the widget just draws
    a white box, which is easy to mistake for a placeholder left in on purpose.
    """
    if path not in _LOADED:
        asset = unreal.EditorAssetLibrary.load_asset(path)
        if asset is None:
            raise RuntimeError("art missing: {}".format(path))
        _LOADED[path] = asset
    return _LOADED[path]


# ─── asset ────────────────────────────────────────────────────────────────────

#: 짓는 동안 쓰는 임시 꼬리표. commit_asset 이 제자리로 옮긴다.
BUILDING_SUFFIX = "__building"


def create_asset(asset_name, parent=None):
    """Make a widget blueprint parented to a C++ widget class.

    parent 를 주면 그 클래스를 부모로 삼는다. 안 주면 전투 배치안의 부모를
    쓴다 -- 화면이 늘어나면서 배치안마다 부모가 달라졌다.

    부모는 구울 때 정한다. 굽고 나서 에디터에서 손으로 바꾸면 다시 구울
    때마다 도로 풀린다 -- 굽기가 에셋을 새로 만들기 때문이다.

    임시 이름으로 만든다. 예전에는 기존 에셋을 지우고 새로 만들었는데, 그러면
    짓는 동안 그 배치안이 디스크에 아예 없다 -- 그 사이에 게임을 켜면
    RD.Layout 이 "못 찾음"으로 죽고, 생성이 한 번 실패하면 지워진 채로 남는다.
    실제로 그렇게 05번과 09번이 사라졌고 RD.Layout 1 이 됐다 안 됐다 했다.

    다 짓고 저장한 뒤에 commit_asset 이 자리를 바꾼다. 중간에 무엇이 터져도
    기존 배치안은 그대로 남는다.
    """
    temp = asset_name + BUILDING_SUFFIX
    temp_full = "{}/{}".format(PACKAGE_PATH, temp)
    if unreal.EditorAssetLibrary.does_asset_exist(temp_full):
        unreal.EditorAssetLibrary.delete_asset(temp_full)
    asset_name = temp

    factory = unreal.WidgetBlueprintFactory()
    parent_class = unreal.CombatLayoutHUDWidget
    if parent:
        found = unreal.load_object(None, parent)
        if found is None:
            raise RuntimeError("부모 클래스를 못 읽음: {}".format(parent))
        parent_class = found
    factory.set_editor_property("parent_class", parent_class)
    asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        asset_name, PACKAGE_PATH, unreal.WidgetBlueprint, factory)
    if asset is None:
        raise RuntimeError("could not create {}".format(full))
    # UWidgetBlueprint does not expose ParentClass to Python, so confirm the
    # parenting through the generated class instead of trusting the factory.
    generated = asset.generated_class()
    if generated is None or not unreal.MathLibrary.class_is_child_of(
            generated, parent_class):
        raise RuntimeError("{} 의 부모가 {} 가 아니다".format(
            temp_full, parent_class.get_name()))
    return asset


def commit_asset(asset_name):
    """다 지은 임시 에셋을 제자리로 옮긴다. 여기까지 와야 교체가 일어난다."""
    temp_full = "{}/{}{}".format(PACKAGE_PATH, asset_name, BUILDING_SUFFIX)
    full = "{}/{}".format(PACKAGE_PATH, asset_name)
    if not unreal.EditorAssetLibrary.does_asset_exist(temp_full):
        raise RuntimeError("nothing to commit at {}".format(temp_full))
    if unreal.EditorAssetLibrary.does_asset_exist(full):
        if not unreal.EditorAssetLibrary.delete_asset(full):
            raise RuntimeError("could not replace {}".format(full))
    if not unreal.EditorAssetLibrary.rename_asset(temp_full, full):
        raise RuntimeError("could not move {} into place".format(temp_full))


# ─── tree building ────────────────────────────────────────────────────────────

#: 짓는 동안 남기는 좌표 장부.
#:
#: WBP 에서 되읽으려 했지만 표준 파이썬은 widget_tree 에 못 닿고, 헬퍼는
#: 이름 조회만 준다. 어차피 이 키트가 모든 좌표를 정하므로 정할 때 적어 둔다.
#: 이 장부가 위젯별 정밀 비교(cut_widget_pairs.py)의 근거가 된다.
LEDGER = {"parent": {}, "rect": {}, "type": {}}


def reset_ledger():
    LEDGER["parent"].clear()
    LEDGER["rect"].clear()
    LEDGER["type"].clear()


def absolute_rects():
    """장부를 화면 좌표로 편다. 부모를 타고 올라가며 더한다."""
    out = []
    for name, (x, y, w, h) in LEDGER["rect"].items():
        ax, ay = x, y
        cursor = LEDGER["parent"].get(name)
        guard = 0
        while cursor and guard < 32:
            parent_rect = LEDGER["rect"].get(cursor)
            if parent_rect:
                ax += parent_rect[0]
                ay += parent_rect[1]
            cursor = LEDGER["parent"].get(cursor)
            guard += 1
        out.append({"name": name, "type": LEDGER["type"].get(name, "?"),
                    "x": round(ax, 1), "y": round(ay, 1),
                    "w": round(w, 1), "h": round(h, 1)})
    out.sort(key=lambda e: (e["y"], e["x"]))
    return out


def add(blueprint, widget_type, name, parent):
    LEDGER["parent"][name] = parent or None
    LEDGER["type"][name] = widget_type
    result = helper.umg_add_widget(blueprint, widget_type, name, parent)
    if '"success":true' not in result.replace(" ", ""):
        raise RuntimeError("add {} {} under {} -> {}".format(
            widget_type, name, parent, result))
    return helper.umg_find_widget(blueprint, name)


ANCHORS = {
    "tl": (0.0, 0.0), "tc": (0.5, 0.0), "tr": (1.0, 0.0),
    "ml": (0.0, 0.5), "mc": (0.5, 0.5), "mr": (1.0, 0.5),
    "bl": (0.0, 1.0), "bc": (0.5, 1.0), "br": (1.0, 1.0),
}


def place(blueprint, name, x, y, w, h, anchor="tl", parent_size=None,
          z_order=None):
    """Position a canvas child given absolute coordinates in its parent.

    `anchor` decides which parent edge the widget stays glued to when the
    screen is not 16:9 -- left blocks hug the left, centred rails stay centred,
    right blocks hug the right.
    """
    ax, ay = ANCHORS[anchor]
    pw, ph = parent_size if parent_size else (CANVAS_W, CANVAS_H)
    LEDGER["rect"][name] = (float(x), float(y), float(w), float(h))
    result = helper.umg_set_slot_layout(
        blueprint, name, ax, ay, ax, ay, x - ax * pw, y - ay * ph, w, h)
    if '"success":true' not in result.replace(" ", ""):
        raise RuntimeError("place {} -> {}".format(name, result))
    if z_order is not None:
        # 캔버스는 자식 순서대로 그리지만, 순서만 믿으면 부품이 많은 카드에서
        # 뒤로 밀려 사라진다. 프레임과 표시물은 층을 못 박는다.
        widget = helper.umg_find_widget(blueprint, name)
        widget.get_editor_property("slot").set_editor_property("z_order",
                                                              int(z_order))


def brush_of(widget):
    """The brush property, which is named differently per widget type."""
    for field in ("brush", "background"):
        try:
            return field, widget.get_editor_property(field)
        except Exception:
            continue
    raise RuntimeError("no brush on {}".format(type(widget).__name__))


def paint(widget, texture=None, tint=None, size=None, tiling=None, margin=None):
    """Point a widget's brush at art.

    `tiling` repeats the source at its own size instead of scaling it, which is
    the difference between a painted grain that stays the thickness it was
    drawn and one that smears. `margin` switches the brush to nine-slice so the
    corners keep their shape while the middle stretches.
    """
    field, brush = brush_of(widget)
    if texture is not None:
        brush.set_editor_property("resource_object", art(texture))
    if size is not None:
        # ImageSize is FDeprecateSlateVector2D, not FVector2D. Python exposes
        # the type but not a constructor that takes values, so it is built
        # empty and filled through its own fields.
        extent = unreal.DeprecateSlateVector2D()
        extent.set_editor_property("x", float(size[0]))
        extent.set_editor_property("y", float(size[1]))
        brush.set_editor_property("image_size", extent)
    if margin is not None:
        # 여백은 이미지 크기에 대한 비율이고 축마다 다르다. 가로로 긴 판에서
        # 한 값을 네 변에 쓰면 세로 모서리가 뭉개진다.
        mx, my = margin if isinstance(margin, (tuple, list)) else (margin, margin)
        brush.set_editor_property("draw_as", unreal.SlateBrushDrawType.BOX)
        brush.set_editor_property("margin", unreal.Margin(mx, my, mx, my))
    elif tiling is not None:
        brush.set_editor_property("draw_as", unreal.SlateBrushDrawType.IMAGE)
        brush.set_editor_property("tiling", tiling)
    if tint is not None:
        brush.set_editor_property("tint_color", unreal.SlateColor(tint))
    widget.set_editor_property(field, brush)
    return widget


def image(blueprint, name, parent, x, y, w, h, parent_size=None,
          z_order=None, **paint_args):
    z_order = Z_CONTENT if z_order is None else z_order
    widget = add(blueprint, "Image", name, parent)
    if paint_args:
        paint(widget, **paint_args)
    place(blueprint, name, x, y, w, h, "tl", parent_size, z_order)
    return widget


def fold(blueprint, name):
    """위젯을 접어 둔다. 런타임이 필요할 때 펼친다.

    선택 테두리와 사용불가 덮개는 카드를 통째로 덮는 판이라, 펼친 채로
    구워 두면 두 가지가 깨진다. 에디터에서 그 밑에 있는 이름과 막대를 찍을
    수가 없고(위젯 이름을 계층에서만 고를 수 있었다), 화면에서는 아군 세 줄과
    카드 여섯 장이 전부 선택된 것처럼 보인다.

    런타임은 켤 때 SelfHitTestInvisible, 끌 때 Collapsed 로 둔다
    (CombatLayoutHUDWidget 의 SetShown). 접힌 상태로 구우면 그 규칙과 맞물려
    저절로 바른 값이 된다.
    """
    widget = helper.umg_find_widget(blueprint, name)
    if widget is not None:
        widget.set_editor_property("visibility",
                                   unreal.SlateVisibility.COLLAPSED)
    return widget


#: HUD 글자 폰트. Oswald 를 기본으로 두고 한글은 그 안에서 LINE Seed 로
#: 떨어진다 -- build_oswald_font.py 가 만든 합성 폰트다.
HUD_FONT = "/Fonts/F_HUD_Oswald"

#: 글자 테두리. 글자 크기의 이 비율만큼 두른다.
#:
#: 판이 나무와 가죽이라 바탕색이 자리마다 다르다. 테두리가 없으면 밝은 양피지
#: 위의 미색 글자가 사라지고, 어두운 판 위의 진갈색 글자도 마찬가지다.
OUTLINE_RATIO = 0.09
OUTLINE_COLOR = unreal.LinearColor(0.0, 0.0, 0.0, 1.0)

#: 그림자. 오른쪽 아래로 조금.
SHADOW_OFFSET = (2.0, 3.0)
SHADOW_COLOR = unreal.LinearColor(0.0, 0.0, 0.0, 0.55)


def hud_font(size, bold=False):
    info = unreal.SlateFontInfo()
    info.set_editor_property("font_object", art(KK + HUD_FONT))
    info.set_editor_property("typeface_font_name",
                             unreal.Name("Bold" if bold else "Regular"))
    info.set_editor_property("size", int(size))

    # 테두리는 폰트 정보 안에 있다. 글자마다 따로 두르는 것이 아니라 이 값이
    # 폰트 캐시에 함께 구워진다.
    outline = info.get_editor_property("outline_settings")
    outline.set_editor_property("outline_size",
                                max(1, int(round(size * OUTLINE_RATIO))))
    outline.set_editor_property("outline_color", OUTLINE_COLOR)
    # 테두리를 글자 바깥으로만 그린다. 안쪽까지 먹으면 작은 글자가 뭉갠다.
    outline.set_editor_property("apply_outline_to_drop_shadows", False)
    info.set_editor_property("outline_settings", outline)
    return info


def label(blueprint, name, parent, x, y, w, h, text, size=14,
          color=TEXT_COLOR, align="left", parent_size=None, bold=False):
    block = add(blueprint, "TextBlock", name, parent)
    block.set_editor_property("text", unreal.Text(text))
    block.set_editor_property("font", hud_font(size, bold))
    block.set_editor_property("color_and_opacity", unreal.SlateColor(color))
    # 그림자. 테두리만으로는 바탕과 붙어 보여서 한 겹 더 띄운다.
    block.set_editor_property("shadow_offset",
                              unreal.Vector2D(*SHADOW_OFFSET))
    block.set_editor_property("shadow_color_and_opacity", SHADOW_COLOR)
    block.set_editor_property("justification", {
        "left": unreal.TextJustify.LEFT,
        "center": unreal.TextJustify.CENTER,
        "right": unreal.TextJustify.RIGHT,
    }[align])
    place(blueprint, name, x, y, w, h, "tl", parent_size, Z_CONTENT)
    return block


#: 글자를 칸 안 어디에 붙일지. (가로, 세로) -> 칸 안 비율.
ALIGN_SPOT = {
    "left": 0.0, "center": 0.5, "right": 1.0,
    "top": 0.0, "middle": 0.5, "bottom": 1.0,
}


def align_in(blueprint, name, x, y, w, h, halign="center", valign="middle",
             parent_size=None):
    """글자를 그 칸 안에서 가로·세로로 붙인다.

    ## 왜 따로 필요한가

    캔버스 칸은 자리와 크기만 정한다. 글자는 그 칸의 **왼쪽 위**에서 시작하고,
    justification 은 가로만 옮긴다 -- 세로는 손댈 데가 없어 칸이 클수록
    글자가 위로 떠 보인다.

    그래서 칸을 자동 크기로 바꾸고, 글자 자신의 어느 점을 칸의 어느 점에
    맞출지로 붙인다. 자동 크기라 글자가 길어도 안 잘리고 옆으로 넘친다 --
    잘리는 것보다는 넘치는 편이 낫다. 잘리면 무엇이 잘렸는지 안 보인다.
    """
    ax, ay = ANCHORS["tl"]
    pw, ph = parent_size if parent_size else (CANVAS_W, CANVAS_H)
    hx = ALIGN_SPOT[halign]
    vy = ALIGN_SPOT[valign]

    widget = helper.umg_find_widget(blueprint, name)
    slot = widget.get_editor_property("slot")
    slot.set_editor_property("auto_size", True)

    # 붙일 점은 칸(slot)이 아니라 그 안의 배치 자료(layout_data)에 있다.
    layout = slot.get_editor_property("layout_data")
    layout.set_editor_property("alignment", unreal.Vector2D(hx, vy))
    offsets = layout.get_editor_property("offsets")
    offsets.set_editor_property("left", x + w * hx - ax * pw)
    offsets.set_editor_property("top", y + h * vy - ay * ph)
    layout.set_editor_property("offsets", offsets)
    slot.set_editor_property("layout_data", layout)


def bar(blueprint, name, parent, x, y, w, h, fill, parent_size=None):
    """A progress bar dressed with the KayKit rail pieces.

    Slate draws the bar itself, so this is the one place a brush still gets
    stretched -- but the pieces tile along the bar's axis at their own size,
    so the rounded ends keep their shape instead of smearing.
    """
    progress = add(blueprint, "ProgressBar", name, parent)
    style = progress.get_editor_property("widget_style")
    for slot, texture, tint in (
            ("background_image", KK + "/KK_Bar_Track_Link", HP_TRACK),
            ("fill_image", KK + "/KK_Bar_Link", fill)):
        brush = style.get_editor_property(slot)
        brush.set_editor_property("resource_object", art(texture))
        brush.set_editor_property("draw_as", unreal.SlateBrushDrawType.IMAGE)
        brush.set_editor_property("tiling", unreal.SlateBrushTileType.HORIZONTAL)
        extent = unreal.DeprecateSlateVector2D()
        extent.set_editor_property("x", BAR_TILE)
        extent.set_editor_property("y", float(h))
        brush.set_editor_property("image_size", extent)
        brush.set_editor_property("tint_color", unreal.SlateColor(tint))
        style.set_editor_property(slot, brush)
    progress.set_editor_property("widget_style", style)
    progress.set_editor_property("fill_color_and_opacity", WHITE)
    progress.set_editor_property("percent", 0.75)
    # 새 바 아트는 여백 없이 캔버스를 꽉 채운다. 옛 아트의 상하 여백을
    # 가정하고 2배 높이로 그리면 뚱뚱한 초록 덩어리가 된다.
    place(blueprint, name, x, y, w, h, "tl", parent_size, Z_CONTENT)
    return progress


def ghost_button(blueprint, name, parent, x, y, w, h, parent_size=None,
                 z_order=None):
    """A hit area with no chrome of its own, so the plate under it shows."""
    button = add(blueprint, "Button", name, parent)
    style = button.get_editor_property("widget_style")
    for state in ("normal", "hovered", "pressed", "disabled"):
        brush = style.get_editor_property(state)
        brush.set_editor_property("resource_object", None)
        brush.set_editor_property("tint_color", unreal.SlateColor(
            unreal.LinearColor(1.0, 0.95, 0.85,
                               0.10 if state == "hovered" else 0.0)))
        style.set_editor_property(state, brush)
    button.set_editor_property("widget_style", style)
    place(blueprint, name, x, y, w, h, "tl", parent_size,
          Z_OVERLAY - 1 if z_order is None else z_order)
    return button


# ─── KayKit frame ─────────────────────────────────────────────────────────────

def flip(blueprint, name, horizontal=False, vertical=False):
    """Mirror a placed piece. Four corners come from one drawing."""
    widget = helper.umg_find_widget(blueprint, name)
    scale = unreal.Vector2D(-1.0 if horizontal else 1.0,
                            -1.0 if vertical else 1.0)
    transform = widget.get_editor_property("render_transform")
    transform.set_editor_property("scale", scale)
    widget.set_editor_property("render_transform", transform)
    widget.set_editor_property("render_transform_pivot",
                               unreal.Vector2D(0.5, 0.5))


def frame(blueprint, prefix, parent, w, h, weight=None, family="wood"):
    """아무 것도 하지 않는다. 프레임은 이제 판 그림에 이미 들어 있다.

    예전에는 모서리 넷과 직선 조각들을 이어 붙여 테두리를 만들었다. 그렇게
    만든 프레임은 직선 구간의 진행 방향 요동이 0.02~0.25 였고 시안은
    9.67~12.50 이었다 -- 몰딩이 없어 길게 뽑은 막대로 읽혔다. 이음을 0으로
    만드는 데는 성공했지만 완성품으로 보이지는 않았다.

    지금은 시안에서 통째로 뜬 9슬라이스 판을 쓰므로 프레임이 그 한 장에 이미
    그려져 있다. 배치안 열 개가 이 함수를 여기저기서 부르고 있어, 지우는 대신
    비워 둔다 -- 호출을 다 걷어내면 diff 가 커지고 되돌리기 어려워진다.
    """
    return None


def button_frame(blueprint, prefix, parent, w, h, pressed=False):
    """The raised button plate, assembled from one corner and two links.

    A button needs a different silhouette from an information panel -- the
    end-turn control used the panel frame and so nothing about its shape said
    it could be pressed. This set has a lit top edge and a darker skirt, and a
    pressed variant whose outline matches to the pixel.
    """
    state = "Down" if pressed else "Up"
    size = (w, h)
    corner, link = UNIT, UNIT / 2.0

    def piece(tag, source, x, y, pw, ph, tiling=None, **flips):
        nm = "{}_B{}".format(prefix, tag)
        image(blueprint, nm, parent, x, y, pw, ph, size, z_order=Z_FRAME,
              texture="{}/KK_Button_{}_{}".format(KK, source, state),
              tint=WHITE, tiling=tiling,
              size=(link, link) if tiling is not None else None)
        if flips:
            flip(blueprint, nm, **flips)

    # 직선 구간은 타일 브러시 한 장 (frame()과 같은 이유).
    run_h, run_v = w - 2 * corner, h - 2 * corner
    piece("T", "Link_H", corner, 0, run_h, link,
          tiling=unreal.SlateBrushTileType.HORIZONTAL)
    piece("B", "Link_H", corner, h - link, run_h, link,
          tiling=unreal.SlateBrushTileType.HORIZONTAL, vertical=True)
    piece("L", "Link_V", 0, corner, link, run_v,
          tiling=unreal.SlateBrushTileType.VERTICAL)
    piece("R", "Link_V", w - link, corner, link, run_v,
          tiling=unreal.SlateBrushTileType.VERTICAL, horizontal=True)
    piece("CTL", "Corner", 0, 0, corner, corner)
    piece("CTR", "Corner", w - corner, 0, corner, corner, horizontal=True)
    piece("CBL", "Corner", 0, h - corner, corner, corner, vertical=True)
    piece("CBR", "Corner", w - corner, h - corner, corner, corner,
          horizontal=True, vertical=True)


def card(blueprint, name, parent, x, y, w, h, anchor="tl", parent_size=None,
         role="command"):
    """A panel: ink, tiled fill, then an inner canvas. The frame goes on last.

    Sizes snap to the grid so the frame divides into whole pieces. The inner
    canvas matters: hiding the card has to hide its contents, and a widget only
    hides its own subtree.
    """
    weight = HEAVY if min(w, h) >= 8 * UNIT else LIGHT
    w = snap(w, weight["link"])
    h = snap(h, weight["link"])

    # 계약 이름은 래퍼가 갖는다.
    #
    # 그림자를 판의 형제로 깔았더니, 런타임이 판을 감출 때 그림자만 남아
    # 화면에 회색 유령 사각형이 떴다. 턴 순서 빈 칸 자리에서 정확히 그랬다.
    # 감춰야 할 것들을 한 껍데기에 넣어야 그런 잔재가 안 생긴다.
    add(blueprint, "CanvasPanel", name, parent)
    place(blueprint, name, x, y, w, h, anchor, parent_size)
    size = (w, h)

    shadow = "{}_Shadow".format(name)
    paint(add(blueprint, "Image", shadow, name), tint=CARD_SHADOW)
    place(blueprint, shadow, -3, 5, w + 6, h + 4, "tl", size, Z_SHADOW)

    plate = "{}_Plate".format(name)
    border = add(blueprint, "Border", plate, name)
    border.set_editor_property("padding", unreal.Margin(0.0, 0.0, 0.0, 0.0))
    paint(border, tint=INK)
    place(blueprint, plate, 0, 0, w, h, "tl", size, Z_FILL)

    inner = "{}_Canvas".format(name)
    add(blueprint, "CanvasPanel", inner, plate)

    # 판 하나를 이미지 한 장으로 그린다.
    #
    # 예전에는 면을 타일로 깔고 그 위에 조명 램프와 테두리 두 줄을 얹고,
    # 마지막에 프레임 조각 스무 개를 이어 붙였다. 그렇게 만든 프레임은 직선
    # 구간의 진행 방향 요동이 0.02~0.25 였다 -- 시안은 9.67~12.50 이다. 몰딩이
    # 없으니 길게 뽑은 플라스틱 막대로 읽히고, 조각을 다듬어도 이음과 반복이
    # 남았다.
    #
    # 시안에서 완성된 판을 통째로 떠서 BOX 브러시로 쓴다. 네 모서리는 그린
    # 그대로 남고 가운데만 늘어난다. 조명·테두리·프레임이 전부 그 한 장에
    # 이미 들어 있으므로 코드가 더 얹을 것이 없다.
    #
    # 곁가지 효과: 카드 하나가 위젯 스무 개에서 한 개로 줄어 1안이 540개에서
    # 150개 근처가 된다. 나중에 UMG 에서 손으로 만질 수 있는 규모다.
    texture, extent, margin_x, margin_y = SLICES.get(role, SLICES["command"])
    image(blueprint, "{}_Fill".format(name), inner, 0, 0, w, h, size,
          z_order=Z_FILL, texture="{}/{}".format(SLICE, texture),
          tint=WHITE, size=extent, margin=(margin_x, margin_y))
    return inner, size


def ring(blueprint, name, parent, x, y, extent, tint, parent_size,
         enemy=False):
    """A portrait bezel, returning where the portrait goes inside it."""
    # 베젤을 초상화 위에 올린다.
    #
    # 게임플레이가 주는 초상화는 사각 스프라이트라 원형 구멍에 그대로 넣으면
    # 네 귀퉁이가 삐져나온다. 구멍에 내접한 사각형의 귀퉁이는 중심에서 124px,
    # 베젤은 88px부터 128px까지 불투명하니 위에 얹기만 하면 가려진다.
    inner = extent * RING_HOLE_RATIO
    offset = (extent - inner) / 2.0
    image(blueprint, name + "_Ring", parent, x, y, extent, extent, parent_size,
          z_order=Z_CONTENT + 2,
          texture="{}/KK_Ring_{}".format(KK, "Enemy" if enemy else "Portrait"),
          tint=tint)
    return x + offset, y + offset, inner


def tag(blueprint, name, parent, x, y, extent, glyph, parent_size):
    """A small pictogram that sits beside a number.

    Drawn at 2U in the art and used at 16~24px here, so the shapes were kept to
    one solid silhouette -- anything finer turns to mush at this size.
    """
    return image(blueprint, name, parent, x, y, extent, extent, parent_size,
                 texture="{}/KK_Tag_{}".format(KK, glyph), tint=WHITE)


def marker(blueprint, name, parent, w, h, parent_size, tint=None):
    """The selection bracket, assembled from a corner and two links.

    A single stretched ring is what swallowed a card last time: its corner was
    a fraction of the source, and the fraction was read off the drawing.
    """
    del tint
    # The runtime shows and hides this by name, so the group carries the
    # contract name and the pieces ride inside it.
    root = name
    add(blueprint, "CanvasPanel", root, parent)
    place(blueprint, root, -SELECT_BLEED, -SELECT_BLEED,
          w + 2 * SELECT_BLEED, h + 2 * SELECT_BLEED, "tl", parent_size,
          Z_MARKER)
    gw, gh = w + 2 * SELECT_BLEED, h + 2 * SELECT_BLEED
    size = (gw, gh)
    c, l = SELECT_CORNER, SELECT_LINK

    def piece(tag, source, x, y, pw, ph, tiling=None, **flips):
        nm = "{}_{}".format(name, tag)
        image(blueprint, nm, root, x, y, pw, ph, size,
              texture="{}/KK_Select_{}".format(KK, source), tint=WHITE,
              tiling=tiling, size=(l, l) if tiling is not None else None)
        if flips:
            flip(blueprint, nm, **flips)

    # 직선 구간은 타일 브러시 한 장. 조각마다 위젯을 놓으면 폭이 넓을수록
    # 개수가 폭발한다 -- 화면 폭 바 하나가 위젯 114개까지 갔다.
    piece("T", "Link_H", c, 0, gw - 2 * c, l,
          tiling=unreal.SlateBrushTileType.HORIZONTAL)
    piece("B", "Link_H", c, gh - l, gw - 2 * c, l,
          tiling=unreal.SlateBrushTileType.HORIZONTAL, vertical=True)
    piece("L", "Link_V", 0, c, l, gh - 2 * c,
          tiling=unreal.SlateBrushTileType.VERTICAL)
    piece("R", "Link_V", gw - l, c, l, gh - 2 * c,
          tiling=unreal.SlateBrushTileType.VERTICAL, horizontal=True)
    piece("CTL", "Corner", 0, 0, c, c)
    piece("CTR", "Corner", gw - c, 0, c, c, horizontal=True)
    piece("CBL", "Corner", 0, gh - c, c, c, vertical=True)
    piece("CBR", "Corner", gw - c, gh - c, c, c, horizontal=True, vertical=True)

    helper.umg_set_widget_is_variable(blueprint, root, True)


# ─── components ───────────────────────────────────────────────────────────────

def round_panel(blueprint, root, x, y, w=200.0, h=60.0, anchor="tl", size=18):
    body, extent = card(blueprint, "RoundPanel", root, x, y, w, h, anchor,
                        role="info")
    label(blueprint, "RoundText", body, 0, (h - size - 10) / 2.0, w, size + 10,
          "ROUND 1", size, TEXT_ON_LIGHT, "center", extent, bold=True)
    frame(blueprint, "RoundPanel", body, *extent, family="wood")


def objective_panel(blueprint, root, x, y, w=340.0, h=60.0, anchor="tr",
                    size=16, icon=True):
    body, extent = card(blueprint, "ObjectivePanel", root, x, y, w, h, anchor,
                        role="info")
    text_x = 8.0
    if icon:
        # 깃발 원화는 캔버스의 절반만 그림이다. 시안의 46px 깃발을 맞추려면
        # 위젯을 그만큼 키워야 한다.
        glyph = min(108.0, h - 6.0)
        image(blueprint, "ObjectiveIcon", body, 14, (h - glyph) / 2.0,
              glyph, glyph, extent,
              texture=KK + "/KK_Icon_Objective", tint=WHITE)
        # 깃발 원화는 캔버스 전체를 채우지 않는다. 투명 여백까지 전부 글자
        # 자리에서 빼면 긴 목표 문장이 오른쪽에서 잘린다.
        text_x = 14 + glyph * 0.62 + 8
    label(blueprint, "ObjectiveText", body, text_x, (h - size - 10) / 2.0,
          w - text_x - 12, size + 10, "목표", size, TEXT_ON_LIGHT, "center",
          extent)
    frame(blueprint, "ObjectivePanel", body, *extent, family="wood")


def turn_row(blueprint, root, x, y, token=96.0, gap=12.0, anchor="tc",
             count=6, names=True, framed=True, arrows=False, token_h=None):
    """The turn order, as a row of framed portrait tokens."""
    token_h = token if token_h is None else token_h
    for index in range(count):
        name = "TurnToken_{}".format(index)
        body, size = card(blueprint, name, root, x + index * (token + gap), y,
                          token, token_h, anchor, role="turn")
        # 이름표가 있으면 링을 줄여 프레임 안쪽에 이름 자리를 만든다. 프레임은
        # 내용보다 위 층이라, 자리를 안 비우면 이름 아래쪽이 잘린다.
        # 시안의 이름 없는 턴 토큰은 링이 카드 높이의 거의 전부를 쓴다.
        # 원화 자체의 투명 여백까지 감안하면 위젯을 카드보다 조금 크게
        # 잡아야 실제 보이는 링 지름이 시안과 같아진다.
        portrait = min(token, token_h) * (0.62 if names else 1.12)
        portrait_y = token_h * 0.05 if names else (token_h - portrait) / 2.0
        px, py, pe = ring(blueprint, "TurnPortrait_{}".format(index), body,
                          (token - portrait) / 2.0, portrait_y, portrait,
                          WHITE, size)
        face = TURN_PORTRAITS[index] if index < len(TURN_PORTRAITS) else None
        image(blueprint, "TurnPortrait_{}".format(index), body,
              px, py, pe, pe, size, texture=face,
              tint=WHITE if face else EMPTY_SOCKET)
        if names:
            label(blueprint, "TurnName_{}".format(index), body,
                  4, token_h * 0.05 + portrait + 2, token - 8, 16, "이름", 11,
                  TEXT_DIM, "center", size)
        if framed:
            # 시안은 직선 레일이 나무이고 모서리만 금속 브래킷이다.
            frame(blueprint, name, body, *size, family="wood")
        marker(blueprint, "TurnCurrent_{}".format(index), body,
               size[0], size[1], size)

        # 칸 사이 화살표는 기본으로 끈다.
        #
        # 진행 방향을 알려 주려고 넣었는데 시안에는 없다. 위젯별 대조에서
        # 차이 1~4위를 전부 이 화살표가 차지했다 -- 시안의 같은 자리에는
        # 옆 칸의 초상이 있다. 방향 표시가 필요하다고 판단되면 arrows=True 로
        # 되살리면 된다.
        if arrows and index + 1 < count:
            arrow = min(gap * 1.7, min(token, token_h) * 0.30)
            arrow_name = "TurnArrow_{}".format(index)
            paint(add(blueprint, "Image", arrow_name, root),
                  texture=KK + "/KK_Turn_Arrow", tint=GOLD)
            place(blueprint, arrow_name,
                  x + index * (token + gap) + token + (gap - arrow) / 2.0,
                  y + (token_h - arrow) / 2.0, arrow, arrow, anchor, None,
                  Z_CONTENT)


def party_card(blueprint, root, index, x, y, w, h, anchor="tl", style="card",
               chip_gems=False):
    """One ally read-out.

    `style` picks how much of it is drawn:
      card  -- portrait, name, HP bar and text, four AP gems, status
      strip -- the same on one horizontal line
      chip  -- portrait and an HP bar only, for layouts that keep the HUD
               small; `chip_gems` adds the AP row back for the one layout that
               shows a single unit and nothing else
      hero  -- a large panel for the one unit whose turn it is
    """
    name = "PartyCard_{}".format(index)
    # 차례인 유닛의 카드만 밝게 -- 선택 테두리와 함께 두 겹으로 읽힌다.
    plate, size = card(blueprint, name, root, x, y, w, h, anchor,
                       role="party_lead" if index == 0 else "party")

    # 내용은 전부 이 캔버스 안에 넣는다. 빈 칸일 때 런타임이 이 하나만
    # 감추면 장식까지 같이 사라진다 -- 위젯을 하나씩 감추면 계약에 없는
    # 초상화 테와 꺼진 보석 바탕이 남아 빈 칸에 유령 고리와 유령 보석이 뜬다.
    body = "PartyContent_{}".format(index)
    add(blueprint, "CanvasPanel", body, plate)
    place(blueprint, body, 0, 0, size[0], size[1], "tl", size, Z_CONTENT)
    portrait_art = PARTY_PORTRAITS[index]
    portrait_tint = (unreal.LinearColor(1, 1, 1, 1) if portrait_art
                     else EMPTY_SOCKET)

    def gems(gx, gy, extent, pitch):
        # Each gem gets a dim backing that is never hidden: the widget
        # collapses spent gems, and a gem that just disappears reads as "this
        # unit has fewer slots" instead of "spent".
        for pip in range(4):
            image(blueprint, "PartyAPPipBg_{}_{}".format(index, pip), body,
                  gx + pip * pitch, gy, extent, extent, size,
                  texture=KK + "/KK_Gem_Blue_Off", tint=WHITE)
        for pip in range(4):
            image(blueprint, "PartyAPPip_{}_{}".format(index, pip), body,
                  gx + pip * pitch, gy, extent, extent, size,
                  texture=KK + "/KK_Gem_Blue_On", tint=WHITE)

    if style == "chip":
        px, py, pe = ring(blueprint, "PartyPortrait_{}".format(index), body,
                          6, (h - (h - 12)) / 2.0 + 6, h - 12, WHITE, size)
        image(blueprint, "PartyPortrait_{}".format(index), body,
              px, py, pe, pe, size, texture=portrait_art, tint=portrait_tint)
        left = h + 4
        gem_run = 4 * 22.0 if chip_gems else 0.0
        label(blueprint, "PartyName_{}".format(index), body,
              left, 6, w - left - 8 - gem_run, 20, "이름", 13, TEXT_COLOR,
              "left", size)
        bar(blueprint, "PartyHPBar_{}".format(index), body,
            left, h - 26, w - left - 8, 12, HP_GREEN, size)
        if chip_gems:
            gems(w - gem_run - 8, 6, 20, 22)

    elif style == "strip":
        # 아군 행: 왼쪽에 둥근 초상화, 오른쪽 위에 이름, 그 아래 HP 바와
        # 숫자, 맨 아래 AP 보석 줄. 상태이상은 이름 줄 오른쪽 끝.
        #
        # 자리를 높이의 비율로 잡는다. 고정 오프셋으로 짰더니 통합 바(72px)와
        # 상단 정보 바(150px)에서 줄이 서로 올라타 글자가 겹쳤다. 같은 부품이
        # 배치안마다 다른 높이로 들어가는 게 이 키트의 전제다.
        pad = max(5.0, h * 0.045)
        portrait = h - 2 * pad
        px, py, pe = ring(blueprint, "PartyPortrait_{}".format(index), body,
                          pad, pad, portrait, WHITE, size)
        image(blueprint, "PartyPortrait_{}".format(index), body,
              px, py, pe, pe, size, texture=portrait_art, tint=portrait_tint)

        # 원화의 링 투명 여백 때문에 portrait 위젯 폭을 그대로 더하면
        # 이름·HP·AP가 시안보다 약 12px 오른쪽으로 밀린다.
        left = h * 0.90
        run = w - left - pad * 1.6
        line = h * 0.26
        name_size = int(max(12, min(22, h * 0.17)))
        info_size = int(max(10, min(16, h * 0.12)))

        status_w = min(run * 0.42, 150.0)
        label(blueprint, "PartyName_{}".format(index), body,
              left, h * 0.08, run - status_w, line, "이름", name_size,
              TEXT_COLOR, "left", size, bold=True)
        # 상태이상 아이콘은 글자와 짝이라 런타임이 같이 켜고 끈다. 늘 켜 두면
        # 아무 상태도 없는 카드에 해골이 상시로 붙는다.
        tag(blueprint, "PartyStatusIcon_{}".format(index), body,
            left + run - status_w, h * 0.10, max(14.0, min(22.0, h * 0.15)),
            "Poison", size)
        label(blueprint, "PartyStatus_{}".format(index), body,
              left + run - status_w + max(14.0, min(22.0, h * 0.15)) + 4,
              h * 0.09, status_w - 20, line, "", info_size, GOLD, "left", size)

        # 하트 - 바 - 숫자를 붙여서 한 덩어리로 읽히게 한다. 숫자를 카드
        # 오른쪽 끝에 붙여 두면 바와 사이가 텅 비어 둘이 딴 정보로 보인다.
        # 하트 표는 뺐다. 시안 아군 카드에는 없다(붉은 픽셀 22개 = 사실상
        # 0). 초록 바 자체가 이미 HP로 읽히는데 앞에 붉은 점을 붙이면
        # 색이 하나 늘고 바가 그만큼 짧아진다.
        hp_text = min(run * 0.26, 88.0)
        # 시안 바는 행 폭의 20% 남짓이고 숫자가 바로 옆에 붙는다. 남는 폭을
        # 전부 바에 주면 얇고 긴 선이 되어 이름·숫자와 한 덩어리로 안 읽힌다.
        bar_w = min(run * 0.58, run - hp_text - 14)
        bar(blueprint, "PartyHPBar_{}".format(index), body,
            left, h * 0.45, bar_w, max(14.0, h * 0.13), HP_GREEN, size)
        label(blueprint, "PartyHPText_{}".format(index), body,
              left + bar_w + 12, h * 0.40, hp_text, line,
              "0/0", info_size, HP_TEXT, "left", size)

        # 보석 줄도 같은 이유로 숫자를 바로 옆에 붙인다.
        gem = max(22.0, min(30.0, h * 0.24))
        # 젬 사이를 더 벌린다. 붙여 놓으면 넷이 한 덩어리로 뭉쳐 개수가
        # 안 읽힌다 -- 시안은 젬 하나 폭의 3분의 1을 띄운다.
        # 시안은 보석 줄 윗변이 행 높이의 61%에 온다. 0.70 이면 8%p 낮게
        # 앉아 이름·바와의 간격이 벌어지고 카드 아래가 비어 보인다.
        gems(left, h * 0.61, gem, gem * 1.35)
        label(blueprint, "PartyAPText_{}".format(index), body,
              left + gem * 1.35 * 4 + 10, h * 0.61, 64, line, "0/0", info_size,
              AP_TEXT, "left", size)

    elif style == "hero":
        portrait = h * 0.56
        px, py, pe = ring(blueprint, "PartyPortrait_{}".format(index), body,
                          18, 18, portrait, WHITE, size)
        image(blueprint, "PartyPortrait_{}".format(index), body,
              px, py, pe, pe, size, texture=portrait_art, tint=portrait_tint)
        left = portrait + 34
        label(blueprint, "PartyName_{}".format(index), body,
              left, 22, w - left - 20, 32, "이름", 24, TEXT_COLOR, "left",
              size, bold=True)
        bar(blueprint, "PartyHPBar_{}".format(index), body,
            left, 62, w - left - 20, 20, HP_GREEN, size)
        label(blueprint, "PartyHPText_{}".format(index), body,
              left, 86, w - left - 20, 22, "0/0", 15, TEXT_DIM, "left", size)
        gems(left, 116, 26, 30)
        label(blueprint, "PartyAPText_{}".format(index), body,
              left + 124, 118, 60, 22, "0/0", 14, TEXT_DIM, "left", size)
        label(blueprint, "PartyStatus_{}".format(index), body,
              18, h - 42, w - 36, 24, "", 15, GOLD, "left", size)

    else:  # card
        portrait = min(84.0, h * 0.5)
        px, py, pe = ring(blueprint, "PartyPortrait_{}".format(index), body,
                          14, 14, portrait, WHITE, size)
        image(blueprint, "PartyPortrait_{}".format(index), body,
              px, py, pe, pe, size, texture=portrait_art, tint=portrait_tint)
        left = portrait + 22
        label(blueprint, "PartyName_{}".format(index), body,
              left, 16, w - left - 14, 24, "이름", 16, TEXT_COLOR, "left",
              size, bold=True)
        bar(blueprint, "PartyHPBar_{}".format(index), body,
            left, 46, w - left - 14, 14, HP_GREEN, size)
        label(blueprint, "PartyHPText_{}".format(index), body,
              left, 62, w - left - 14, 18, "0/0", 12, TEXT_DIM, "left", size)
        gems(left, 84, 20, 23)
        label(blueprint, "PartyAPText_{}".format(index), body,
              14, portrait + 20, portrait, 16, "0/0", 11, TEXT_DIM, "center",
              size)
        label(blueprint, "PartyStatus_{}".format(index), body,
              14, h - 40, w - 28, 20, "", 12, GOLD, "left", size)

    # 프레임과 선택 표시는 판에 직접 얹는다. 내용 캔버스에 넣으면 빈 칸에서
    # 테두리까지 같이 사라져 카드가 통째로 없어진 것처럼 보인다.
    frame(blueprint, name, plate, *size, family="wood")
    marker(blueprint, "PartySelected_{}".format(index), plate,
           size[0], size[1], size)


def command_card(blueprint, root, index, x, y, w, h, anchor="tl", style="card",
                 angle=None):
    """One command slot. Slot 0 is 이동, 1..5 are skills.

    `style`:
      card -- painted icon, name, damage, cooldown, cost gem
      icon -- the glyph and the cost only, for minimal layouts
    """
    name = "CommandCard_{}".format(index)
    body, size = card(blueprint, name, root, x, y, w, h, anchor, role="command")

    if angle is not None:
        # A hand of cards needs the fan; canvas slots cannot rotate, so the
        # widget's own render transform does it. The pivot is the bottom edge
        # so the cards splay from a common point rather than spinning in place.
        widget = helper.umg_find_widget(blueprint, name)
        widget.set_render_transform_angle(angle)
        pivot = unreal.Vector2D(0.5, 1.4)
        widget.set_editor_property("render_transform_pivot", pivot)

    if style == "compact":
        # 고리 배치용 납작한 카드. 세로로 긴 카드를 원 둘레에 놓으면 위아래
        # 카드가 상단·하단 띠에 닿는다.
        #
        # 아이콘을 카드 높이만큼 키웠더니 글자 자리가 26px만 남아 이름이 카드
        # 밖으로 흘렀다. 아이콘은 위쪽 절반만 쓰고 글자는 아래에 쌓는다.
        glyph = min(w * 0.34, h * 0.42)
        image(blueprint, "CommandIcon_{}".format(index), body,
              10, 10, glyph, glyph, size,
              texture=COMMAND_ICONS[index], tint=WHITE)

        left = glyph + 18
        label(blueprint, "CommandName_{}".format(index), body,
              left, 12, w - left - 10, 26, "이름", 16, TEXT_COLOR, "left",
              size, bold=True)
        label(blueprint, "CommandCostLine_{}".format(index), body,
              left, 38, w - left - 10, 22, "", 14, TEXT_COLOR, "left", size,
              bold=True)

        label(blueprint, "CommandDamage_{}".format(index), body,
              10, h - 52, w - 20, 22, "0~0", 14, TEXT_DIM, "center", size)
        label(blueprint, "CommandCooldown_{}".format(index), body,
              10, h - 30, w - 20, 22, "", 13, GOLD, "center", size)

        gem = 30.0
        image(blueprint, "CommandCostGem_{}".format(index), body,
              w - gem - 10, 10, gem, gem, size,
              texture=KK + "/KK_Badge_Round", tint=WHITE)
        label(blueprint, "CommandCost_{}".format(index), body,
              w - gem - 10, 15, gem, 22, "0", 15,
              unreal.LinearColor(1.0, 0.97, 0.90, 1.0), "center", size,
              bold=True)

    elif style == "icon":
        glyph = min(w, h) * 0.78
        image(blueprint, "CommandIcon_{}".format(index), body,
              (w - glyph) / 2.0, (h - glyph) / 2.0, glyph, glyph, size,
              texture=COMMAND_ICONS[index], tint=unreal.LinearColor(1, 1, 1, 1))
        gem = 26.0
        image(blueprint, "CommandCostGem_{}".format(index), body,
              w - gem - 10, 10, gem, gem, size,
              texture=KK + "/KK_Badge_Round", tint=WHITE)
        label(blueprint, "CommandCost_{}".format(index), body,
              w - gem - 10, 14, gem, 20, "0", 13,
              unreal.LinearColor(1.0, 0.97, 0.90, 1.0), "center", size,
              bold=True)
        label(blueprint, "CommandCooldown_{}".format(index), body,
              0, h - 30, w, 20, "", 12, GOLD, "center", size)
    else:
        # 아이콘이 카드의 중심이다. 위에 붙여 두고 글자를 아래에 흩으면
        # 가운데가 비어 카드가 세 토막으로 읽힌다.
        #
        # 글자 블록 높이를 먼저 재고, 남는 세로를 아이콘이 갖는다.
        text_block = 24 + 20 + 20 + 26
        # 시안은 아이콘을 카드 면 위에 직접 올린다. 원형 베젤을 한 겹 더
        # 두르면 카드 프레임과 위계가 겹치고 실제 아이콘이 작아진다.
        glyph = min(w * 0.68, h * 0.34)
        icon_top = 28.0
        image(blueprint, "CommandIcon_{}".format(index), body,
              (w - glyph) / 2.0, icon_top, glyph, glyph, size,
              texture=COMMAND_ICONS[index], tint=WHITE)
        # 줄 자리는 시안 실측 비율로 잡는다. 카드 아래에서 역산했더니 네 줄이
        # 12%p 아래로 몰려 AP 가 프레임에 닿았다. 순서도 시안이 옳다 --
        # 이름 다음이 쿨이고 피해가 그 아래다.
        name_y = h * 0.49
        cool_y = h * 0.58
        dmg_y = h * 0.70
        text_top = name_y
        label(blueprint, "CommandName_{}".format(index), body,
              8, name_y, w - 16, 26, "이름", 15, TEXT_COLOR, "center", size)
        tag(blueprint, "CommandCooldownIcon_{}".format(index), body,
            (w - 96) / 2.0, cool_y, 18, "Cooldown", size)
        label(blueprint, "CommandCooldown_{}".format(index), body,
              (w - 96) / 2.0 + 22, cool_y, 78, 20, "", 12,
              COOLDOWN_TEXT, "left", size)
        label(blueprint, "CommandDamage_{}".format(index), body,
              8, dmg_y, w - 16, 20, "0~0", 13, DAMAGE_TEXT, "center", size)
        # 배지는 밝은 원에 진한 숫자다. 반대로 하면 밝은 카드 위에서 숫자가
        # 배지에 먹힌다 -- 대비 방향이 뒤집혀 있었다.
        gem = 40.0
        image(blueprint, "CommandCostGem_{}".format(index), body,
              w - gem - 12, 12, gem, gem, size,
              texture=KK + "/KK_Badge_Round", tint=BADGE_FACE)
        label(blueprint, "CommandCost_{}".format(index), body,
              w - gem - 12, 12 + (gem - 26) / 2.0, gem, 26, "0", 19,
              BADGE_TEXT, "center", size, bold=True)
        # 배지 숫자는 아이콘에 붙어 있어 훑을 때 안 걸린다. 카드 아래에
        # "AP n"을 한 번 더 적는다. 고를 때 보는 건 이쪽이다.
        label(blueprint, "CommandCostLine_{}".format(index), body,
              8, h * 0.89, w - 16, 24, "", 15, AP_TEXT, "center", size,
              bold=True)

    frame(blueprint, name, body, *size, family="metal")

    # The button covers the card and is added after the plate but before the
    # overlays, so the overlays draw on top. Text and images are hit-test
    # invisible, so nothing steals the click.
    ghost_button(blueprint, "CommandButton_{}".format(index), body,
                 0, 0, w, h, size)

    # Unusable skills are pressed down by ink with the material over it.
    #
    # The ink has to be *inside* the widget the runtime hides, because that is
    # the only thing the runtime knows how to toggle. Two earlier shapes both
    # failed quietly: a separate ink image the runtime never knew the name of
    # (so every card stayed equally dark), and ink painted onto the Border's
    # own background (a Border keeps the engine's outline brush, which draws a
    # rule rather than a fill, so the card barely changed).
    disabled_name = "CommandDisabled_{}".format(index)
    disabled = add(blueprint, "Border", disabled_name, body)
    disabled.set_editor_property("padding", unreal.Margin(0.0, 0.0, 0.0, 0.0))
    paint(disabled, tint=unreal.LinearColor(1.0, 1.0, 1.0, 0.0))
    place(blueprint, disabled_name, 0, 0, w, h, "tl", size, Z_OVERLAY)

    stack = "CommandDisabledStack_{}".format(index)
    add(blueprint, "CanvasPanel", stack, disabled_name)
    image(blueprint, "CommandDisabledInk_{}".format(index), stack, 0, 0, w, h,
          size, tint=unreal.LinearColor(0.0, 0.0, 0.0, 0.22))
    image(blueprint, "CommandDisabledMat_{}".format(index), stack, 0, 0, w, h,
          size, texture=KK + "/KK_Veil_Disabled",
          tint=unreal.LinearColor(1.0, 1.0, 1.0, 0.35),
          size=(VEIL_TILE, VEIL_TILE),
          tiling=unreal.SlateBrushTileType.BOTH)

    marker(blueprint, "CommandSelected_{}".format(index), body, w, h, size)


def enemy_panel(blueprint, root, x, y, w, h, anchor="br", style="wide"):
    """The selected enemy read-out."""
    body, size = card(blueprint, "EnemyPanel", root, x, y, w, h, anchor,
                      role="enemy")
    enemy_ring = WHITE

    if style == "tall":
        portrait = min(w - 2 * max(12.0, w * 0.07), h * 0.34)
        # 자리를 높이의 비율로 잡는다.
        #
        # 여기도 고정 오프셋으로 박혀 있었다 -- 넓은 분기에서 이미 같은 이유로
        # 고쳤는데 이쪽이 남아 있었다. 470px 짜리 세로 패널에서 내용이 위쪽
        # 300px 안에만 들어가 아래가 빈 벌판이 됐다.
        #
        # 시안(2안·6안)은 초상이 크고, 숫자마다 아이콘이 짝을 이루며, 예상
        # 피해가 맨 아래에 제목과 함께 온다.
        pad = max(12.0, w * 0.07)
        icon = max(16.0, min(26.0, w * 0.11))
        px, py, pe = ring(blueprint, "EnemyPortrait", body,
                          (w - portrait) / 2.0, h * 0.05, portrait,
                          enemy_ring, size, enemy=True)
        image(blueprint, "EnemyPortrait", body, px, py, pe, pe, size,
              texture=KK + "/KK_Face_Enemy_Eagle_ActionV3",
              tint=unreal.LinearColor(1, 1, 1, 1))
        name_size = int(max(14, min(22, w * 0.09)))
        info_size = int(max(12, min(17, w * 0.075)))
        line = max(22.0, h * 0.055)

        label(blueprint, "EnemyName", body, pad, h * 0.34, w - pad * 2, line,
              "적", name_size, TEXT_COLOR, "center", size, bold=True)

        # 하트-바-숫자를 한 줄로 묶는다. 바만 폭 전체로 늘리면 숫자와 따로 논다.
        run = w - pad * 2
        hp_text = min(run * 0.34, 86.0)
        bar_w = run - icon - hp_text - 12
        tag(blueprint, "EnemyHPIcon", body, pad, h * 0.44, icon, "HP", size)
        bar(blueprint, "EnemyHPBar", body, pad + icon + 6, h * 0.45,
            bar_w, max(12.0, h * 0.035), HP_RED, size)
        label(blueprint, "EnemyHPText", body, pad + icon + bar_w + 12,
              h * 0.435, hp_text, line, "0/0", info_size, TEXT_COLOR,
              "left", size, bold=True)

        tag(blueprint, "EnemyDefenseIcon", body, pad, h * 0.54, icon,
            "Defense", size)
        label(blueprint, "EnemyDefense", body, pad + icon + 8, h * 0.535,
              run - icon - 8, line, "", info_size, TEXT_DIM, "left", size)

        label(blueprint, "EnemyStatus", body, pad, h * 0.64, run, line, "",
              info_size, GOLD, "center", size)

        # 예상 피해는 맨 아래. 제목을 따로 두려다 "예상 피해 / 예상 피해 8~14"
        # 로 두 번 나왔다 -- C++ 이 이미 "예상 피해 8~14" 전체를 넣는다.
        tag(blueprint, "EnemyForecastIcon", body, pad, h * 0.80, icon,
            "Damage", size)
        label(blueprint, "EnemyForecast", body, pad + icon + 8, h * 0.79,
              run - icon - 8, line, "", info_size, DAMAGE_TEXT, "left", size)
    else:
        # 자리를 높이의 비율로 잡는다. 고정 오프셋으로 박아 두고 패널만
        # 키웠더니 위쪽 1/3만 쓰고 아래는 빈 벌판이 됐다. 아군 띠에서 이미
        # 겪은 것과 같은 실수다.
        pad = max(12.0, h * 0.09)
        # 초상을 키웠더니 패널 폭의 44%를 먹어 오른쪽 글자칸이 85px 로
        # 줄었고 "예상 피해 8~14" 가 잘렸다. 시안 초상은 76px(패널 폭의 25%)
        # 이다. 글자가 잘리는 건 초상이 큰 것보다 나쁘다.
        portrait = min(h * 0.50, w * 0.30)
        px, py, pe = ring(blueprint, "EnemyPortrait", body, pad, pad,
                          portrait, enemy_ring, size, enemy=True)
        image(blueprint, "EnemyPortrait", body, px, py, pe, pe, size,
              texture=KK + "/KK_Face_Enemy_Eagle_ActionV3",
              tint=unreal.LinearColor(1, 1, 1, 1))

        left = pad + portrait + pad * 0.8
        run = w - left - pad
        name_size = int(max(13, min(20, h * 0.13)))
        info_size = int(max(11, min(16, h * 0.10)))
        line = h * 0.16

        label(blueprint, "EnemyName", body, left, h * 0.09, run, line, "적",
              name_size, TEXT_COLOR, "left", size, bold=True)

        hp_text = min(run * 0.30, 92.0)
        # 바에 남는 폭을 다 주면 안 된다. 10안처럼 가로로 긴 띠에서는 바가
        # 1100px 로 늘어나 초상·숫자와 한 덩어리로 안 읽힌다. 시안은 어느
        # 배치안에서든 바를 200px 남짓으로 둔다.
        bar_w = min(run - hp_text - 10, max(180.0, run * 0.34))
        bar(blueprint, "EnemyHPBar", body, left, h * 0.32,
            bar_w, max(12.0, h * 0.09), HP_RED, size)
        label(blueprint, "EnemyHPText", body, left + run - hp_text,
              h * 0.26, hp_text, line, "0/0", info_size, TEXT_COLOR, "right",
              size)

        icon = max(16.0, min(22.0, h * 0.11))
        tag(blueprint, "EnemyDefenseIcon", body, left, h * 0.50, icon,
            "Defense", size)
        label(blueprint, "EnemyDefense", body, left + icon + 6, h * 0.49,
              run - icon - 6, line, "", info_size, TEXT_DIM, "left", size)

        tag(blueprint, "EnemyForecastIcon", body, left, h * 0.68, icon,
            "Damage", size)
        label(blueprint, "EnemyForecast", body, left + icon + 6, h * 0.67,
              run - icon - 6, line, "", info_size, DAMAGE_TEXT, "left", size)

        label(blueprint, "EnemyStatus", body, pad, h - line - pad * 0.6,
              w - 2 * pad, line, "", info_size, GOLD, "left", size)

    frame(blueprint, "EnemyPanel", body, *size, family="metal")


def end_turn(blueprint, root, x, y, w=300.0, h=64.0, anchor="br", size=19):
    """The end-turn button: a hit area with its own framed plate inside."""
    ghost_button(blueprint, "EndTurnButton", root, x, y, w, h)
    place(blueprint, "EndTurnButton", x, y, w, h, anchor)

    add(blueprint, "CanvasPanel", "EndTurnCanvas", "EndTurnButton")
    plate, extent = "EndTurnCanvas", (w, h)
    image(blueprint, "EndTurnInk", plate, 0, 0, w, h, extent, tint=INK)
    action_fill, action_tint = SURFACES["action"]
    image(blueprint, "EndTurnSurface", plate, 0, 0, w, h, extent,
          texture="{}/KK_Fill_{}".format(KK, action_fill), tint=action_tint,
          size=(FILL_TILE, FILL_TILE),
          tiling=unreal.SlateBrushTileType.BOTH)
    image(blueprint, "EndTurnShade", plate, 0, 0, w, h, extent,
          texture=SHADE, tint=WHITE, size=SHADE_SIZE)
    text_w = size * 4.0
    label(blueprint, "EndTurnLabel", plate, (w - text_w) / 2.0,
          (h - size - 10) / 2.0, text_w, size + 10, "턴 종료", size,
          TEXT_COLOR, "center", extent, bold=True)
    # 정보 판이 아니라 누르는 것이다. 볼록한 판을 둘러야 형태만 보고
    # 눌러도 되는 것인지 알 수 있다.
    button_frame(blueprint, "EndTurn", plate, *extent)


# ─── verification ─────────────────────────────────────────────────────────────

#: Names the runtime drives that every layout must carry. The rest are
#: optional on purpose -- a layout that drops the enemy panel or the names is
#: making a design point, and the runtime tolerates the gap.
REQUIRED = (
    ["RoundText", "EndTurnButton"]
    + ["PartyCard_{}".format(i) for i in range(1)]
    + ["CommandCard_{}".format(i) for i in range(6)]
    + ["CommandButton_{}".format(i) for i in range(6)]
    + ["CommandCost_{}".format(i) for i in range(6)]
    + ["CommandSelected_{}".format(i) for i in range(6)]
    + ["CommandDisabled_{}".format(i) for i in range(6)]
)

#: Everything the runtime will write to if it is there. Counted, not required,
#: so each layout's report says how much of the model it actually shows.
OPTIONAL = (
    ["ObjectiveText", "EnemyPanel", "EnemyPortrait", "EnemyName", "EnemyHPBar",
     "EnemyHPText", "EnemyDefense", "EnemyStatus", "EnemyForecast"]
    + ["PartyCard_{}".format(i) for i in range(1, 3)]
    + ["PartySelected_{}".format(i) for i in range(3)]
    + ["PartyPortrait_{}".format(i) for i in range(3)]
    + ["PartyName_{}".format(i) for i in range(3)]
    + ["PartyHPBar_{}".format(i) for i in range(3)]
    + ["PartyHPText_{}".format(i) for i in range(3)]
    + ["PartyAPText_{}".format(i) for i in range(3)]
    + ["PartyStatus_{}".format(i) for i in range(3)]
    + ["PartyAPPip_{}_{}".format(i, p) for i in range(3) for p in range(4)]
    + ["CommandIcon_{}".format(i) for i in range(6)]
    + ["CommandName_{}".format(i) for i in range(6)]
    + ["CommandCooldown_{}".format(i) for i in range(6)]
    + ["CommandDamage_{}".format(i) for i in range(6)]
    + ["TurnToken_{}".format(i) for i in range(6)]
    + ["TurnPortrait_{}".format(i) for i in range(6)]
    + ["TurnName_{}".format(i) for i in range(6)]
    + ["TurnCurrent_{}".format(i) for i in range(6)]
)


def verify(asset_name, required=None):
    """Re-load from disk and confirm the contract names survived the save.

    Missing names are tolerated at runtime, which is exactly why they have to
    be checked here -- a typo produces a blank field, not an error.
    """
    full = "{}/{}".format(PACKAGE_PATH, asset_name)
    blueprint = unreal.EditorAssetLibrary.load_asset(full)
    if blueprint is None:
        raise RuntimeError("{} did not save".format(full))
    missing = [n for n in (REQUIRED if required is None else required)
               if helper.umg_find_widget(blueprint, n) is None]
    if missing:
        raise RuntimeError("{}: missing required widgets:\n  {}".format(
            asset_name, "\n  ".join(missing)))
    shown = sum(1 for n in OPTIONAL
                if helper.umg_find_widget(blueprint, n) is not None)
    return shown, len(OPTIONAL)
