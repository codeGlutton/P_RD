"""지금 판에 들어 있는 배치를 그대로 갤러리로.

무엇이 다른가
-------------
``mockups/index.html`` 과 ``variants.html`` 은 **제안**이다. 손으로 적은 좌표로
그린 시안이라, 판을 다시 구워도 안 바뀐다. 실제로 오늘 재설계한 내용이 둘 다
안 들어가 있었다.

이건 **판에서 뽑은 값**으로 그린다. 재료는 WBP 편집기가 언리얼에서 추출해 둔
``data/workspace.json`` 이고, 위젯마다 자리·그림·글자가 들어 있다. 그래서
빌더를 돌린 뒤 이걸 다시 돌리면 화면이 따라온다.

한 칸은 캔버스 하나
-------------------
추출본은 캔버스 단위로 납작하게 펴져 있다(자리는 그 캔버스 기준). 화면 하나가
캔버스 여럿으로 나뉘어 있으면 칸도 여럿이 된다 -- 뿌리 캔버스 칸이 곧 그 화면
전체다. 겹쳐 그리려면 부모-자식 관계가 있어야 하는데 추출본에는 없다.

Run with plain python:
    python Tools/UI/build_current_gallery.py
"""

import html
import json
from pathlib import Path

EDITOR = Path("D:/UnrealProjects_WBP_Editor")
WORKSPACE = EDITOR / "data/workspace.json"
OUT = Path("D:/UnrealProjects/P_RD_develop_20260803/Tools/UI/mockups/current.html")

CARD_W = 470.0        # 한 칸의 가로 픽셀
# 살펴볼 것이 없는 캔버스는 뺀다. 위젯 하나짜리 칸이 백 개 있으면 목록이 아니라
# 소음이다.
MIN_WIDGETS = 2

FLOW = Path("D:/UnrealProjects/P_RD_develop_20260803/Saved/LegacyAudit/screen_flow.json")

# 게임이 지나가는 순서. 왼쪽이 먼저 나오는 화면이다.
#
# 어느 화면인지는 **경로**로 정한다 -- 판이 어느 폴더에 있는지가 그 화면을
# 말한다. 게임모드가 물고 있는 것은 둘뿐이고(타이틀·전투 HUD) 나머지는 C++ 이
# 경로로 부르므로, 레지스트리만으로는 순서를 못 세운다.
STAGE_RULES = [
    ("1. 인트로", ("/Game/UI/Intro",)),
    ("2. 타이틀", ("/Game/UI/WBP_TitleMenu", "/Game/UI/WBP_ClassSelect")),
    # 용병 선택은 타이틀에서 NEW START 를 누르면 바로 나오는 화면인데,
    # 판이 CombatLayouts 폴더에 있어 "전투 - 상세·탭" 에 묻혀 있었다.
    # 폴더가 아니라 **게임이 지나가는 차례**로 묶어야 찾을 수 있다.
    # (DefaultGame.ini 의 mWorldWidgetClasses[10] 이 이것을 문다)
    ("3. 용병 선택", ("/Game/UI/CombatLayouts/WBP_MercenaryHire_Marchbound",)),
    ("4. 지도", ("/Game/UI/WBP_FrontendMap",)),
    ("5. 전투 HUD", ("/Game/UI/CombatLayouts/WBP_CombatHUD04",
                     "/Game/UI/CombatHUD",)),
    ("6. 전투 - 상세·탭", ("/Game/UI/CombatDetail", "/Game/UI/MonsterTab",
                           "/Game/UI/CombatLayouts")),
    ("7. 공용 팝업", ("/Game/UI/WBP_Inventory", "/Game/UI/WBP_SettingsPanel")),
    ("8. 보상", ("/Game/UI/RewardSettlement",)),
    ("9. 패배", ("/Game/UI/CombatResult",)),
    ("10. 상점·보물", ("/Game/UI/Shop", "/Game/UI/Treasure")),
    ("Z. 시안 (안 쓰임)", ("/Game/UI/Concepts",)),
]


def stage_of(asset):
    for label, prefixes in STAGE_RULES:
        if any(asset.startswith(prefix) for prefix in prefixes):
            return label
    return "Y. 분류 안 됨"


def usage():
    """패키지 -> (쓰임 표시, 누가 부르나). 흐름 조사 결과를 읽는다."""
    if not FLOW.is_file():
        return {}
    data = json.loads(FLOW.read_text(encoding="utf-8"))
    marks = {}
    for item in data.get("order", []):
        marks[item["package"]] = ("쓰임", item.get("stage", ""))
    for item in data.get("linked", []):
        who = [c.rsplit("/", 1)[-1] for c in item.get("callers", [])] + item.get("code", [])
        marks[item["package"]] = ("코드가 부름", ", ".join(who[:3]))
    for item in data.get("floating", []):
        marks[item["package"]] = ("아무도 안 부름", "")
    return marks

