"""글자칸마다 **칸에 들어가는 가장 큰 크기**를 잰다.

왜 재나
-------
"제일 크게" 를 눈대중으로 정하면 어떤 칸은 삐져나가고 어떤 칸은 헐렁하다.
칸 높이만 보고 키우면 더 나쁘다 -- 150px 짜리 칸에 111pt 를 넣으면 세로는
맞지만 가로로 한참 넘친다.

그래서 **폭까지 실제로 잰다.** FontFace 에셋에서 꺼내 둔 TTF 로 글줄 폭을
재고(carve_fonts.py), 높이는 글꼴이 말하는 ascent+descent 로 잡는다.

    들어가는 크기 = min(칸높이 / 줄높이비율, 칸너비 / 글줄폭비율)

글이 비어 있는 칸
-----------------
36개는 판에 글이 없다 -- 런타임에 채운다(이름 · 숫자 · 스킬명). 폭을 잴
대상이 없으므로 높이로만 잡되 **지금 크기의 1.6배까지**만 키운다. 무엇이
들어올지 모르는데 세 배로 키우면 그게 곧 삐져나가는 원인이 된다.

    python Tools/UI/fit_text.py
"""

import json
import math
import re
from pathlib import Path

import sys

from PIL import ImageFont

ROOT = Path("D:/UnrealProjects/P_RD_develop_20260803")
sys.path.insert(0, str(ROOT / "Tools/UI"))

import wbp_names  # noqa: E402
MOCKUPS = ROOT / "Tools/UI/mockups"
WORKSPACE = Path("D:/UnrealProjects_WBP_Editor/data/workspace.json")
OUT = MOCKUPS / "text_fit.json"

# 게임 글꼴 -> 잴 때 쓸 TTF.
#
# Oswald 와 LINESeed 는 에셋에서 꺼낸 진짜다. Roboto 와 NotoSansKR 은 꺼낼
# 원본이 없어 폭이 비슷한 것으로 대신 잰다 -- 크기를 정하는 데는 충분하다.
FACES = {
    ("F_HUD_Oswald", True): MOCKUPS / "fonts/Oswald-Bold.ttf",
    ("F_HUD_Oswald", False): MOCKUPS / "fonts/Oswald-Regular.ttf",
    ("F_HUD_LINESeedKR", True): MOCKUPS / "fonts/LINESeedKR-Bold.ttf",
    ("F_HUD_LINESeedKR", False): MOCKUPS / "fonts/LINESeedKR-Regular.ttf",
    ("Roboto", True): Path("C:/Windows/Fonts/arialbd.ttf"),
    ("Roboto", False): Path("C:/Windows/Fonts/arial.ttf"),
    ("F_HUD_NotoSansKR", True): Path("C:/Windows/Fonts/malgunbd.ttf"),
    ("F_HUD_NotoSansKR", False): Path("C:/Windows/Fonts/malgun.ttf"),
}

PROBE = 100          # 이 크기로 재고 비율로 환산한다

# **pt 는 픽셀이 아니다.**
#
# 슬레이트는 크기 N 인 글꼴을 N x 96/72 픽셀로 그린다(DefaultEngine.ini 의
# FontDPI=72). 이걸 빼먹고 pt 를 그대로 픽셀로 여겼더니 재 준 크기가 죄다
# 3분의 1 크게 나왔다 -- 타이틀 단추에 39pt 가 들어가 틀 밖으로 삐져나왔다.
#
# 게임 화면을 찍어 20 · 26 · 32 · 39pt 를 나란히 재서 확인한 값이다:
#   대문자높이(디자인px) = 1.105 x pt  ->  em 배율 1.316 (4/3 = 1.333)
PT_TO_PX = 96.0 / 72.0
MARGIN_X = 0.94      # 틀에 딱 붙지 않게 조금 남긴다
# 세로는 거의 꽉 채워도 된다.
#
# 줄 상자(ascent+descent)가 칸보다 커지는 순간 두 가지가 한꺼번에 망가진다 --
# 글자가 틀 밖으로 나가고, 세로 가운데도 잃는다(내용이 칸보다 크면 슬레이트는
# 가운데 대신 위에 붙인다). 그 선만 안 넘으면 된다. 눈에 보이는 대문자는 줄
# 상자의 56% 뿐이라 여유는 이미 충분하다.
MARGIN_Y = 0.95
SMALLEST, BIGGEST = 10, 72
UNKNOWN_BOX = (100.0, 30.0)   # 추출본이 못 잰 칸에 넣는 기본값

