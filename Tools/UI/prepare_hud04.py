# -*- coding: utf-8 -*-
"""시안4 전투 HUD 의 판과 자리표를 준비한다.

## 왜 손으로 안 적나

시안4 는 판을 뜯은 결과와 그 안의 자리를 전부 기계가 읽을 수 있는 형태로
받았다.

    cropped_ui-v2/manifest.json          판 열넷과 화면에서의 자리
    detail_coordinates/detail_coordinates.json   판마다의 글자/아이콘 자리

앞선 시안들은 색과 크기로 판정하는 기계를 만들었다가 시안마다 다르게 틀려서
버렸고, 그 뒤로는 눈으로 보고 적었다. 여기서는 적을 것이 없다 -- 이름이 이미
붙어 있다. 손으로 옮기면 옮기는 만큼 틀린다.

## 하는 일

    1. 투명 판 열넷을 아트 폴더로 옮긴다
    2. hud04_slots.py 를 만든다 (판 자리 + 판 안의 자리)

임포트는 언리얼 안에서 해야 하므로 import_hud04.py 가 따로 한다.

    python prepare_hud04.py
"""
import io
import json
import os
import shutil

from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
SOURCE = r"D:/UnrealProjects/P_RD_develop/시안4"
PLATES = os.path.join(SOURCE, "클롭_투명")
MANIFEST = os.path.join(SOURCE, "cropped_ui-v2", "manifest.json")
DETAIL = os.path.join(SOURCE, "detail_coordinates", "detail_coordinates.json")

ART = os.path.join(HERE, "KayKitUIKit", "HUD04")
SLOTS = os.path.join(HERE, "hud04_slots.py")

#: 판 이름 -> 텍스처 이름. 번호 붙은 파일명을 그대로 쓰면 에셋 이름이
#: "01_top_left_parchment" 이 되어 코드에서 읽기 나쁘다.
def texture_name(stem):
    return "KK_HUD04_" + stem.split("_", 1)[1] if stem[:2].isdigit() else "KK_HUD04_" + stem


def collect_plates():
    """투명 판을 아트 폴더로 옮기고 이름을 돌려준다."""
    os.makedirs(ART, exist_ok=True)
    names = {}
    for entry in sorted(os.listdir(PLATES)):
        if not entry.lower().endswith(".png"):
            continue
        stem = os.path.splitext(entry)[0]
        target = texture_name(stem)
        shutil.copyfile(os.path.join(PLATES, entry),
                        os.path.join(ART, target + ".png"))
        # "01_top_left_parchment" -> "top_left_parchment"
        names[stem.split("_", 1)[1] if stem[:2].isdigit() else stem] = target
    return names


def write_slots(place, detail, textures):
    lines = [
        "# -*- coding: utf-8 -*-",
        '"""시안4 전투 HUD 의 자리표. prepare_hud04.py 가 만든다.',
        "",
        "손으로 고치지 마라. 시안이 바뀌면 다시 만든다 -- 손으로 고친 값은",
        "다음에 만들 때 조용히 사라진다.",
        "",
        "PLACE   판을 화면 어디에 놓나. 시안 1672 x 941 기준.",
        "DETAIL  판 안의 글자와 그림 자리. **화면 기준**이다.",
        "",
        "판 안 기준이 아니라 화면 기준으로 두는 이유: 판을 캔버스에 놓고 그 안에",
        "다시 얹으면 두 좌표계를 오가야 하고, 그러다 한 번 어긋나면 어느 쪽이",
        "틀렸는지 안 보인다. 전부 한 좌표계로 둔다.",
        '"""',
        "",
        "#: 판 이름 -> 텍스처 이름.",
        "TEXTURE = {",
    ]
    for key in sorted(textures):
        lines.append('    "%s": "%s",' % (key, textures[key]))
    lines += ["}", "", "#: 판을 화면 어디에 놓나. (x, y, w, h)", "PLACE = {"]
    for key in sorted(place):
        lines.append("    \"%s\": %s," % (key, tuple(place[key])))
    lines += ["}", "",
              "#: 판 안의 자리. 판 이름 -> {요소 이름: (x, y, w, h)}",
              "DETAIL = {"]
    for key in sorted(detail):
        lines.append('    "%s": {' % key)
        for name in detail[key]:
            lines.append("        \"%s\": %s," % (name, tuple(detail[key][name])))
        lines.append("    },")
    lines += ["}", ""]
    io.open(SLOTS, "w", encoding="utf-8", newline="\n").write("\n".join(lines))


textures = collect_plates()
detail_raw = json.load(io.open(DETAIL, encoding="utf-8"))
detail = {}
place = {}
for asset in detail_raw["assets"]:
    stem = os.path.splitext(asset["file"])[0]
    detail[stem] = {e["id"]: e["canvas_bbox"] for e in asset["elements"]}

    # 자리는 crop_origin 과 **실제 그림 크기**로 잡는다. 투명 판은 둘레에
    # 4픽셀 여유를 두고 잘려 있어서, manifest 의 source_box 와 size 를 쓰면
    # 판마다 4픽셀씩 어긋난다 -- 열넷이 다 같은 방향으로 밀린다.
    left, top = asset["crop_origin"]
    with Image.open(os.path.join(ART, textures[stem] + ".png")) as image:
        width, height = image.size
    place[stem] = (left, top, width, height)