STYLE = """
/* 게임이 쓰는 글꼴 그대로.
   FontFace 에셋(.uasset) 안에 TTF 가 통째로 박혀 있어 꺼내 뒀다 --
   Tools/UI/carve_fonts.py. 이게 없으면 크기와 정렬만 맞고 글자 모양이 달라
   "이 크기면 되겠다" 를 판단할 수가 없다. */
@font-face { font-family: "F_HUD_Oswald"; font-weight: 700;
             src: url("fonts/Oswald-Bold.ttf") format("truetype"); }
@font-face { font-family: "F_HUD_Oswald"; font-weight: 400;
             src: url("fonts/Oswald-Regular.ttf") format("truetype"); }
@font-face { font-family: "F_HUD_LINESeedKR"; font-weight: 700;
             src: url("fonts/LINESeedKR-Bold.ttf") format("truetype"); }
@font-face { font-family: "F_HUD_LINESeedKR"; font-weight: 400;
             src: url("fonts/LINESeedKR-Regular.ttf") format("truetype"); }
:root { color-scheme: dark; }
* { box-sizing: border-box; margin: 0; }
body { background: #14100c; color: #efe3cd; padding: 22px 26px 60px;
       font: 14px/1.55 "Malgun Gothic", system-ui, sans-serif; }
h1 { font-size: 21px; color: #f0c479; }
.note { color: #a2917a; font-size: 13px; margin: 6px 0 18px; max-width: 900px; }
.bar { position: sticky; top: 0; z-index: 30; background: #14100ceb; padding: 10px 0 12px;
       border-bottom: 1px solid #3a2d1c; margin-bottom: 18px; display: flex;
       gap: 10px; align-items: center; flex-wrap: wrap; }
.bar input { background: #241b12; color: #efe3cd; border: 1px solid #5a462c;
             border-radius: 5px; padding: 5px 9px; width: 250px; }
.bar .count { color: #a2917a; font-size: 12.5px; }
.bar button { background: #3a2d1c; color: #e6d6b8; border: 1px solid #6b5433;
              border-radius: 5px; padding: 4px 11px; cursor: pointer;
              font-size: 12.5px; font-family: inherit; }
.bar button:hover { background: #55401f; color: #ffe9c0; }
.bar button.on { background: #55401f; color: #ffe9c0; border-color: #f0c479; }
h2 { font-size: 15.5px; color: #f0c479; margin: 26px 0 3px; }
h2 .path { color: #7d6e5c; font-weight: 400; font-size: 12px; }
.row { display: flex; gap: 16px; flex-wrap: wrap; }
.card { width: %(cardw)spx; }
.card > .name { font-size: 12.5px; color: #d9c6a4; margin-bottom: 3px;
                overflow: hidden; text-overflow: ellipsis; white-space: nowrap; }
.card > .meta { font-size: 11px; color: #7d6e5c; margin-bottom: 5px; }
.stage { position: relative; overflow: hidden; background: #241b12;
         border: 1px solid #3a2d1c; border-radius: 4px; }
.scale { position: absolute; top: 0; left: 0; transform-origin: 0 0; }
.w { position: absolute; }
.w img { width: 100%%; height: 100%%; display: block; }
/* 그림 없는 것은 무엇인지 알아보게 테두리와 이름만. 색은 종류별로 다르다. */
.w.box { border: 1px solid #ffffff26; }
/* 글자는 맨 위층이라 그림 위에 얹힌다. 밝은 양피지 위에서도 읽히게 테두리를
   깔아 준다 -- 색만 밝게 하면 밝은 바탕에서 사라진다. */
/* 글자칸은 **판에 적힌 대로** 그린다(text_style.json). 다 같은 모양으로
   그리면 "이 크기면 되겠다" 를 판단할 수가 없다. 정렬은 flex 로 낸다 --
   UMG 에서 TextBlock 은 칸 위쪽에 붙으므로 기본은 flex-start 다. */
.w.text { border: 1px dashed #57e08a5c; color: #fff6e2; display: flex;
          align-items: flex-start; justify-content: flex-start;
          overflow: hidden; }
.w.text > span { display: block; }
/* 게임처럼 보기.
   테두리·이름표는 어디에 무엇이 있는지 알려 주는 표시일 뿐 게임에는 없다.
   나란히 놓고 견줄 때는 꺼야 한다. */
body.bare .w.text, body.bare .w.box, body.bare .w.button,
body.bare .w.panel { border: 0; outline: 0; }
body.bare .w .tag { display: none; }
body.bare .w.picked, body.bare .w.swapped, body.bare .w.cleared {
  outline: 0; background: none; }
.w.button { border: 1px solid #ffb03a7a; }
.w.panel { border: 1px solid #6ba7ff40; }
/* 그림에 그어 둔 자리.
   바깥선은 그림이 차지한 자리, 안쪽 점선은 손으로 그어 둔 칸이다(여럿일 수
   있다). 글자칸과 다른 것이 정상이다 -- 글자칸은 세로 가운데를 지키려고 틀
   전체를 쓰고, 이 칸은 글자가 얼마나 커도 되는지만 정한다. */
.guide { position: absolute; pointer-events: none; }
.guide.edge { border: 1px solid #6ce0ff59; }
.guide.hole { border: 1px dashed #6ce0ffc4; }
body.bare .guide { display: none; }
.w .tag { position: absolute; left: 1px; top: 0; font-size: 9px; color: #ffffff7a;
          white-space: nowrap; pointer-events: none; text-shadow: 0 0 3px #000; }
/* 숨겨 둔 위젯. 게임에는 안 나오지만 어디에 무엇이 준비돼 있는지는 보여야
   한다. 옅게 두고, 게임처럼 보기에서는 아예 감춘다. */
.w.off { opacity: .18; }
.w.off .tag { color: #ffffff40; }
body.bare .w.off { display: none; }
.hide { display: none; }
/* 페이지끼리 오가는 줄. 없으면 주소를 손으로 쳐야 한다 -- 실제로 한 번
   에셋 페이지로 넘어갔다가 돌아올 길이 없었다. */
.nav { display: flex; gap: 6px; flex-wrap: wrap; align-items: center;
       margin: 0 0 12px; font-size: 12.5px; }
.nav a { text-decoration: none; padding: 4px 11px; border-radius: 5px;
         background: #241b12; color: #c6b498; border: 1px solid #3a2d1c; }
.nav a:hover { background: #55401f; color: #ffe9c0; }
.nav a.here { background: #55401f; color: #ffe9c0; border-color: #6b5433;
              font-weight: 600; }
h1.stage { font-size: 19px; color: #ffd79a; margin: 40px 0 2px;
           border-bottom: 1px solid #4a3a24; padding-bottom: 6px; }
h1.stage:first-of-type { margin-top: 10px; }
.badge { font-size: 11px; padding: 1px 8px; border-radius: 9px; margin: 0 7px;
         font-weight: 400; vertical-align: 2px; }
.badge.ok { background: #1e3d29; color: #7ee2a4; }
.badge.code { background: #33301a; color: #e0cf7a; }
.badge.dead { background: #3d1f1f; color: #f09090; }
.who { color: #7d6e5c; font-size: 11px; font-weight: 400; margin-right: 7px; }
/* 이름 복사.
   전에는 이름을 끌어 긁어야 했는데, 칸을 누르면 크게 보기가 먼저 열려
   집을 수가 없었다. 단추 하나로 집어 준다. */
.copy { display: inline-block; margin-left: 6px; font-size: 10.5px; padding: 0 6px;
        border-radius: 4px; background: #3a2d1c; color: #c6b498; border: 1px solid #5a462c;
        cursor: pointer; vertical-align: 1px; font-weight: 400; }
.copy:hover { background: #55401f; color: #ffe9c0; }
.copy.done { background: #2f6b3a; color: #cfffdb; border-color: #2f6b3a; }
/* 갈아 끼워 보기 */
/* 패널은 그림 **옆**이지 위가 아니다. 덮으면 정작 볼 것이 안 보인다 --
   그림 자리를 패널만큼 줄여 잡고(fitBig), 접을 수도 있게 둔다.
   머리(고른 것·단추·분류)는 붙박이고 그림 목록만 구른다. 목록이 길어서
   되돌리기 단추가 스크롤 밖으로 밀려나 있었다. */
#tryon { position: absolute; right: 0; top: 0; bottom: 0; width: 296px;
         background: #1b150ff5; border-left: 1px solid #4a3a24; padding: 10px 12px;
         display: none; flex-direction: column; }
#tryon.on { display: flex; }
#tryon h3 { font-size: 13px; color: #f0c479; margin-bottom: 2px; }
#tryon .sub { color: #7d6e5c; font-size: 11px; margin-bottom: 8px; }
#tryon #tryimg { flex: 1; display: flex; flex-direction: column; min-height: 0; }
#tryon #tryimg.off { display: none; }
#tryon #tryopts { flex: 1; overflow: auto; margin-right: -6px; padding-right: 4px; }
#tryon h3 .fold { float: right; font-size: 11px; font-weight: 400; padding: 1px 8px;
                  border-radius: 4px; background: #3a2d1c; color: #c6b498;
                  border: 1px solid #5a462c; cursor: pointer; }
#tryon h3 .fold:hover { background: #55401f; color: #ffe9c0; }
/* 접었을 때 다시 펴는 손잡이 */
#tryTab { position: absolute; right: 0; top: 0; z-index: 5; display: none;
          font-size: 11px; padding: 4px 9px; border-radius: 0 0 0 5px;
          background: #3a2d1cee; color: #e6d6b8; border: 1px solid #6b5433;
          border-right: 0; border-top: 0; cursor: pointer; }
#tryTab.on { display: block; }
#tryTab:hover { background: #55401f; color: #ffe9c0; }
#tryon input { width: 100%%; background: #241b12; color: #efe3cd; border: 1px solid #5a462c;
               border-radius: 4px; padding: 4px 7px; margin-bottom: 8px; font-size: 12px; }
#tryon .opt { display: flex; gap: 7px; align-items: center; padding: 3px; border-radius: 4px;
              cursor: pointer; }
#tryon .opt:hover { background: #2c2216; }
#tryon .opt.on { background: #3f3018; outline: 1px solid #f0c479; }
#tryon .opt img { width: 54px; height: 34px; object-fit: contain; background: #241b12;
                  border: 1px solid #3a2d1c; border-radius: 3px; }
#tryon .opt span { font-size: 11px; color: #d9c6a4; overflow: hidden;
                   text-overflow: ellipsis; white-space: nowrap; }
#tryon .none { color: #f09090; }
/* 되돌리기·없애기는 **단추로** 위에 둔다.
   전에는 목록 맨 위에 한 줄로 끼워 뒀는데, 그림 예순 장 사이에 글자만 있는
   줄이라 아무도 못 봤다. 하는 일이 다르면 자리도 달라야 한다. */
#tryon .acts { display: flex; gap: 5px; flex-wrap: wrap; margin: 6px 0 8px; }
#tryon .acts button { background: #3a2d1c; color: #e6d6b8; border: 1px solid #6b5433;
                      border-radius: 5px; padding: 4px 9px; cursor: pointer; font-size: 11.5px;
                      font-family: inherit; }
#tryon .acts button:hover { background: #55401f; color: #ffe9c0; }
#tryon .acts button:disabled { opacity: .35; cursor: default; background: #2a2016; }
#tryon .acts button.warn { border-color: #7a3a2f; color: #f0b0a0; }
#tryon .acts button.warn:hover { background: #5a2a20; }
/* 글자칸을 고르면 그림 목록 대신 이게 뜬다. */
#trytext { display: none; }
#trytext.on { display: block; }
#trytext .row { display: flex; gap: 6px; align-items: center; margin-bottom: 6px; }
#trytext .row > label { font-size: 11px; color: #a2917a; width: 40px; flex: none; }
#trytext select, #trytext input[type=number] {
    background: #241b12; color: #efe3cd; border: 1px solid #5a462c;
    border-radius: 4px; padding: 3px 6px; font-size: 12px; font-family: inherit; }
#trytext select { flex: 1; }
#trytext input[type=number] { width: 62px; }
#trytext input[type=range] { flex: 1; accent-color: #f0c479; }
#trytext input[type=color] { width: 34px; height: 24px; padding: 0; border: 1px solid #5a462c;
                             background: #241b12; border-radius: 4px; }
#trytext .seg { display: flex; gap: 3px; flex: 1; }
#trytext .seg b { flex: 1; text-align: center; font-size: 11px; font-weight: 400;
                  padding: 3px 0; border-radius: 4px; cursor: pointer;
                  background: #241b12; color: #a2917a; border: 1px solid #3a2d1c; }
#trytext .seg b:hover { background: #33271a; }
#trytext .seg b.on { background: #55401f; color: #ffe9c0; border-color: #6b5433; }
#trytext .was { font-size: 11px; color: #7d6e5c; margin: 8px 0 2px; }
#tryon .tally { font-size: 11px; color: #a2917a; margin-bottom: 6px; }
#tryon .tally b { color: #f0c479; }
/* 그림을 없앤 자리. 빈 것과 "아직 안 건드린 것" 이 같아 보이면 안 된다. */
.w.cleared { outline: 1px dashed #f0909099; outline-offset: -1px;
             background: repeating-linear-gradient(45deg, #ffffff0f 0 5px, transparent 5px 10px); }
/* 분류 거르기. 사람이 assets.html 에서 316개를 손으로 나눠 둔 값을 그대로 쓴다 --
   프레임을 갈아 끼울 때 아이콘 300개를 훑을 이유가 없다. */
#tryon .cats { display: flex; gap: 4px; flex-wrap: wrap; margin-bottom: 7px; }
#tryon .cats b { font-size: 11px; padding: 2px 8px; border-radius: 9px; cursor: pointer;
                 background: #241b12; color: #a2917a; border: 1px solid #3a2d1c;
                 font-weight: 400; }
#tryon .cats b:hover { background: #33271a; }
#tryon .cats b.on { background: #55401f; color: #ffe9c0; border-color: #6b5433; }
.w.picked { outline: 2px solid #57e08a; outline-offset: 1px; }
.w.swapped { outline: 2px dashed #f0c479; outline-offset: 1px; }
.card { cursor: zoom-in; }
.card:hover .stage { border-color: #8a6a3a; }
/* 크게 보기. 칸 안의 것을 그대로 복제해 화면에 맞게 다시 잰다 --
   두 번 그리지 않으므로 작은 것과 큰 것이 어긋날 수 없다. */
#big { position: fixed; inset: 0; z-index: 50; background: #0b0805f2;
       display: none; flex-direction: column; padding: 14px 18px 18px; }
#big.on { display: flex; }
#bigHead { display: flex; gap: 12px; align-items: baseline; flex-wrap: wrap;
           margin-bottom: 10px; }
#bigName { font-size: 17px; color: #f0c479; }
#bigMeta { color: #7d6e5c; font-size: 12px; }
#bigHint { margin-left: auto; color: #7d6e5c; font-size: 12px; }
#bigBody { flex: 1; position: relative; overflow: auto; }
#bigStage { position: absolute; left: 0; top: 0; background: #241b12;
            border: 1px solid #4a3a24; border-radius: 4px; overflow: hidden; }
"""