# 얼마나 키울 수 있나.
#
# 칸에 들어가는 최대치를 그냥 쓰면 안 된다. 판에 적힌 글은 자리 표시인 경우가
# 많아서('스킬 1', '-'), 그걸 기준으로 꽉 채우면 진짜 이름이 들어오는 순간
# 삐져나간다. 그래서 배율에 뚜껑을 씌운다 -- 디자이너가 정한 비례는 지키고
# 크기만 올린다.
MAX_GROW = 1.8       # 글이 최종인 칸
# 자리 표시 칸에 몇 글자가 들어온다고 볼 것인가. 지금 크기를 기준 삼을 수도
# 있지만, 그 값 자체가 한 번 잘못 부풀려진 적이 있어 못 믿는다. 칸 크기만
# 보는 편이 낫다.
BLIND_CHARS = 3.0

# 자리 표시로 보이는 글. 이런 것에 폭을 맞추면 안 된다.
PLACEHOLDER = re.compile(r"^(-+|0+|[-+]?\d[\d,.%/]*|.{1,6}\s*\d+|"
                         r"(?i:text|label|name|value|lorem).*)$")


def is_placeholder(text):
    return not text or bool(PLACEHOLDER.match(text))

_cache = {}


def face(font_name, bold):
    key = (font_name, bold)
    if key not in _cache:
        path = FACES.get(key) or FACES.get((font_name, not bold))
        _cache[key] = (ImageFont.truetype(str(path), PROBE)
                       if path and path.is_file() else None)
    return _cache[key]


def texts():
    """글자칸 -> (글, 칸 너비, 칸 높이).

    감싼 글자는 추출본에서 빠져 자기 rect 가 없다. 감싼 Overlay 의 자리가
    곧 글자칸이므로 wbp_names 가 그것을 짝지어 준다. 글은 판에서 뽑은
    text_style.json 에 들어 있다.
    """
    sizes = wbp_names.boxes()
    styles = json.loads((MOCKUPS / "text_style.json").read_text(
        encoding="utf-8"))["styles"]
    found = {}
    for key, size in sizes.items():
        shown = (styles.get(key, {}).get("text") or "").strip()
        found[key] = (shown, size[0], size[1])
    return found


