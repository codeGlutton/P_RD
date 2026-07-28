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
    """투명 판을 아트 폴더로 옮기고 이름을 돌려준다.

    **여기서 덮어쓴다.** 시안에서 오려 낸 판을 같은 이름으로 아트 폴더에
    복사하므로, 갈아 끼운 판을 같은 이름으로 두면 다음 prepare 에 조용히
    사라진다. 실제로 턴 순서 판이 그렇게 되돌아갔다.

    갈아 끼울 때는 **다른 이름**으로 두고 hud04_plate_art.py 에 걸어라.
    """
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

#: 아군 칸 셋. 카드와 같은 까닭으로 한 벌로 맞춘다 -- 시안이 기사 줄에만
#: 상태이상을 그려 두어 셋의 요소 수부터 다르다.
PARTY = ("bottom_status_left", "bottom_status_center", "bottom_status_right")
PARTY_TEMPLATE = "bottom_status_left"

#: 본에 없는 것을 데려올 곳. 시안이 기사 줄(왼쪽)에만 상태이상과 차례 표시를
#: 그려 두어, 아군 3을 본으로 삼으면 그 둘이 없다.
PARTY_FALLBACK = ("bottom_status_left", "bottom_status_center")


def fill_missing(place, detail, plates, template, fallbacks):
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

    ## 덮지 않는다

    전에는 본의 값으로 통째로 덮었다. 그런데 구역 조정 쪽에도 "묶어 고치기"
    가 있어 같은 일을 두 곳에서 하게 됐고, 쪽에서 맞춘 값이 여기서 다시
    덮이면서 열 때마다 조금씩 밀렸다.

    맞추는 것은 쪽이 한다. 여기서는 **위젯이 사라지지 않게 빈 자리만 메운다**
    -- 굽는 코드는 없는 자리를 건너뛰므로, 어느 판에 요소가 빠져 있으면 그
    카드에서만 글자가 통째로 안 나온다.
    """
    def boxes(plate):
        left, top = place[plate][0], place[plate][1]
        return {element: [rect[0] - left, rect[1] - top, rect[2], rect[3]]
                for element, rect in detail[plate].items()}

    base = boxes(template)

    # 본에 없는 것은 다른 판에서 데려온다. 본을 덮지는 않는다.
    for spare in fallbacks:
        for element, rect in boxes(spare).items():
            base.setdefault(element, rect)

    # 가림막은 선택 테두리와 같은 자리를 덮는다. 덮는 것과 두르는 것이
    # 어긋나면 같은 카드가 상태에 따라 다른 크기로 보인다.
    # 가림막과 골라진 표시는 명령 카드만의 것이다.
    #
    # 가림막은 못 쓰는 카드를 덮고, 골라진 표시는 안 쓴다(고르는 순간 카드가
    # 비켜서 안 보인다). 아군 칸에는 가림막이 없고, 골라진 표시는 "지금 차례"
    # 라는 다른 뜻으로 늘 보인다.
    if template == CARD_TEMPLATE:
        if "cooldown_overlay" not in base and "selected_outline" in base:
            base["cooldown_overlay"] = list(base["selected_outline"])
        base.pop("selected_outline", None)
    else:
        base.pop("cooldown_overlay", None)

    # 아래 두 줄은 명령 카드에만 뜻이 있다. 자리만 남겨 두면 쪽에서 옮길 수
    # 있는 죽은 구역이 되어, 맞춰 놓아도 아무 데도 안 쓰인다.
    # 태세 글은 피해 글과 같은 칸이다. 한 칸이 둘을 같이 쓰는 일은 없다.
    if "damage_text" in base and "stance_text" not in base:
        base["stance_text"] = list(base["damage_text"])

    # 사람이 손댄 판은 건드리지 않는다.
    #
    # 빈 자리를 메우는 것은 **시안을 처음 잴 때** 쓰는 장치다. 시안이 카드마다
    # 다른 내용을 그려서 어떤 카드에는 피해가, 어떤 카드에는 쿨타임이 없기
    # 때문이다.
    #
    # 그런데 구역 조정 쪽에서 지운 것까지 도로 넣고 있었다. 지운 구역이 다음에
    # 열면 되살아나니, 지우는 기능이 있으나 마나였다. 손댄 판은 그 사람 말이
    # 맞다 -- 위젯이 사라지는 것도 그 사람이 정한 것이다.
    touched = set(TUNED_PLATES)

    added = []
    for plate in plates:
        if plate in touched:
            continue
        # 안 그리기로 한 것은 지운다. 메우기만 하고 지우지 않으면, 시안이 한
        # 장에만 그려 준 것이 그 판에만 남아 쪽에서 죽은 구역으로 보인다 --
        # 맞춰 놓아도 아무 데도 안 쓰인다.
        for gone in set(detail.get(plate, {})) - set(base):
            detail[plate].pop(gone, None)
        left, top = place[plate][0], place[plate][1]
        for element, rect in base.items():
            if element in detail.get(plate, {}):
                continue
            detail.setdefault(plate, {})[element] = [
                rect[0] + left, rect[1] + top, rect[2], rect[3]]
            added.append("%s.%s" % (plate, element))
    return added

#: 사람이 손댄 판. apply_tuning 이 채운다.
TUNED_PLATES = set()


def apply_tuning(place, detail):
    """손으로 맞춘 자리를 덮어씌운다.

    시안을 잰 값은 시안이 그린 그대로다 -- 비용 배지를 오른쪽 아래에 두느라
    글자와 아이콘이 죄다 왼쪽으로 밀려 있는 식이다. 그것을 눈으로 맞춘 결과가
    hud04_tuning.py 에 있다.

    잰 것과 정한 것을 갈라 둔다. 시안이 바뀌면 잰 값은 새로 나오고, 맞춘 값은
    그 위에 그대로 다시 얹힌다.

    적어 둔 값은 **판 안** 자리라 판 자리를 더해 화면 자리로 옮긴다. 미리
    더해 두면 판이 움직였을 때 따라오지 못한다.
    """
    try:
        from hud04_tuning import TUNING
    except ImportError:
        return 0

    # 판 크기부터 갈아 끼운다. 구역은 판 안 자리라 판이 먼저 정해져야 한다.
    #
    # 이 값은 hud04_tuning.py 가 아니라 따로 둔다. 그 파일은 apply_zones.py 가
    # 통째로 다시 쓰므로, 손으로 적어 두면 구역을 한 번 내려받아 넣는 순간
    # 사라진다 -- 턴 순서 판이 그렇게 되돌아가 있었다.
    try:
        from hud04_plate_size import PLATE_SIZE
    except ImportError:
        PLATE_SIZE = {}
    for plate, size in PLATE_SIZE.items():
        if plate not in place and len(size) == 4:
            # 시안에 없던 판이다. 넷을 다 적었으면 새로 세운다 -- 그림만 있고
            # 시안에 없는 것을 넣으려면 여기 말고는 세울 자리가 없다.
            place[plate] = list(size)
            continue
        if plate not in place:
            continue
        if len(size) == 4:
            place[plate] = list(size)
        else:
            place[plate] = [place[plate][0], place[plate][1], size[0], size[1]]

    moved = 0
    for plate, rows in TUNING.items():
        if plate not in place:
            continue
        TUNED_PLATES.add(plate)
        # 손댄 판은 적힌 것이 전부다. 안 적힌 요소는 지운 것이다.
        detail[plate] = {}
        left, top = place[plate][0], place[plate][1]
        for element, rect in rows.items():
            detail.setdefault(plate, {})[element] = [
                rect[0] + left, rect[1] + top, rect[2], rect[3]]
            moved += 1
    return moved


tuned = apply_tuning(place, detail)
if tuned:
    print("손으로 맞춘 자리 %d개를 덮었다" % tuned)

def same_size(place, plates, template):
    """묶인 판의 **크기**를 본에 맞춘다. 자리는 그대로 둔다.

    안쪽 구역만 맞추면 모자란다. 판 크기가 197x212, 196x218, 196x213 으로
    제각각이라, 같은 판 안 자리라도 화면에서는 카드마다 가장자리까지의 거리가
    달라진다 -- 비용 배지가 어느 카드에서는 테두리에 붙고 어느 카드에서는
    떠 보인다.

    자리(x, y)는 안 건드린다. 그것은 시안이 정한 배치이고, 여기서 맞추려는
    것은 "카드 한 장이 어떻게 생겼나" 다.
    """
    if template not in place:
        return 0
    _, _, width, height = place[template]
    moved = 0
    for plate in plates:
        if plate == template:
            continue
        x, y, w, h = place[plate]
        if (w, h) == (width, height):
            continue
        place[plate] = (x, y, width, height)
        moved += 1
    return moved


for group in ((CARDS, CARD_TEMPLATE), (PARTY, PARTY_TEMPLATE)):
    changed = same_size(place, group[0], group[1])
    if changed:
        print("판 %d장의 크기를 %s 에 맞췄다 (%dx%d)"
              % (changed, group[1], place[group[1]][2], place[group[1]][3]))

# 빈 자리 메우기가 마지막이다. 맞춘 값을 얹은 뒤에 해야, 쪽에서 새로 만든
# 구역이 있는 판은 그대로 두고 없는 판에만 들어간다.
for group in ((CARDS, CARD_TEMPLATE, CARD_FALLBACK),
              (PARTY, PARTY_TEMPLATE, PARTY_FALLBACK)):
    added = fill_missing(place, detail, *group)
    if added:
        print("빈 자리를 메웠다(%d): %s" % (len(added), ", ".join(added)))

# 시안에 없던 판은 갈아 끼운 그림이 곧 제 그림이다. 그것마저 없으면 진짜로
# 그릴 것이 없는 것이라 여기서 멈춘다.
try:
    from hud04_plate_art import PLATE_ART
except ImportError:
    PLATE_ART = {}
for plate in set(place) - set(textures):
    if plate in PLATE_ART:
        textures[plate] = PLATE_ART[plate]

missing = set(place) - set(textures)
if missing:
    raise RuntimeError("판은 있는데 그림이 없다: %s" % sorted(missing))

write_slots(place, detail, textures)
print("판 %d장, 자리 %d판 -> %s" % (len(textures), len(detail), SLOTS))