def kind(widget):
    name = widget.get("className", "")
    if name in ("TextBlock", "RichTextBlock"):
        return "text"
    if name in ("Button", "CheckBox", "Slider"):
        return "button"
    if name in ("CanvasPanel", "Overlay", "HorizontalBox", "VerticalBox",
                "SizeBox", "ScaleBox", "RetainerBox", "Border"):
        return "panel"
    return "box"


# 무엇을 무엇 위에 그릴 것인가.
#
# 판에 적힌 z 만 따르면 **글자가 그림에 덮인다**. UMG 에서는 받침을 먼저 놓고
# 그 위에 글자를 얹는 것이 보통이지만, 늘 그렇지는 않고(받침이 나중에 추가된
# 자리가 많다) 갈아 끼우기로 그림을 물리면 확실히 가려진다.
#
# 여기서는 보는 것이 목적이므로 종류로 층을 나눈다. 그림이 맨 아래, 테두리만
# 있는 것이 중간, **글자가 맨 위**다. 층 안에서는 원래 z 를 지킨다.
LAYERS = {"image": 0, "panel": 100_000, "box": 100_000,
          "button": 100_000, "text": 200_000}

TEXT_STYLE = Path("D:/UnrealProjects/P_RD_develop_20260803/"
                  "Tools/UI/mockups/text_style.json")
WIDGET_SRC = Path("D:/UnrealProjects/P_RD_develop_20260803/"
                  "Tools/UI/mockups/widget_src.json")
STYLES = {}          # 에셋/위젯 -> 글자 모양. export_text_style.py 가 적어 둔다.
SOURCES = {}         # 에셋/위젯 -> 물린 그림. export_widget_source.py 가 적어 둔다.
REGIONS = {}         # 그림 이름 -> 사람이 그어 둔 영역(rects_user.json)
CURRENT_ASSET = [""]
# center_all_text.py 가 글자를 감쌀 때 붙이는 꼬리.
WRAP_SUFFIX = "_Center"


def load_styles():
    if not TEXT_STYLE.is_file():
        return {}
    return json.loads(TEXT_STYLE.read_text(encoding="utf-8")).get("styles", {})


def load_sources():
    if not WIDGET_SRC.is_file():
        return {}
    return json.loads(WIDGET_SRC.read_text(encoding="utf-8"))


def load_regions():
    """사람이 assets.html 에서 그림마다 손으로 그어 둔 자리."""
    path = WIDGET_SRC.parent / "rects_user.json"
    if not path.is_file():
        return {}
    return json.loads(path.read_text(encoding="utf-8"))


def style_of(name):
    return STYLES.get(f"{CURRENT_ASSET[0]}/{name}")


# 글자를 어느 쪽에 붙이나. UMG 는 Justification(좌우)과 슬롯 alignment(둘 다)를
# 따로 두는데, 칸을 정해진 크기로 깔면 세로는 늘 위쪽이다.
FLEX_X = {"LEFT": "flex-start", "CENTER": "center", "RIGHT": "flex-end",
          "FILL": "stretch"}


# pt 를 픽셀로.
#
# 슬레이트는 크기 N 인 글꼴을 N x 96/72 픽셀로 그린다(FontDPI=72). 이걸
# 빼먹고 pt 를 그대로 픽셀로 그렸더니, 갤러리 글자가 게임보다 **4분의 1
# 작게** 나왔다 -- 나란히 놓고 견주는 자리인데 크기가 다르면 소용이 없다.
PT_TO_PX = 96.0 / 72.0


def region_guide(fresh, x, y, box_w, box_h, z):
    """그림의 **바깥 테두리**와 **그어 둔 칸들**을 선으로. 없으면 빈 글.

    9-slice 여백은 여기서 안 본다. 그건 그림을 어떻게 늘일지의 이야기고,
    이 선은 "이 그림 안에서 쓸 수 있는 자리가 어디냐" 만 보여 준다. 칸은
    여럿일 수 있다 -- 3열 틀은 구멍이 셋이다.

    **틀과 형제로 놓는다.** 틀 안에 넣었더니 9-slice 테두리 안쪽이 기준이
    되어(padding box) 자리가 41px 씩 밀리고 높이가 2px 로 찌그러졌다.
    """
    if not fresh or box_w <= 0 or box_h <= 0:
        return ""
    art = (fresh.get("src") or "").rsplit("/", 1)[-1]
    marks = REGIONS.get(art) or []
    if not marks:
        return ""

    # 바깥 테두리 -- 그림이 차지한 자리 그대로.
    out = [f'<div class="guide edge" style="left:{x:.1f}px;top:{y:.1f}px;'
           f'width:{box_w:.1f}px;height:{box_h:.1f}px;z-index:{z}"></div>']
    for mark in marks:
        rect = mark.get("inner") or mark.get("rect") or mark.get("box")
        if not rect or len(rect) != 4:
            continue
        # 비율은 **놓인 자리** 기준이다. 그림이 늘어나면 칸도 같이 늘어난다.
        left = x + rect[0] * box_w
        top = y + rect[1] * box_h
        width = (rect[2] - rect[0]) * box_w
        height = (rect[3] - rect[1]) * box_h
        if width <= 1.0 or height <= 1.0:
            continue
        out.append(f'<div class="guide hole" style="left:{left:.1f}px;'
                   f'top:{top:.1f}px;width:{width:.1f}px;height:{height:.1f}px;'
                   f'z-index:{z}"></div>')
    return "".join(out) if len(out) > 1 else ""


def nine_slice(fresh, art, box_w=0.0, box_h=0.0):
    """Box 브러시면 border-image css. 아니면 빈 글.

    슬레이트가 그리는 테두리 두께는

        테두리픽셀 = 여백비율 x **원본 텍스처 크기**      (ElementBatcher.cpp:857)

    이고 칸이 커져도 안 변한다. css 도 ``border-width`` 를 그 픽셀로 못
    박으면 같은 그림이 된다. ``border-image-slice`` 는 **비율(%)** 로 준다 --
    갤러리가 쓰는 미리보기 png 는 원본보다 작을 수 있어 픽셀로 주면 어긋난다.
    """
    if not fresh or fresh.get("draw") != "BOX":
        return ""
    margin, size = fresh.get("margin"), fresh.get("size")
    if not margin or not size or len(margin) != 4:
        return ""
    left, top, right, bottom = (float(v) for v in margin)
    width, height = float(size[0]), float(size[1])
    if width <= 0 or height <= 0 or max(margin) <= 0:
        return ""

    edge = [top * height, right * width, bottom * height, left * width]
    # 테두리가 칸보다 두꺼우면 **줄여서** 그린다.
    #
    # 슬레이트는 그렇게 한다. css 는 안 그래서, 64px 짜리 칸에 89px 테두리를
    # 주면 칸이 178px 로 늘어나 버렸다(box-sizing 이 있어도 테두리보다 작아질
    # 수는 없다). 같은 비율로 줄여 준다.
    for axis, span in ((0, box_h), (1, box_w)):
        total = edge[axis] + edge[axis + 2]
        if span > 0 and total > span:
            shrink = span / total
            edge[axis] *= shrink
            edge[axis + 2] *= shrink

    return (f";border-style:solid;border-image-repeat:stretch"
            f";border-image-source:url({art})"
            f";border-image-slice:{top * 100:.3f}% {right * 100:.3f}%"
            f" {bottom * 100:.3f}% {left * 100:.3f}% fill"
            f";border-width:{edge[0]:.1f}px {edge[1]:.1f}px"
            f" {edge[2]:.1f}px {edge[3]:.1f}px")


# 글꼴별 줄 높이. 브라우저 기본값(1.15 언저리)과 슬레이트가 쓰는 값이 다르다.
#
# 슬레이트는 글꼴이 말하는 ascent+descent 를 줄 높이로 쓴다. Oswald 는 그것이
# 1.49em 이라, css 기본값으로 그리면 글자가 줄 안에서 다른 자리에 앉는다.
# 위쪽 정렬일 때 그 차이가 그대로 어긋남으로 보인다.
FACE_FILES = {
    ("F_HUD_Oswald", True): "fonts/Oswald-Bold.ttf",
    ("F_HUD_Oswald", False): "fonts/Oswald-Regular.ttf",
    ("F_HUD_LINESeedKR", True): "fonts/LINESeedKR-Bold.ttf",
    ("F_HUD_LINESeedKR", False): "fonts/LINESeedKR-Regular.ttf",
}
_LINE = {}


def line_ratio(font_name, bold):
    key = (font_name, bold)
    if key not in _LINE:
        leaf = FACE_FILES.get(key) or FACE_FILES.get((font_name, not bold))
        ratio = None
        if leaf:
            path = WIDGET_SRC.parent / leaf
            if path.is_file():
                try:
                    from PIL import ImageFont
                    probe = ImageFont.truetype(str(path), 100)
                    ascent, descent = probe.getmetrics()
                    ratio = (ascent + descent) / 100.0
                except Exception:  # noqa: BLE001
                    ratio = None
        _LINE[key] = ratio or 1.2      # 못 재면 브라우저 기본에 가깝게
    return _LINE[key]


def paint(colour, fallback="#000000"):
    """[r, g, b, a] -> css. 못 읽으면 fallback."""
    if not colour or len(colour) < 4:
        return fallback
    return f"rgba({int(colour[0])},{int(colour[1])},{int(colour[2])},{colour[3]})"