def main():
    styles = json.loads((MOCKUPS / "text_style.json").read_text(
        encoding="utf-8"))["styles"]
    strings = texts()

    fits, grew, shrank, blind, skipped = {}, 0, 0, 0, 0
    for key, look in styles.items():
        now = float(look.get("size", 20.0))
        text, box_w, box_h = strings.get(key, ("", 0.0, 0.0))
        # 칸 크기는 **판에서 방금 뽑은 값**을 먼저 쓴다.
        #
        # 추출본(workspace.json)의 자리는 WBP 편집기가 뽑아 둔 때의 것이라,
        # 그 뒤에 칸을 고치면 옛 값이 남는다 -- 타이틀 글자칸을 41px 로 키워
        # 놓고도 옛 28px 로 재서 13pt 를 내놨다.
        #
        # 다만 슬롯 값이 크기를 뜻하지 않는 경우가 있다. 앵커를 늘려 놓은
        # 칸에서는 안쪽 여백이고, auto 칸에서는 남아 있는 옛 값이다. 그럴
        # 때만 추출본으로 물러선다.
        slot_box = look.get("box")
        if (slot_box and not look.get("stretched") and not look.get("auto")
                and tuple(slot_box) != UNKNOWN_BOX
                and slot_box[0] > 1.0 and slot_box[1] > 1.0):
            box_w, box_h = float(slot_box[0]), float(slot_box[1])
        if box_h <= 1.0 or box_w <= 1.0 or (box_w, box_h) == UNKNOWN_BOX:
            # 100x30 은 추출본이 **못 잰 칸**에 넣는 기본값이다(40칸).
            # 이걸 진짜 칸으로 믿고 '설정' 을 70 -> 13pt 로 줄이려 했다.
            # 모르는 것은 모르는 대로 두고 손대지 않는다.
            skipped += 1
            continue

        typeface = str(look.get("typeface", "")).lower()
        probe = face(look.get("font", "Roboto"), typeface.startswith("bold"))
        if probe is None:
            skipped += 1
            continue

        ascent, descent = probe.getmetrics()
        line = (ascent + descent) / PROBE * PT_TO_PX      # pt 하나가 먹는 세로 픽셀
        by_height = box_h * MARGIN_Y / line

        if look.get("wrap"):
            # 문단이다. 키우면 줄 수가 늘어 칸을 넘긴다 -- 설명글은 지금
            # 크기가 맞다. 가운데만 맞춘다.
            fits[key] = {"size": int(round(now)), "was": now,
                         "why": "줄바꿈 켜짐(문단) -- 크기는 그대로",
                         "box": [round(box_w, 1), round(box_h, 1)],
                         "text": text[:24]}
            skipped += 1
            continue

        if look.get("auto"):
            # auto 칸은 글자를 딱 감싼다 -- 잰 자리가 곧 지금 글자 크기라
            # 기준이 없다. 크기는 그대로 두고 가운데만 맞춘다.
            fits[key] = {"size": int(round(now)), "was": now,
                         "why": "auto 칸이라 키울 기준이 없음",
                         "box": [round(box_w, 1), round(box_h, 1)],
                         "text": text[:24]}
            skipped += 1
            continue

        if is_placeholder(text):
            # 자리 표시다. 폭을 안 보면 '스킬 1' 두 글자에 맞춰 67pt 를 주고,
            # 진짜 이름이 들어오는 순간 넘친다. **넓은 글자 세 개**는 들어갈
            # 자리를 남긴다 -- 한글 한 글자가 대략 1em 이다.
            blind_em = max(probe.getlength(text) / PROBE if text else 0.0,
                           BLIND_CHARS)
            by_width = box_w * MARGIN_X / (blind_em * PT_TO_PX)
            want = min(by_height, by_width)
            why = (f"자리 표시 -- 높이 {by_height:.0f} · "
                   f"폭({blind_em:.1f}글자 가정) {by_width:.0f}")
            blind += 1
        else:
            width = probe.getlength(text) / PROBE * PT_TO_PX
            by_width = (box_w * MARGIN_X / width) if width > 0 else by_height
            want = min(by_height, by_width, now * MAX_GROW)
            why = f"높이 {by_height:.0f} · 폭 {by_width:.0f} · {MAX_GROW}배까지"

        # 줄이는 것도 한다.
        #
        # 처음에는 "제일 크게" 라는 부탁이니 키우기만 했는데, 그때 계산이
        # pt->픽셀 4/3 을 빼먹어 오히려 33% 크게 넣어 버렸다. 이제 계산이
        # 맞으니 **칸에 들어가는 크기가 곧 제일 큰 크기**다 -- 넘치는 것은
        # 커 보이는 게 아니라 틀 밖으로 나가고, 세로 가운데도 잃는다
        # (내용이 칸보다 크면 슬레이트는 가운데 대신 위에 붙인다).
        size = max(SMALLEST, min(BIGGEST, int(math.floor(want))))
        if size < now:
            shrank += 1
        elif size > now:
            grew += 1
        fits[key] = {"size": size, "was": now, "why": why,
                     "box": [round(box_w, 1), round(box_h, 1)], "text": text[:24]}

    OUT.write_text(json.dumps(fits, ensure_ascii=False, indent=1), encoding="utf-8")
    print(f"{len(fits)}개 잼  (키움 {grew} · 줄임 {shrank} · "
          f"글이 빈 칸 {blind} · 못 잼 {skipped})")

    biggest = sorted(fits.items(), key=lambda kv: kv[1]["size"] - kv[1]["was"],
                     reverse=True)
    print("\n가장 많이 커지는 것")
    for key, fit in biggest[:8]:
        print(f"  {fit['was']:.0f} -> {fit['size']:<3} 칸 "
              f"{fit['box'][0]:.0f}x{fit['box'][1]:.0f}  "
              f"{key.rsplit('/', 1)[-1]}  '{fit['text']}'")
    print("\n가장 많이 작아지는 것 (지금 삐져나가는 것)")
    for key, fit in biggest[-6:]:
        print(f"  {fit['was']:.0f} -> {fit['size']:<3} 칸 "
              f"{fit['box'][0]:.0f}x{fit['box'][1]:.0f}  "
              f"{key.rsplit('/', 1)[-1]}  '{fit['text']}'")


if __name__ == "__main__":
    main()
