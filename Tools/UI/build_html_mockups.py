"""Generate HTML mockups — 5 variants per screen — from the measured art.

왜 HTML 인가
------------
WBP 로 시안을 만들면 한 번 보는 데 언리얼을 켜고 굽고 띄워야 한다. 배치를 다섯
개씩 비교하기엔 너무 느리다. HTML 은 새로고침이면 된다.

**그림은 실제 KitA 부품을 그대로 쓴다.** 색만 흉내 낸 목업이면 "실제로 붙이면
달라지는" 문제를 못 잡는다. 9-slice 도 CSS `border-image` 로 똑같이 준다 --
언리얼의 Box 브러시와 계산이 같아서, 여기서 안 뭉개지면 게임에서도 안 뭉개진다.

자리는 짐작하지 않는다
----------------------
칸 위치는 ``measure_openings.py`` 가 잰 값에서 나온다.

    겉(bbox)  장식이 끝나는 자리. 9-slice 마진이 여기까지 와야 한다
    안        글자를 놓아도 삐져나오지 않는 자리. 둥근 구멍이면 겉보다 훨씬 작다

칩 링은 겉 95x95, 안 69x67 로 38% 차이가 난다. 겉을 글자 자리로 쓰면 링을 밟는다.

쓰는 법:
    python Tools/UI/build_html_mockups.py
    그 뒤 Tools/UI/mockups/index.html 을 띄운다
"""

import shutil
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from mockup_screens import SCREENS, VARIANTS  # noqa: E402

ROOT = Path("D:/UnrealProjects/P_RD_develop_20260803")
OUT = ROOT / "Tools/UI/mockups"
IMG = OUT / "img"

# 실측값. measure_openings.py 가 낸 비율을 그대로 옮긴 것이다.
#   frame  세 열 통짜 틀의 구멍(참고용). 지금은 조립식이라 안 쓴다
#   outer  바깥 틀 하나. 9-slice 마진은 장식이 끝나는 자리(겉)
# 9-slice 마진은 **구멍이 시작하는 자리가 아니라 장식이 끝나는 자리**여야 한다.
# 구멍은 (65,73) 에서 시작하지만 모서리 황동은 x90 · y94 까지 뻗어 있다(실측).
# 65 로 자르면 장식 바깥 25px 이 늘어나는 구간에 들어가 이중선으로 보인다 --
# 실제로 그렇게 나왔다. 장식을 다 덮도록 100 으로 준다.
FRAME_OUTER = dict(src="T_KitA_Frame_Outer.png", size=(1920, 1080),
                   slice=(98, 91, 98, 91),
                   hole=(65, 73, 1854, 1012))
DIVIDER = dict(src="T_KitA_Frame_Divider.png", size=(58, 1026), cap=69)

# 부품 -> (파일, 크기, 9-slice 마진, 안쪽 비율(글자 놓아도 되는 자리))
# 부품 -> (파일, 크기, 9-slice 마진, 안쪽 비율). kit_manifest_a 에서 뽑는다 --
# 숫자를 두 곳에 두면 갈라진다. 마진 0 은 9-slice 를 안 쓴다는 뜻이다.
PARTS = {
    "row": ("T_KitA_Row_Plate.png", (526, 140), (70, 64, 70, 64),
             (0.1236, 0.2286, 0.8745, 0.7643)),
    "button": ("T_KitA_Button_Wide_Normal.png", (764, 164), (65, 73, 65, 73),
             (0.1200, 0.1200, 0.8800, 0.8800)),
    "small": ("T_KitA_Button_Small_Normal.png", (281, 133), (85, 34, 85, 34),
             (0.1317, 0.3008, 0.8683, 0.6992)),
    "cell": ("T_KitA_Cell_Normal.png", (186, 176), (0, 0, 0, 0),
             (0.1452, 0.2159, 0.8548, 0.7841)),
    "chip": ("T_KitA_StatChip_Ring.png", (153, 152), (0, 0, 0, 0),
             (0.2745, 0.2829, 0.7255, 0.7237)),
    "portrait": ("T_KitA_Portrait_Frame.png", (268, 252), (73, 76, 73, 76),
             (0.1978, 0.1944, 0.8022, 0.8056)),
    "title": ("T_KitA_Title_Plate.png", (719, 183), (0, 0, 0, 0),
             (0.0793, 0.3443, 0.9207, 0.6557)),
    "hp": ("T_KitA_HPBar_Fill_Red.png", (365, 61), (25, 29, 25, 29),
             (0.0548, 0.3443, 0.9452, 0.6557)),
}