#: 명령 카드 여섯. 차례는 시안 그림 차례다.
CARDS = ("action_top", "action_left_upper", "action_right_upper",
         "action_left_lower", "action_right_lower", "action_bottom")

#: 여섯을 맞출 본. 위 칸이다.
#:
#: 여섯이 어차피 한 벌이라 어느 것을 본으로 삼든 되고, 위 칸이 사람이 기준으로
#: 삼기 쉬운 자리다.
CARD_TEMPLATE = "action_top"

#: 본에 없는 요소를 어디서 가져오나. 앞에서부터 찾는다.
#:
#: 본으로 삼은 판에 요소가 다 있는 것은 아니다. 시안이 카드마다 다른 내용을
#: 그려서, 위 칸에는 피해도 쿨타임도 안 적혀 있다. 그것만 보고 통일하면 그 셋이
#: 여섯 장에서 통째로 사라지고, 굽는 코드는 없는 자리를 건너뛰므로 위젯이
#: 아예 안 만들어진다 -- 런타임이 이름으로 찾다가 못 찾고 조용히 넘어간다.
CARD_FALLBACK = ("action_right_upper", "action_right_lower", "action_bottom",
                 "action_left_lower", "action_left_upper")


def unify_cards(place, detail):
    """명령 카드 여섯의 판 안 자리를 한 벌로 맞춘다.

    ## 왜 다르게 나오나

    시안이 카드마다 다른 내용을 그렸다. 어떤 카드는 피해가 적혀 있고 어떤
    카드는 쿨타임이 적혀 있다. 재는 쪽은 **그려진 것만** 재므로 카드마다
    요소 수도, 글자 상자 크기도 달라진다 -- 이름칸이 49px 인 카드와 91px 인
    카드가 같이 있었다.

    그런데 런타임은 여섯을 **같은 카드**로 다룬다. 자리가 제각각이면 3번
    카드에서만 글자가 반 칸 내려앉는 식이 되고, 그걸 여섯 번 따로 맞춰야 한다.

    ## 어떻게 맞추나

    요소를 가장 많이 잰 판을 본으로 삼아, 그 **판 안 자리**를 나머지에 그대로
    옮긴다. 판 자체 크기가 1~5px 씩 다르지만 카드는 늘리지 않고 놓는 것이라
    안 맞춰도 된다 -- 안쪽 자리만 같으면 된다.

    본에 없는 두 가지는 만들어 넣는다.

        cooldown_overlay  쿨타임 가림막. 카드를 통째로 덮는다
        stance_text       "방어 태세" 같은 글. 피해 자리와 같은 칸을 쓴다

    이 함수가 **잰 값을 덮어쓴다.** 통일은 잰 결과가 아니라 정한 것이라
    여기에 둔다 -- hud04_slots.py 를 손으로 고치면 다음에 만들 때 사라진다.
    """
    def boxes(plate):
        left, top = place[plate][0], place[plate][1]
        return {element: [rect[0] - left, rect[1] - top, rect[2], rect[3]]
                for element, rect in detail[plate].items()}

    template = boxes(CARD_TEMPLATE)

    # 본에 없는 것은 다른 판에서 데려온다. 본을 덮지는 않는다.
    for spare in CARD_FALLBACK:
        for element, rect in boxes(spare).items():
            template.setdefault(element, rect)

    # 가림막은 선택 테두리와 같은 자리를 덮는다. 덮는 것과 두르는 것이
    # 어긋나면 같은 카드가 상태에 따라 다른 크기로 보인다.
    if "selected_outline" in template:
        template["cooldown_overlay"] = list(template["selected_outline"])

    # 태세 글은 피해 글과 같은 칸이다. 한 카드가 둘을 같이 쓰는 일은 없다.
    if "damage_text" in template:
        template["stance_text"] = list(template["damage_text"])

    borrowed = sorted(set(template) - set(detail[CARD_TEMPLATE])
                      - {"cooldown_overlay", "stance_text"})
    if borrowed:
        print("본에 없어 데려온 것: %s" % ", ".join(borrowed))

    for card in CARDS:
        left, top = place[card][0], place[card][1]
        detail[card] = {element: [rect[0] + left, rect[1] + top,
                                  rect[2], rect[3]]
                        for element, rect in template.items()}
    return sorted(template)


unified = unify_cards(place, detail)
print("명령 카드 %d장을 %s 기준으로 통일: %s"
      % (len(CARDS), CARD_TEMPLATE, ", ".join(unified)))

missing = set(place) - set(textures)
if missing:
    raise RuntimeError("판은 있는데 그림이 없다: %s" % sorted(missing))

write_slots(place, detail, textures)
print("판 %d장, 자리 %d판 -> %s" % (len(textures), len(detail), SLOTS))