def text_css(look):
    """글자칸 하나의 css. 없으면 눈에 띄는 기본값."""
    if look is None:
        return ";font-size:13px;color:#f09090"
    size = float(look.get("size", 20.0)) * PT_TO_PX
    face = look.get("font", "Roboto")
    family = (f"'{face}', 'Malgun Gothic', sans-serif"
              if face.startswith("F_HUD") else "system-ui, sans-serif")
    weight = 700 if str(look.get("typeface", "")).lower().startswith("bold") else 400
    across = FLEX_X.get(str(look.get("just", "LEFT")).upper(), "flex-start")
    # 칸이 auto 면 글자 크기만큼이라 슬롯 alignment 가 곧 정렬이다.
    if look.get("auto"):
        align = look.get("align") or [0.0, 0.0]
        across = FLEX_X.get(
            {0.0: "LEFT", 0.5: "CENTER", 1.0: "RIGHT"}.get(round(float(align[0]), 1)),
            across)
        down = {0.0: "flex-start", 0.5: "center", 1.0: "flex-end"}.get(
            round(float(align[1]), 1), "flex-start")
    elif look.get("valign"):
        # **판에 적힌 대로.** 감쌌으니 가운데겠거니 하고 넘겨짚었더니,
        # 위쪽 정렬로 놓은 자리에서 갤러리만 글자가 다른 데 있었다.
        down = {"V_ALIGN_TOP": "flex-start", "V_ALIGN_CENTER": "center",
                "V_ALIGN_BOTTOM": "flex-end", "V_ALIGN_FILL": "stretch"}.get(
            str(look["valign"]).upper(), "flex-start")
    else:
        down = "flex-start"          # 정해진 높이면 글자는 위쪽에 붙는다
    # 그림자·테두리는 **판에 적힌 것만** 그린다.
    #
    # 전에는 읽으라고 검은 그림자를 임의로 깔았는데, 게임에는 없는 것이라
    # 나란히 놓고 견줄 때 오히려 방해가 됐다.
    marks = []
    shadow = look.get("shadow") or {}
    if (shadow.get("x") or shadow.get("y")) and (shadow.get("color") or [0, 0, 0, 0])[3] > 0:
        # 슬레이트 그림자는 번짐이 없다. 그대로 한 번 밀어 그린다.
        marks.append(f"{shadow['x'] * PT_TO_PX:.1f}px "
                     f"{shadow['y'] * PT_TO_PX:.1f}px 0 {paint(shadow.get('color'))}")
    extra = f";text-shadow:{', '.join(marks)}" if marks else ";text-shadow:none"

    outline = look.get("outline") or {}
    if outline.get("size"):
        extra += (f";-webkit-text-stroke:{outline['size'] * PT_TO_PX:.1f}px "
                  f"{paint(outline.get('color'))};paint-order:stroke fill")

    return (f";font-size:{size:.0f}px;color:{look.get('color', '#ffffff')}"
            f";font-family:{family};font-weight:{weight}"
            f";line-height:{line_ratio(face, weight == 700):.3f}"
            f";justify-content:{across};align-items:{down}{extra}")


def piece(widget, offset, bounds=None, depth=0):
    """위젯 하나를 조각으로. 못 그릴 것이면 None.

    @param bounds 이 위젯이 든 캔버스의 크기. 늘려 놓은(앵커 채우기) 위젯은
                  추출본에 **1920x1080** 으로 적혀 있다 -- 캔버스의 실제 크기가
                  아니라 판의 디자인 크기다. 열 안의 받침이 화면 전체를 덮어
                  보이던 원인이다. 든 칸보다 커질 수 없으므로 잘라 준다.
    @param depth  자식 캔버스로 몇 겹 들어왔나. 안쪽 것이 바깥 것보다 위다 --
                  캔버스마다 z 가 0 부터 다시 세지므로 겹 수를 얹어 줘야 한다.
    """
    rect = widget.get("rect") or {}
    x = float(rect.get("x", 0)) + offset[0]
    y = float(rect.get("y", 0)) + offset[1]
    box_w, box_h = float(rect.get("w", 0)), float(rect.get("h", 0))
    if bounds is not None:
        box_w = min(box_w, max(0.0, bounds[0] - float(rect.get("x", 0))))
        box_h = min(box_h, max(0.0, bounds[1] - float(rect.get("y", 0))))
    if box_w <= 0 or box_h <= 0:
        return None, (x, y), (box_w, box_h)
    style = f"left:{x:.1f}px;top:{y:.1f}px;width:{box_w:.1f}px;height:{box_h:.1f}px"
    # 숨겨 둔 것(Hidden·Collapsed)은 게임에 안 나온다. 확인창처럼 평소엔
    # 숨어 있다가 필요할 때만 뜨는 것들인데, 다 그리니 설정 화면이 겹쳐
    # 뭉갠 것처럼 보였다. 옅게 두고 `게임처럼 보기` 에서는 아예 숨긴다.
    hidden = " off" if widget.get("hidden") else ""
    label = html.escape(widget.get("name", ""))
    image = widget.get("image", "")
    inner = min(999, max(0, int(widget.get("z", 0)))) + min(90, depth) * 1000
    # 이 위젯에 지금 물려 있는 그림. 갈아 끼우기 판에서 이름을 집게 하려고
    # 들고 다닌다 -- 무엇이 붙어 있는지 모르면 무엇으로 바꿀지도 못 정한다.
    #
    # **판에서 방금 읽은 값을 먼저 쓴다.** 추출본의 resourcePath 는 추출한
    # 그 때의 것이라, 그 뒤에 물린 그림은 안 들어 있다 -- 타이틀 단추에
    # 판을 물려 놓고도 "붙어 있는 그림 없음" 으로 나왔다.
    fresh = SOURCES.get(f"{CURRENT_ASSET[0]}/{widget.get('name', '')}") or {}
    source = (fresh.get("src") or widget.get("resourcePath") or "").split(".")[0]
    holds = f' data-src="{html.escape(source)}"' if source else ""
    if image:
        art = f"wbp/{html.escape(Path(image).name)}"
        guide = region_guide(fresh, x, y, box_w, box_h,
                             LAYERS['image'] + inner + 1)
        sliced = nine_slice(fresh, art, box_w, box_h)
        if sliced:
            # 9-slice 는 그림을 통째로 늘이는 게 아니다. 모서리와 변은 원본
            # 픽셀 그대로 두고 가운데만 늘어난다. border-image 가 같은 일을
            # 하므로 그것으로 그린다 -- 전에는 <img> 로 늘여서, 큰 칸일수록
            # 갤러리 테두리가 실제보다 두껍게 보였다.
            return (f'<div class="w{hidden}" data-widget="{label}"{holds}'
                    f' style="{style};z-index:{LAYERS["image"] + inner}{sliced}">'
                    f'<span class="tag">{label}</span></div>{guide}'), (x, y), (box_w, box_h)
        return (f'<div class="w{hidden}" data-widget="{label}"{holds}'
                f' style="{style};z-index:{LAYERS["image"] + inner}">'
                f'<img loading="lazy" src="{art}">'
                f'<span class="tag">{label}</span></div>{guide}'), (x, y), (box_w, box_h)
    # 글자를 감싼 Overlay 는 **글자칸으로 그린다.**
    #
    # 세로 가운데를 주려고 글자를 Overlay 안에 넣었더니, 추출기가 캔버스
    # 직계만 담는 탓에 글자칸 339개가 통째로 빠졌다. 남은 것은 이름이
    # `X_Center` 인 빈 Overlay 뿐이다. 그 이름에서 원래 글자칸을 되찾아
    # 판에서 뽑아 둔 글·글꼴·크기로 그린다.
    raw_name = widget.get("name", "")
    # 추출기가 이제 감싼 알맹이를 겉으로 올려 준다(holds). 옛 추출본에도
    # 맞게 이름 꼬리로도 찾아 둔다.
    inside = widget.get("holds") or (
        raw_name[:-len(WRAP_SUFFIX)] if raw_name.endswith(WRAP_SUFFIX) else "")
    if inside and widget.get("className") in ("TextBlock", "RichTextBlock",
                                              "Overlay"):
        look = style_of(inside)
        if look is not None:
            label = html.escape(inside)
            body = (f'<span>{html.escape((look.get("text") or "").strip())}</span>'
                    if look.get("text") else "")
            return (f'<div class="w text{hidden}" data-widget="{label}"'
                    f' style="{style};z-index:{LAYERS["text"] + inner}'
                    f'{text_css(look)}">{body}'
                    f'<span class="tag">{label}</span></div>'), (x, y), (box_w, box_h)

    text = (widget.get("text") or "").strip()
    css = kind(widget)
    extra, body = "", ""
    if css == "text":
        # 판에 적힌 글꼴·크기·색·정렬 그대로. 못 찾으면 눈에 띄게 둔다.
        look = style_of(widget.get("name", ""))
        extra = text_css(look)
        body = f'<span>{html.escape(text)}</span>' if text else ""
    return (f'<div class="w {css}{hidden}" data-widget="{label}"{holds}'
            f' style="{style};z-index:{LAYERS[css] + inner}{extra}">{body}'
            f'<span class="tag">{label}</span></div>'), (x, y), (box_w, box_h)


def render(document, siblings, out, depth, offset=(0.0, 0.0), seen=None,
           bounds=None):
    """캔버스 하나를 그리고, **자식 캔버스가 따로 있으면 그 안까지 이어 그린다.**

    추출본은 캔버스마다 문서를 따로 만들고 자리도 그 캔버스 기준으로 적는다.
    그래서 뿌리 캔버스만 그리면 열이 빈 상자로만 보였다 -- 열 안의 글자와
    그림은 다른 문서에 들어 있다. 자식 위젯 이름이 같은 에셋의 다른 캔버스
    이름과 맞으면, 그 문서를 자식 자리만큼 밀어서 이어 그린다.
    """
    seen = seen or set()
    name = document.get("canvasName", "")
    if name in seen or depth > 6:
        return
    seen = seen | {name}
    for widget in sorted(document.get("widgets", []), key=lambda w: w.get("z", 0)):
        html_piece, at, size = piece(widget, offset, bounds, depth)
        if html_piece:
            out.append(html_piece)
        child = siblings.get(widget.get("name", ""))
        if child is not None:
            render(child, siblings, out, depth + 1, at, seen, size)