CSS = """
:root { color-scheme: dark; }
* { box-sizing: border-box; margin: 0; }
body { background: #14100c; color: #efe3cd;
       font: 15px/1.5 "Malgun Gothic", system-ui, sans-serif; padding: 24px; }
h1 { font-size: 22px; margin-bottom: 4px; }
h2 { font-size: 18px; margin: 28px 0 6px; color: #f0c479; }
.note, .sub { color: #a2917a; font-size: 13px; margin-bottom: 12px; }
.row { display: flex; gap: 18px; flex-wrap: wrap; }
.card { width: 620px; }
.card > .name { font-size: 13px; color: #f0c479; margin-bottom: 4px; }
.card > .why { color: #948572; margin-bottom: 6px; min-height: 32px; }
.stage { position: relative; width: 620px; height: 348.75px;
         background: #3b2d20; overflow: hidden; }
/* .stage 의 직계 자식은 .scale 하나뿐이다. 내용은 그 안에 있으므로
   여기서 잡아 주지 않으면 전부 static 이 되어 left/top 이 무시된다.
   실제로 그래서 시안이 전부 빈 틀만 나왔다. */
.stage > *, .scale > * { position: absolute; }
.scale { position: absolute; top: 0; left: 0; width: 1920px; height: 1080px;
         transform: scale(0.322916); transform-origin: 0 0; }
.frame { inset: 0; }
.divider { top: 0; }
.plate { }
.label { color: #3a2411; font-weight: 700; display: flex;
         align-items: center; justify-content: center; text-align: center;
         line-height: 1.15; }
.label.left { justify-content: flex-start; }
.heading { color: #7a4a12; font-weight: 800; }
.body { color: #4a3320; }
.grid { display: grid; gap: 6px; }
.cellbox { background: rgba(20,12,4,.35); }
.hit { background: rgba(240,200,120,.85); }
.aim { background: rgba(120,180,240,.85); }
"""


def fit_margin(margin, width, height):
    """그릴 크기에 맞게 마진을 줄인다.

    9-slice 는 테두리를 **원본 픽셀 크기 그대로** 그린다. 그래서 그릴 높이가
    위·아래 마진 합보다 작으면 두 테두리가 서로 겹쳐 그려진다. 실제로 설정 줄이
    그랬다 -- 받침 그림의 세로 마진이 64+64=128 인데 줄 높이는 74 였다.

    겹치면 테두리가 두 겹으로 보인다. 줄여 그리면 장식이 조금 작아질 뿐이다.
    가운데를 최소 12% 는 남긴다.
    """
    ml, mt, mr, mb = margin
    limit_x = max(1.0, width * 0.44)
    limit_y = max(1.0, height * 0.44)
    scale_x = min(1.0, limit_x / ml) if ml else 1.0
    scale_y = min(1.0, limit_y / mt) if mt else 1.0
    return (ml * scale_x, mt * scale_y, mr * scale_x, mb * scale_y)


def part_html(kind, x, y, width, height, text="", cls="label", size=None):
    """부품 하나. 9-slice 는 CSS border-image 로 준다 -- 언리얼 Box 와 같은 계산."""
    src, (tw, th), raw, inner = PARTS[kind]
    if raw[0] == 0 and raw[1] == 0:
        # 9-slice 를 안 쓰는 부품(원형 칩 · 파인 명패). 통짜로 늘린다.
        style = (f"left:{x}px;top:{y}px;width:{width}px;height:{height}px;"
                 f"background:url(img/{src}) center/100% 100% no-repeat;")
        out = [f'<div class="plate" style="{style}"></div>']
        if text:
            il, it, ir, ib = inner
            tx, ty = x + width * il, y + height * it
            tw2, th2 = width * (ir - il), height * (ib - it)
            out.append(f'<div class="{cls}" style="left:{tx}px;top:{ty}px;'
                       f'width:{tw2}px;height:{th2}px;'
                       f'font-size:{size or max(16, min(th2 * 0.5, 44))}px">{text}</div>')
        return "".join(out)
    ml, mt, mr, mb = fit_margin(raw, width, height)
    style = (f"left:{x}px;top:{y}px;width:{width}px;height:{height}px;"
             f"border-image:url(img/{src}) {raw[1]} {raw[2]} {raw[3]} {raw[0]} fill stretch;"
             f"border-width:{mt:.1f}px {mr:.1f}px {mb:.1f}px {ml:.1f}px;border-style:solid;")
    out = [f'<div class="plate" style="{style}"></div>']
    if text:
        il, it, ir, ib = inner
        tx, ty = x + width * il, y + height * it
        tw2, th2 = width * (ir - il), height * (ib - it)
        font = size or max(16, min(th2 * 0.5, 44))
        out.append(f'<div class="{cls}" style="left:{tx}px;top:{ty}px;'
                   f'width:{tw2}px;height:{th2}px;font-size:{font}px">{text}</div>')
    return "".join(out)


def text_html(x, y, width, height, text, cls="body", size=24, colour="#3a2411"):
    return (f'<div class="{cls}" style="left:{x}px;top:{y}px;width:{width}px;'
            f'height:{height}px;font-size:{size}px;color:{colour}">{text}</div>')


def frame_html(weights):
    """바깥 틀 + 기둥. 열 사각도 같이 돌려준다."""
    hx0, hy0, hx1, hy1 = FRAME_OUTER["hole"]
    ml, mt, mr, mb = FRAME_OUTER["slice"]
    out = [f'<div class="frame" style="border-image:url(img/{FRAME_OUTER["src"]}) '
           f'{mt} {mr} {mb} {ml} fill stretch;'
           f'border-width:{mt}px {mr}px {mb}px {ml}px;border-style:solid;'
           f'position:absolute;left:0;top:0;width:1920px;height:1080px"></div>']

    divider_w = DIVIDER["size"][0]
    usable = (hx1 - hx0) - divider_w * (len(weights) - 1)
    total = sum(weights)
    columns, cursor = [], hx0
    for index, weight in enumerate(weights):
        span = usable * (weight / total)
        columns.append((cursor, hy0, span, hy1 - hy0))
        cursor += span
        if index < len(weights) - 1:
            out.append(f'<img class="divider" src="img/{DIVIDER["src"]}" '
                       f'style="left:{cursor}px;top:{hy0 - 10}px;'
                       f'width:{divider_w}px;height:{hy1 - hy0 + 20}px">')
            cursor += divider_w
    return "".join(out), columns


def render(screen, variant):
    weights = variant["columns"]
    frame, columns = frame_html(weights)
    parts = [frame]
    parts.append(screen["draw"](columns, variant, part_html, text_html))
    if variant.get("title", "top") != "none":
        plate_w, plate_h = 640.0, 118.0
        top = 8.0 if variant["title"] == "top" else FRAME_OUTER["hole"][1] + 6.0
        parts.append(part_html("title", (1920 - plate_w) / 2, top,
                               plate_w, plate_h, screen["title"], size=48))
    return ('<div class="stage"><div class="scale">' + "".join(parts)
            + "</div></div>")


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    IMG.mkdir(exist_ok=True)
    for folder in (ROOT / "Saved/UIKit/ConceptA", ROOT / "Saved/UIKit/FrameA"):
        for path in folder.glob("*.png"):
            if path.name.startswith("_"):
                continue
            shutil.copyfile(path, IMG / path.name)
    # 부품 시트에서 뜬 것은 part_NN 이름이라, 매니페스트 이름으로 한 벌 더 둔다.
    sys.path.insert(0, str(ROOT / "Tools/UI"))
    from kit_manifest_a import PARTS as MANIFEST
    for index, name, *_ in MANIFEST:
        source = ROOT / f"Saved/UIKit/ConceptA/part_{index:02d}.png"
        if source.exists():
            shutil.copyfile(source, IMG / f"{name}.png")

    body = ["<h1>UI 배치 시안 — 컨셉 A</h1>",
            '<p class="note">그림은 실제 KitA 부품이고, 9-slice 는 CSS border-image 로 '
            '언리얼 Box 브러시와 같은 계산을 쓴다. 칸 위치는 measure_openings.py 실측값이다.</p>']
    for key, screen in SCREENS.items():
        body.append(f'<h2>{screen["title"]}</h2>')
        body.append(f'<p class="sub">{screen["note"]}</p>')
        body.append('<div class="row">')
        for variant in VARIANTS[key]:
            body.append('<div class="card">'
                        f'<div class="name">{variant["name"]}</div>'
                        f'<div class="why">{variant["why"]}</div>'
                        + render(screen, variant) + '</div>')
        body.append("</div>")

    html = ("<!doctype html><html lang=\"ko\"><head><meta charset=\"utf-8\">"
            "<title>UI 배치 시안 — 컨셉 A</title><style>" + CSS + "</style></head><body>"
            + "".join(body) + "</body></html>")
    (OUT / "index.html").write_text(html, encoding="utf-8")
    print(f"{OUT / 'index.html'}  화면 {len(SCREENS)}개 x 시안 5개")


if __name__ == "__main__":
    main()