def main():
    data = json.loads(WORKSPACE.read_text(encoding="utf-8"))
    documents = data.get("documents", [])
    STYLES.update(load_styles())
    SOURCES.update(load_sources())
    REGIONS.update(load_regions())

    marks = usage()
    # 에셋 -> 캔버스 이름 -> 문서. 자식 캔버스를 이어 그릴 때 쓴다.
    by_canvas = {}
    for document in documents:
        if document.get("sourceKind") == "current-develop-wbp":
            by_canvas.setdefault(document.get("assetPath", "?"), {})[
                document.get("canvasName", "")] = document

    by_asset = {}
    for document in documents:
        if document.get("sourceKind") != "current-develop-wbp":
            continue
        if len(document.get("widgets", [])) < MIN_WIDGETS:
            continue
        by_asset.setdefault(document.get("assetPath", "?"), []).append(document)

    cards, shown = [], 0
    ordered = sorted(by_asset, key=lambda a: (stage_of(a), a))
    last_stage = None
    for asset in ordered:
        stage = stage_of(asset)
        if stage != last_stage:
            cards.append(f'<h1 class="stage">{html.escape(stage)}</h1>')
            last_stage = stage
        CURRENT_ASSET[0] = asset
        canvases = sorted(by_asset[asset],
                          key=lambda d: -len(d.get("widgets", [])))
        leaf = asset.rsplit("/", 1)[-1]
        mark, who = marks.get(asset, ("?", ""))
        css = {"쓰임": "ok", "코드가 부름": "code",
               "아무도 안 부름": "dead"}.get(mark, "")
        badge = (f'<span class="badge {css}">{html.escape(mark)}</span>'
                 + (f'<span class="who">{html.escape(who)}</span>' if who else ""))
        cards.append(f'<h2>{html.escape(leaf)}'
                     f'<b class="copy" data-copy="{html.escape(asset)}">경로 복사</b> {badge}'
                     f'<span class="path">{html.escape(asset)}</span></h2><div class="row">')
        for document in canvases:
            size = document.get("designSize") or [1920, 1080]
            width = float(size[0]) or 1920.0
            height = float(size[1]) or 1080.0
            scale = CARD_W / width
            widgets = document.get("widgets", [])

            pieces = []
            render(document, by_canvas.get(asset, {}), pieces, 0)

            shown += 1
            cards.append(
                f'<div class="card" data-key="{html.escape((asset + " " + document.get("canvasName", "")).lower())}"'
                f' data-w="{width:.0f}" data-h="{height:.0f}"'
                f' data-title="{html.escape(document.get("canvasName", "?"))}"'
                f' data-asset="{html.escape(asset)}" title="눌러서 크게 보기">'
                f'<div class="name">{html.escape(document.get("canvasName", "?"))}'
                f'<b class="copy" data-copy="{html.escape(document.get("canvasName", "?"))}">복사</b></div>'
                f'<div class="meta">{len(widgets)}개 · {width:.0f}x{height:.0f}</div>'
                f'<div class="stage" style="width:{CARD_W:.0f}px;height:{height * scale:.0f}px">'
                f'<div class="scale" style="width:{width:.0f}px;height:{height:.0f}px;'
                f'transform:scale({scale:.6f})">' + "".join(pieces) + "</div></div></div>")
        cards.append("</div>")

    styles = STYLE % {'cardw': f'{CARD_W:.0f}'}
    page = f"""<!doctype html><html lang="ko"><head><meta charset="utf-8">
<title>지금 배치 — 현재 develop</title><style>{styles}</style></head><body>
<div class="nav"><a href="current.html" class="here">지금 배치</a><a href="assets.html">에셋 · 영역</a><a href="index.html">시안 목록</a><a href="variants.html">시안 갤러리</a><a href="gen.html">만든 것</a></div>
<h1>지금 배치 — 현재 develop</h1>
<p class="note">판에서 뽑은 값 그대로다. 시안이 아니라 <b>지금 게임에 들어 있는 자리</b>다.
한 칸은 캔버스 하나이고, 자리는 그 캔버스 기준이다. 그림이 있는 위젯은 그림으로,
없는 것은 테두리와 이름으로 그린다 &mdash;
<span style="color:#57e08a">초록 점선</span> 글자 ·
<span style="color:#ffb03a">주황</span> 누르는 것 ·
<span style="color:#6ba7ff">파랑</span> 담는 것.</p>
<div class="bar"><input id="q" placeholder="에셋·캔버스 이름으로 거르기">
<span class="count" id="count">{shown}칸</span>
<button id="bare" title="테두리와 이름표를 숨겨 게임과 같은 모습으로 본다">게임처럼 보기</button></div>
<div id="big"><div id="bigHead"><span id="bigName"></span><span id="bigMeta"></span>
<span id="bigHint">위젯을 누르면 갈아 끼우기 · ← → 옆 칸 · Esc 닫기</span></div>
<div id="bigBody"><div id="bigStage"></div>
<b id="tryTab">갈아 끼우기 ▸</b>
<div id="tryon"><h3>갈아 끼워 보기<b class="fold">접기 ▸</b></h3>
<div class="sub">위젯을 누르면 여기서 그림을 고른다. <b>브라우저에서만</b> 바뀐다 &mdash;
판은 안 건드린다. 9-slice 는 흉내라 테두리 두께는 실제와 다를 수 있다.</div>
<div class="tally" id="trytally"></div>
<div class="acts" id="tryacts">
<button data-act="undo">되돌리기</button>
<button data-act="none">그림 없애기</button>
</div>
<div id="trytext">
<div class="row"><label>글꼴</label><select id="txFont"></select></div>
<div class="row"><label>크기</label><input type="range" id="txSizeR" min="8" max="72" step="1">
<input type="number" id="txSize" min="8" max="200" step="1"></div>
<div class="row"><label>색</label><input type="color" id="txColor">
<span id="txColorHex" style="font-size:11px;color:#7d6e5c"></span></div>
<div class="row"><label>좌우</label><div class="seg" id="txH">
<b data-v="LEFT">왼쪽</b><b data-v="CENTER">가운데</b><b data-v="RIGHT">오른쪽</b></div></div>
<div class="row"><label>세로</label><div class="seg" id="txV">
<b data-v="TOP">위</b><b data-v="CENTER">가운데</b><b data-v="BOTTOM">아래</b></div></div>
<div class="row"><label>테두리</label><input type="range" id="txOutR" min="0" max="8" step="1">
<input type="number" id="txOut" min="0" max="16" step="1">
<input type="color" id="txOutC"><input type="range" id="txOutA" min="0" max="100" step="5"
 title="테두리 진하기 — 옅게 두면 은은한 띠가 된다"></div>
<div class="row"><label>그림자</label>
<input type="number" id="txShX" min="-8" max="8" step="1" title="가로로 민 만큼">
<input type="number" id="txShY" min="-8" max="8" step="1" title="세로로 민 만큼">
<input type="color" id="txShC"><input type="range" id="txShA" min="0" max="100" step="5"
 title="진하기"></div>
<div class="row"><label></label><label style="width:auto;cursor:pointer">
<input type="checkbox" id="txOutDrop"> 테두리를 그림자에도 (그림자가 두꺼워져 은은해진다)</label></div>
<div class="row"><label>미리</label><div class="seg" id="txPreset">
<b data-p="none">없음</b><b data-p="soft">은은하게</b><b data-p="hard">또렷하게</b></div></div>
<div class="was" id="txWas"></div></div>
<div id="tryimg">
<div class="cats" id="trycats"></div>
<input id="tryq" placeholder="에셋 이름으로 거르기 (예: Button, HireRow)">
<div id="tryopts"></div></div></div></div></div>
{''.join(cards)}
<script>
const q = document.getElementById('q'), count = document.getElementById('count');
q.addEventListener('input', () => {{
  const term = q.value.trim().toLowerCase();
  let left = 0;
  for (const card of document.querySelectorAll('.card')) {{
    const hit = !term || card.dataset.key.includes(term);
    card.classList.toggle('hide', !hit);
    if (hit) left++;
  }}
  // 칸이 하나도 안 남은 묶음은 제목도 숨긴다.
  for (const row of document.querySelectorAll('.row')) {{
    const empty = row.querySelectorAll('.card:not(.hide)').length === 0;
    row.classList.toggle('hide', empty);
    if (row.previousElementSibling) row.previousElementSibling.classList.toggle('hide', empty);
  }}
  count.textContent = left + '칸';
}});

/* 게임처럼 보기. 표시용 테두리와 이름표만 감춘다 -- 그림·글자·자리는 그대로다. */
document.getElementById('bare').addEventListener('click', event => {{
  // 큰 창이 열려 있어도 안 닫히게. 문서 처리기는 이걸 "그림 밖" 으로 본다.
  event.stopPropagation();
  const on = document.body.classList.toggle('bare');
  event.currentTarget.classList.toggle('on', on);
}});
// 큰 창 안에서도 켜고 끌 수 있게. 손이 자판에 있을 때가 많다.
document.addEventListener('keydown', event => {{
  if (event.key !== 'b' && event.key !== 'ㅠ') return;
  if (document.activeElement && /INPUT|SELECT/.test(document.activeElement.tagName)) return;
  document.getElementById('bare').click();
}});

/* ── 크게 보기 ──────────────────────────────────────────────────────
   칸을 누르면 그 칸의 내용을 그대로 복제해 화면 크기에 맞춰 다시 잰다.
   따로 그리지 않으므로 작은 것과 큰 것이 어긋날 일이 없다. */
const big = document.getElementById('big');
const bigStage = document.getElementById('bigStage');
const bigName = document.getElementById('bigName');
const bigMeta = document.getElementById('bigMeta');
let openCard = null;

function shownCards() {{
  return [...document.querySelectorAll('.card')].filter(c => !c.classList.contains('hide'));
}}

function openBig(card) {{
  openCard = card;
  // **먼저 켠다.** 꺼져 있는 동안에는 크기가 0 이라 배율이 0 이 나온다.
  big.classList.add('on');
  bigStage.innerHTML = '';
  bigStage.appendChild(card.querySelector('.scale').cloneNode(true));
  // 칸에서 그대로 복제하므로 갈아 끼운 것은 안 따라온다. 저장해 둔 것을
  // 다시 입힌다 -- 이게 없으면 닫았다 열 때마다 되돌아가서, 저장이 안 되는
  // 것처럼 보인다.
  applySaved();
  fitBig();
  updateTally();
}}

/* 그림을 남는 자리에 맞춘다.
   패널이 열려 있으면 그만큼 좁게 잡는다 -- 전에는 화면 폭 전체로 재서
   패널이 그림 오른쪽을 덮어 버렸다. 패널을 여닫을 때마다 다시 부른다. */
function fitBig() {{
  if (!openCard) return;
  const w = Number(openCard.dataset.w), h = Number(openCard.dataset.h);
  const body = document.getElementById('bigBody');
  const panel = document.getElementById('tryon');
  const taken = panel.classList.contains('on') ? panel.offsetWidth + 14 : 0;
  const room = Math.max(160, body.clientWidth - taken);
  // 남는 자리에 맞추되 2배까지만. 작은 캔버스를 4배로 늘리면 그림이 뭉개진다.
  const scale = Math.min(room / w, body.clientHeight / h, 2);
  bigName.textContent = openCard.dataset.title;
  bigMeta.textContent = openCard.dataset.asset + '  ·  ' + w + 'x' + h
    + '  ·  ' + (scale * 100).toFixed(0) + '%';
  bigStage.style.width = (w * scale) + 'px';
  bigStage.style.height = (h * scale) + 'px';
  const scaled = bigStage.querySelector('.scale');
  if (scaled) scaled.style.transform = 'scale(' + scale + ')';
}}

function showPanel(on) {{
  document.getElementById('tryon').classList.toggle('on', on);
  document.getElementById('tryTab').classList.toggle('on', !on && !!picked);
  fitBig();
}}

document.getElementById('tryTab').addEventListener('click', event => {{
  event.stopPropagation(); showPanel(true);
}});

function stepBig(delta) {{
  const list = shownCards();
  const at = list.indexOf(openCard);
  if (at < 0) return;
  openBig(list[(at + delta + list.length) % list.length]);
}}

/* 이름 복사.
   칸을 누르면 크게 보기가 열리므로, 복사 단추는 그 전에 가로채야 한다. */
document.addEventListener('click', event => {{
  const copy = event.target.closest('.copy');
  if (copy) {{
    event.stopPropagation();
    navigator.clipboard.writeText(copy.dataset.copy);
    const before = copy.textContent;
    copy.textContent = '복사됨'; copy.classList.add('done');
    setTimeout(() => {{ copy.textContent = before; copy.classList.remove('done'); }}, 900);
    return;
  }}

  if (big.classList.contains('on')) {{
    // 크게 보기 안에서는 위젯을 골라 갈아 끼운다.
    //
    // 어디를 눌렀는지는 **composedPath 로** 본다. closest() 는 지금 DOM 을
    // 거슬러 올라가므로, 처리기가 그 사이 화면을 다시 그렸으면 누른 것이
    // 떨어져 나가 아무 데도 안 속한 것이 된다. path 는 던질 때 찍힌 것이라
    // 다시 그려도 그대로다.
    const path = event.composedPath ? event.composedPath() : [event.target];
    const within = sel => path.some(n => n.matches && n.matches(sel));
    if (within('#tryon')) return;
    const widget = event.target.closest('#bigStage .w');
    if (widget) {{ pickWidget(widget); return; }}
    // 그림 **밖**을 눌렀을 때만 닫는다.
    if (!within('#bigStage') && !within('#tryTab')) {{
      big.classList.remove('on');
      document.getElementById('tryon').classList.remove('on');
      document.getElementById('tryTab').classList.remove('on');
      picked = null;
    }}
    return;
  }}
  const card = event.target.closest('.card');
  if (card) openBig(card);
}});

/* ── 갈아 끼워 보기 ────────────────────────────────────────────────
   판은 안 건드린다. 브라우저에서 그림만 바꿔 보고 "이게 낫나" 를 눈으로
   판단하는 자리다. 마음에 드는 조합은 저장해 두었다가 실제로 물린다. */
let ASSETS = [];
let picked = null;
let swaps = {{}};

let CATS = {{}};
let catFilter = '';

async function loadAssets() {{
  try {{
    const list = await (await fetch('assets.json?t=' + Date.now())).json();
    // slice 도 챙긴다. 갈아 끼운 그림도 9-slice 로 그려야 실제와 같다.
    ASSETS = list.filter(a => a.png).map(a => ({{name: a.name, png: 'assets/' + a.png,
                                                size: a.size, slice: a.slice}}));
  }} catch (e) {{ ASSETS = []; }}
  // 사람이 목록 페이지에서 나눠 둔 분류. 없으면 거르기 줄이 안 뜬다.
  try {{ CATS = await (await fetch('cats_user.json?t=' + Date.now())).json(); }}
  catch (e) {{ CATS = {{}}; }}
  // 판에 적힌 글자 모양. export_text_style.py 가 뽑아 둔다.
  try {{
    const book = await (await fetch('text_style.json?t=' + Date.now())).json();
    TEXTS = book.styles || {{}}; FONTS = book.fonts || ['Roboto'];
  }} catch (e) {{ TEXTS = {{}}; FONTS = ['Roboto']; }}
  try {{ swaps = await (await fetch('tryon.json?t=' + Date.now())).json(); }}
  catch (e) {{ swaps = {{}}; }}
  if (!swaps || typeof swaps !== 'object') swaps = {{}};
  drawCats();
  // 그림 이름을 이제야 알았으므로, 이미 열려 있으면 다시 입혀 준다.
  if (big.classList.contains('on')) {{ applySaved(); }}
  updateTally();
}}

function drawCats() {{
  const counts = {{}};
  for (const a of ASSETS) {{
    const c = CATS[a.name] || '미분류';
    counts[c] = (counts[c] || 0) + 1;
  }}
  const order = ['프레임', '아이콘', '초상화', '미분류'];
  const names = order.filter(c => counts[c]).concat(
    Object.keys(counts).filter(c => !order.includes(c)));
  document.getElementById('trycats').innerHTML =
    '<b data-cat="" class="' + (catFilter === '' ? 'on' : '') + '">전체 '
    + ASSETS.length + '</b>'
    + names.map(c => '<b data-cat="' + c + '" class="' + (catFilter === c ? 'on' : '') + '">'
        + c + ' ' + counts[c] + '</b>').join('');
}}

document.getElementById('trycats').addEventListener('click', event => {{
  const chip = event.target.closest('b[data-cat]');
  if (!chip) return;
  // **여기서 끊어야 한다.** 아래 drawCats() 가 칩을 통째로 다시 그리므로
  // 누른 칩은 DOM 에서 떨어져 나간다. 그 뒤에 문서 처리기가 돌면
  // closest('#tryon') 이 null 이라 "그림 밖을 눌렀다" 로 보고 크게 보기를
  // 닫아 버렸다 -- 분류를 고를 때마다 갤러리로 튕기던 원인이다.
  event.stopPropagation();
  catFilter = chip.dataset.cat;
  drawCats(); drawOptions();
}});

function pickWidget(widget) {{
  document.querySelectorAll('#bigStage .w.picked').forEach(w => w.classList.remove('picked'));
  widget.classList.add('picked');
  picked = widget;
  drawText(); drawOptions(); updateTally(); showPanel(true);
}}

function drawOptions() {{
  const box = document.getElementById('tryopts');
  if (!picked) {{ box.innerHTML = ''; return; }}
  const name = picked.dataset.widget || '(이름 없음)';
  const term = document.getElementById('tryq').value.trim().toLowerCase();
  const hits = ASSETS.filter(a =>
    (!term || a.name.toLowerCase().includes(term))
    && (!catFilter || (CATS[a.name] || '미분류') === catFilter)).slice(0, 60);
  // 지금 이 위젯에 붙어 있는 그림. 갈아 끼운 것이 있으면 그것을, 없으면
  // 판에 물려 있는 것을 보여 준다. 이름은 눌러서 집게 한다.
  const swapped = swaps[keyOf(picked)];
  const held = picked.dataset.src || '';
  const heldName = held.split('/').pop();
  const nowName = swapped === undefined ? heldName
                : (swapped === NONE ? '' : swapped);
  let line = '';
  if (swapped === NONE) {{
    line = '<br>지금: <b style="color:#f09090">그림 없앰</b>';
  }} else if (nowName) {{
    line = '<br>지금: <b style="color:#f0c479">' + nowName + '</b>'
      + ' <b class="copy" data-copy="' + nowName + '">복사</b>'
      + (swapped === undefined && held
         ? ' <b class="copy" data-copy="' + held + '">경로</b>' : '')
      + (swapped !== undefined ? ' <span style="color:#7d6e5c">(갈아 끼운 것)</span>' : '');
  }} else {{
    // 그림이 없다고 끝내면 안 된다. 단추처럼 **누르는 자리만 맡고** 그림은
    // 밑에 깔린 것이 드는 경우가 있다(StartButton 밑의 StartButtonFrameImage).
    // 같은 자리에 겹친 것 중 그림을 든 것을 찾아 알려 준다.
    const under = holder(picked);
    line = under
      ? '<br>지금: <span style="color:#7d6e5c">이 위젯엔 없음 —</span> '
        + '<b style="color:#f0c479">' + (under.dataset.src || '').split('/').pop()
        + '</b> <span style="color:#7d6e5c">를 밑의</span> '
        + '<b class="copy" data-copy="' + under.dataset.widget + '">'
        + under.dataset.widget + '</b> <span style="color:#7d6e5c">가 듦</span>'
      : '<br>지금: <span style="color:#7d6e5c">붙어 있는 그림 없음</span>';
  }}
  const now = swapped;
  box.innerHTML = '<div class="sub">고른 위젯: <b style="color:#57e08a">' + name + '</b>'
    + ' <b class="copy" data-copy="' + name + '">복사</b>' + line + '</div>'
    + hits.map(a => '<div class="opt' + (a.name === now ? ' on' : '') + '"'
        + ' data-png="' + a.png + '" data-name="' + a.name + '">'
        + '<img loading="lazy" src="' + a.png + '">'
        + '<span>' + a.name + '<br><small style="color:#7d6e5c">'
        + a.size[0] + 'x' + a.size[1] + '</small></span></div>').join('');
}}

/* ── 물리기 · 되돌리기 · 없애기 ─────────────────────────────────────
   셋 다 위젯 하나에 대한 일이라 한 곳에 모은다. 원래 모습은 처음 건드릴 때
   data-orig 에 넣어 두고, 되돌릴 때 그걸 도로 넣는다 -- 갤러리를 다시 그릴
   필요가 없다. */
const NONE = '__none__';      // "그림 없앰" 을 저장에 적는 말

function keyOf(widget) {{
  return (openCard ? openCard.dataset.asset + '/' : '') + widget.dataset.widget;
}}

function keepOriginal(widget) {{
  if (widget.dataset.orig === undefined) {{
    widget.dataset.orig = widget.innerHTML;
    // 글자 모양은 inline style 에 있으므로 그것도 같이 챙긴다.
    widget.dataset.origcss = widget.getAttribute('style') || '';
  }}
  const tag = widget.querySelector('.tag');
  return tag ? tag.outerHTML : '';
}}

function putImage(widget, png, entry) {{
  const tag = keepOriginal(widget);
  // 9-slice 로 쓸 그림이면 늘이지 않고 테두리로 그린다. 모서리 두께는
  // **원본 픽셀 그대로**여야 실제와 같다 -- 통째로 늘이면 큰 칸에서
  // 테두리가 실제보다 두껍게 보인다.
  const s = entry && entry.slice, size = entry && entry.size;
  widget.style.borderImageSource = '';
  widget.style.borderWidth = '';
  widget.style.borderStyle = '';
  // 칸 크기는 inline style 에 적혀 있다(자리 계산이 여기 기준이다).
  const boxW = parseFloat(widget.style.width) || 0;
  const boxH = parseFloat(widget.style.height) || 0;
  let wide = s && size ? s[0] * size[0] : 0;
  let tall = s && size ? s[1] * size[1] : 0;
  // 테두리가 칸의 80% 를 넘으면 9-slice 를 못 쓴다. apply_tryon 도 그때
  // 통짜로 늘리므로 미리보기도 같아야 한다.
  const fits = wide * 2 <= boxW * 0.8 && tall * 2 <= boxH * 0.8;
  if (s && size && (wide > 0 || tall > 0) && fits) {{
    widget.innerHTML = tag;
    widget.style.borderStyle = 'solid';
    widget.style.borderImageRepeat = 'stretch';
    widget.style.borderImageSource = 'url(' + png + ')';
    widget.style.borderImageSlice = (s[1]*100).toFixed(3) + '% ' + (s[0]*100).toFixed(3)
      + '% ' + (s[1]*100).toFixed(3) + '% ' + (s[0]*100).toFixed(3) + '% fill';
    widget.style.borderWidth = tall.toFixed(1) + 'px ' + wide.toFixed(1)
      + 'px ' + tall.toFixed(1) + 'px ' + wide.toFixed(1) + 'px';
  }} else {{
    widget.innerHTML = '<img src="' + png + '" style="width:100%;height:100%">' + tag;
  }}
  widget.classList.add('swapped'); widget.classList.remove('cleared');
}}

function putNothing(widget) {{
  // 그림을 빼고 자리만 남긴다. 뒤에 뭐가 깔려 있는지 볼 때 쓴다.
  //
  // **두 군데를 다 지워야 한다.** 9-slice 로 그리는 것은 그림이 `<img>` 가
  // 아니라 테두리(border-image)에 들어 있어서, innerHTML 만 비우면 그림이
  // 그대로 남는다 -- "그림 없애기가 안 먹는다" 던 것이 이것이다.
  const tag = keepOriginal(widget);
  widget.innerHTML = tag;
  widget.style.borderImageSource = 'none';
  widget.style.borderStyle = 'none';
  widget.style.borderWidth = '0';
  widget.classList.add('cleared'); widget.classList.remove('swapped');
}}

function putBack(widget) {{
  if (widget.dataset.orig === undefined) return;
  widget.innerHTML = widget.dataset.orig;
  if (widget.dataset.origcss !== undefined) widget.setAttribute('style', widget.dataset.origcss);
  widget.classList.remove('swapped', 'cleared');
}}

/* ── 글자 모양 바꿔 보기 ────────────────────────────────────────────
   판에서 뽑아 둔 지금 값(text_style.json)이 시작점이고, 바꾼 것은 그림과
   같은 자리에 저장한다 -- 값이 글이면 그림, 덩어리면 글자 설정이다. */
const FLEX_X = {{LEFT: 'flex-start', CENTER: 'center', RIGHT: 'flex-end', FILL: 'stretch'}};
const FLEX_Y = {{TOP: 'flex-start', CENTER: 'center', BOTTOM: 'flex-end'}};
let TEXTS = {{}}, FONTS = [];

function isText(widget) {{ return widget && widget.classList.contains('text'); }}

function hex2(v) {{ return Math.max(0, Math.min(255, Math.round(v))).toString(16).padStart(2, '0'); }}

function baseLook(widget) {{
  const was = TEXTS[keyOf(widget)];
  const look = was ? Object.assign({{}}, was) : {{
    font: 'Roboto', typeface: 'Bold', size: 20, color: '#ffffff',
    just: 'LEFT', vert: 'TOP'}};
  // 판에서 뽑은 그림자·테두리는 [r,g,b,a] 꼴이다. 고르개가 쓰는 꼴로 편다.
  const sh = look.shadow || {{}};
  const shc = sh.color || [0, 0, 0, 0];
  look.shX = sh.x || 0;
  look.shY = sh.y || 0;
  look.shC = look.shC || ('#' + hex2(shc[0]) + hex2(shc[1]) + hex2(shc[2]));
  look.shA = look.shA !== undefined ? look.shA : (shc[3] || 0);
  const ol = look.outline || {{}};
  const olc = ol.color || [0, 0, 0, 1];
  look.outSize = ol.size || 0;
  look.outC = look.outC || ('#' + hex2(olc[0]) + hex2(olc[1]) + hex2(olc[2]));
  look.outA = look.outA !== undefined ? look.outA : (olc[3] !== undefined ? olc[3] : 1);
  look.outDrop = look.outDrop !== undefined ? look.outDrop : !!ol.drop;
  return look;
}}

/* 은은하게 만드는 값.
   슬레이트에는 번짐이 없으므로 **옅은 테두리 + 옅은 그림자**로 흉내 낸다.
   테두리를 그림자에도 입히면 그림자가 두꺼워져 더 부드럽게 읽힌다. */
const PRESETS = {{
  none: {{outSize: 0, outA: 1, shX: 0, shY: 0, shA: 0, outDrop: false}},
  soft: {{outSize: 3, outC: '#000000', outA: 0.30,
          shX: 0, shY: 2, shC: '#000000', shA: 0.35, outDrop: true}},
  hard: {{outSize: 1, outC: '#000000', outA: 1,
          shX: 1, shY: 1, shC: '#000000', shA: 1, outDrop: false}},
}};

function lookOf(widget) {{
  const now = swaps[keyOf(widget)];
  const look = baseLook(widget);
  // 판은 세로 정렬을 따로 안 들고 있다. auto 칸이면 alignment.y 가 곧 그것이다.
  if (look.vert === undefined) {{
    const y = look.auto && look.align ? Number(look.align[1]) : 0;
    look.vert = y > 0.75 ? 'BOTTOM' : (y > 0.25 ? 'CENTER' : 'TOP');
  }}
  return (now && typeof now === 'object') ? Object.assign(look, now) : look;
}}

/* pt 는 픽셀이 아니다. 슬레이트는 N pt 를 N x 96/72 픽셀로 그린다.
   구울 때는 곱해 두는데 여기서 빼먹어, 갈아 끼운 글자만 4분의 1 작게
   나왔다 -- 칸에서는 49px, 큰 창에서는 37px 이었다. */
const PT_TO_PX = 96 / 72;

function putText(widget, look) {{
  keepOriginal(widget);
  const family = look.font.startsWith('F_HUD')
    ? "'" + look.font + "', 'Malgun Gothic', sans-serif" : 'system-ui, sans-serif';
  widget.style.fontSize = (look.size * PT_TO_PX).toFixed(0) + 'px';
  widget.style.color = look.color;
  widget.style.fontFamily = family;
  widget.style.fontWeight = String(look.typeface).toLowerCase().startsWith('bold') ? 700 : 400;
  widget.style.justifyContent = FLEX_X[look.just] || 'flex-start';
  widget.style.alignItems = FLEX_Y[look.vert] || 'flex-start';
  // **번짐은 쓰지 않는다.** 슬레이트 그림자는 글자를 한 번 밀어 다시 그린
  // 것이라 딱딱하다. css blur 로 부드럽게 그리면 게임과 또 달라진다.
  const a = Number(look.shA) || 0;
  const out = Number(look.outSize) || 0;
  const outA = look.outA !== undefined ? Number(look.outA) : 1;
  const marks = [];
  if (a > 0 && (look.shX || look.shY)) {{
    const dx = look.shX * PT_TO_PX, dy = look.shY * PT_TO_PX;
    if (look.outDrop && out > 0) {{
      // 테두리를 그림자에도 입히면 그림자가 테두리만큼 두꺼워진다.
      // css 에는 그런 기능이 없어 둘레로 여러 번 찍어 흉내 낸다.
      const r = out * PT_TO_PX;
      for (let i = 0; i < 8; i++) {{
        const t = i * Math.PI / 4;
        marks.push((dx + Math.cos(t)*r).toFixed(1) + 'px '
                 + (dy + Math.sin(t)*r).toFixed(1) + 'px 0 ' + hexA(look.shC, a));
      }}
    }}
    marks.push(dx.toFixed(1) + 'px ' + dy.toFixed(1) + 'px 0 ' + hexA(look.shC, a));
  }}
  widget.style.textShadow = marks.length ? marks.join(', ') : 'none';
  widget.style.webkitTextStroke = out > 0
    ? (out * PT_TO_PX).toFixed(1) + 'px ' + hexA(look.outC, outA) : '';
  widget.style.paintOrder = out > 0 ? 'stroke fill' : '';
  widget.classList.add('swapped');
}}

function hexA(hex, alpha) {{
  const n = parseInt(String(hex).slice(1), 16) || 0;
  return 'rgba(' + ((n >> 16) & 255) + ',' + ((n >> 8) & 255) + ',' + (n & 255)
    + ',' + alpha + ')';
}}

function drawText() {{
  const on = isText(picked);
  document.getElementById('trytext').classList.toggle('on', on);
  document.getElementById('tryimg').classList.toggle('off', on);
  document.querySelector('#tryacts [data-act="none"]').disabled = on;
  if (!on) return;
  const look = lookOf(picked);
  const font = document.getElementById('txFont');
  font.innerHTML = FONTS.map(f =>
    '<option' + (f === look.font ? ' selected' : '') + '>' + f + '</option>').join('');
  document.getElementById('txSize').value = Math.round(look.size);
  document.getElementById('txSizeR').value = Math.round(look.size);
  document.getElementById('txColor').value = look.color;
  document.getElementById('txColorHex').textContent = look.color;
  for (const [id, value] of [['txH', look.just], ['txV', look.vert]]) {{
    for (const chip of document.querySelectorAll('#' + id + ' b'))
      chip.classList.toggle('on', chip.dataset.v === value);
  }}
  document.getElementById('txOut').value = look.outSize;
  document.getElementById('txOutR').value = look.outSize;
  document.getElementById('txOutC').value = look.outC;
  document.getElementById('txShX').value = look.shX;
  document.getElementById('txShY').value = look.shY;
  document.getElementById('txShC').value = look.shC;
  document.getElementById('txShA').value = Math.round(Number(look.shA) * 100);
  document.getElementById('txOutA').value = Math.round(Number(look.outA) * 100);
  document.getElementById('txOutDrop').checked = !!look.outDrop;
  const was = TEXTS[keyOf(picked)];
  document.getElementById('txWas').textContent = was
    ? '판에 적힌 값: ' + was.font + ' ' + Math.round(was.size) + 'pt · '
      + was.just + (was.auto ? ' · auto' : ' · 칸 ' + (was.box || ['?','?']).join('x'))
    : '판에서 못 찾음';
}}

function editText(change) {{
  if (!isText(picked)) return;
  const look = Object.assign(lookOf(picked), change);
  putText(picked, look);
  swaps[keyOf(picked)] = {{font: look.font, typeface: look.typeface, size: Number(look.size),
                          color: look.color, just: look.just, vert: look.vert,
                          outSize: Number(look.outSize) || 0, outC: look.outC,
                          outA: Number(look.outA), outDrop: !!look.outDrop,
                          shX: Number(look.shX) || 0, shY: Number(look.shY) || 0,
                          shC: look.shC, shA: Number(look.shA) || 0}};
  saveSwaps();
}}

for (const [id, key, scale] of [['txOut', 'outSize', 1], ['txOutR', 'outSize', 1],
                                ['txShX', 'shX', 1], ['txShY', 'shY', 1],
                                ['txShA', 'shA', 0.01], ['txOutA', 'outA', 0.01]]) {{
  document.getElementById(id).addEventListener('input',
    e => editText({{[key]: Number(e.target.value) * scale}}));
}}
for (const [id, key] of [['txOutC', 'outC'], ['txShC', 'shC']]) {{
  document.getElementById(id).addEventListener('input',
    e => editText({{[key]: e.target.value}}));
}}
document.getElementById('txOutDrop').addEventListener('change',
  e => editText({{outDrop: e.target.checked}}));
document.getElementById('txPreset').addEventListener('click', event => {{
  event.stopPropagation();
  const chip = event.target.closest('b[data-p]');
  if (chip && PRESETS[chip.dataset.p]) editText(PRESETS[chip.dataset.p]);
}});

document.getElementById('txFont').addEventListener('change', e => editText({{font: e.target.value}}));
for (const id of ['txSize', 'txSizeR']) {{
  document.getElementById(id).addEventListener('input', e => editText({{size: Number(e.target.value)}}));
}}
document.getElementById('txColor').addEventListener('input', e => editText({{color: e.target.value}}));
for (const [id, key] of [['txH', 'just'], ['txV', 'vert']]) {{
  document.getElementById(id).addEventListener('click', event => {{
    event.stopPropagation();
    const chip = event.target.closest('b[data-v]');
    if (chip) editText({{[key]: chip.dataset.v}});
  }});
}}

/* 이 위젯과 **같은 자리에 겹쳐** 그림을 들고 있는 것. 없으면 null.
   자리가 거의 같아야 한다 -- 화면 전체를 덮는 배경까지 "밑엣것" 이라고
   하면 아무 도움이 안 된다. */
function holder(widget) {{
  const mine = widget.getBoundingClientRect();
  if (mine.width < 2 || mine.height < 2) return null;
  let best = null;
  for (const other of bigStage.querySelectorAll('.w[data-src]')) {{
    if (other === widget || !other.dataset.src) continue;
    const box = other.getBoundingClientRect();
    const overlap = Math.max(0, Math.min(mine.right, box.right) - Math.max(mine.left, box.left))
                  * Math.max(0, Math.min(mine.bottom, box.bottom) - Math.max(mine.top, box.top));
    const union = mine.width * mine.height + box.width * box.height - overlap;
    if (union > 0 && overlap / union > 0.7) {{ best = other; break; }}
  }}
  return best;
}}

function nameToPng(name) {{
  return ASSETS.find(a => a.name === name) || null;
}}

function applySaved() {{
  if (!openCard) return;
  for (const widget of bigStage.querySelectorAll('.w[data-widget]')) {{
    const want = swaps[keyOf(widget)];
    if (want === undefined) continue;
    if (want && typeof want === 'object') putText(widget, lookOf(widget));
    else if (want === NONE) putNothing(widget);
    else {{ const hit = nameToPng(want); if (hit) putImage(widget, hit.png, hit); }}
  }}
}}

function updateTally() {{
  const mine = openCard
    ? Object.keys(swaps).filter(k => k.startsWith(openCard.dataset.asset + '/')).length : 0;
  document.getElementById('trytally').innerHTML =
    '이 화면에서 바꾼 것 <b>' + mine + '</b>개 · 전체 <b>' + Object.keys(swaps).length + '</b>개';
  const has = picked && swaps[keyOf(picked)] !== undefined;
  document.querySelector('#tryacts [data-act="undo"]').disabled = !has;
  // 글자칸에는 지울 그림이 없다. 그림칸일 때만 켠다.
  document.querySelector('#tryacts [data-act="none"]').disabled = !picked
    || isText(picked);
}}

let saveTimer = null;

function saveSwaps() {{
  // 막대를 끌면 입력이 쏟아진다. 그때마다 보내면 늦게 도착한 옛 값이
  // 나중 값을 덮을 수 있다 -- 잠깐 모았다 한 번만 보낸다.
  clearTimeout(saveTimer);
  saveTimer = setTimeout(() => {{
    fetch('/save', {{ method: 'POST', headers: {{ 'Content-Type': 'application/json' }},
                     body: JSON.stringify({{ tryon: swaps }}) }});
  }}, 250);
  updateTally(); drawOptions();
  document.getElementById('txColorHex').textContent =
    document.getElementById('txColor').value;
}}

/* 한꺼번에 지우는 단추는 뒀다가 없앴다.
   한 번 잘못 누르면 맞춰 둔 것이 통째로 날아가는데, 얻는 것은 "여러 번
   되돌리기" 뿐이다. 값이 안 맞는 거래다. 실수는 서버가 저장 직전 앞판을
   tryon.prev.json 에 남기는 것으로 받쳐 준다. */
document.getElementById('tryacts').addEventListener('click', event => {{
  event.stopPropagation();
  const act = event.target.closest('button[data-act]');
  if (!act || act.disabled || !picked) return;
  if (act.dataset.act === 'undo') {{
    delete swaps[keyOf(picked)];
    putBack(picked);
  }} else if (act.dataset.act === 'none') {{
    putNothing(picked);
    swaps[keyOf(picked)] = NONE;
  }}
  saveSwaps(); drawText();
}});

document.getElementById('tryon').addEventListener('click', event => {{
  // 패널 안의 누름은 전부 여기서 끝낸다. 밖으로 흘리면 크게 보기가 닫힌다.
  event.stopPropagation();
  if (event.target.closest('.fold')) {{ showPanel(false); return; }}
  const copy = event.target.closest('.copy');
  if (copy) {{
    navigator.clipboard.writeText(copy.dataset.copy);
    copy.textContent = '복사됨'; copy.classList.add('done');
    return;
  }}
  const opt = event.target.closest('.opt');
  if (!opt || !picked) return;
  putImage(picked, opt.dataset.png, nameToPng(opt.dataset.name));
  swaps[keyOf(picked)] = opt.dataset.name;
  saveSwaps();
}});

document.getElementById('tryq').addEventListener('input', drawOptions);
loadAssets();

document.addEventListener('keydown', event => {{
  if (!big.classList.contains('on')) return;
  if (event.key === 'Escape') {{ big.classList.remove('on'); return; }}
  if (event.key === 'ArrowLeft' || event.key === 'ArrowRight') {{
    event.preventDefault();
    stepBig(event.key === 'ArrowRight' ? 1 : -1);
  }}
}});

window.addEventListener('resize', () => {{ if (big.classList.contains('on')) fitBig(); }});
</script>
</body></html>"""
    OUT.write_text(page, encoding="utf-8")
    print(f"{OUT}  ({len(by_asset)}개 에셋 · {shown}칸)")


if __name__ == "__main__":
    main()
