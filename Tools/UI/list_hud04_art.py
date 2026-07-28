# -*- coding: utf-8 -*-
"""시안4 전투 HUD 의 판과 구역을 눈으로 보고 고치는 쪽을 만든다.

## 왜 또 만드나

list_layout_art.py 가 같은 일을 한다. 다만 그쪽은 **시안1** 을 본다 --
cutout_manifest.json 과 slot_table.SLOTS 를 읽고, 결과를 시안1 폴더에 쓴다.
지금 굽는 것은 시안4 이고 그 자리표는 hud04_slots.py 에 따로 있다.

쪽(art_list_page.html)은 그대로 쓴다. 자료를 박아 넣는 자리가 둘뿐이라
-- 목록과 자리표 -- 먹이는 것만 갈면 된다.

## 좌표를 옮겨 적는다

hud04_slots.DETAIL 은 **화면 기준**으로 적혀 있다. 아군 왼쪽 칸의 초상이
(31, 786) 인 식이다. 그런데 쪽은 판 하나를 따로 띄우고 그 **판 왼쪽 위를
원점**으로 구역을 그린다.

그래서 판 자리를 빼서 넘긴다. 안 빼면 구역이 전부 판 밖 오른쪽 아래로
날아간다.

    python list_hud04_art.py
"""
import base64
import io
from io import BytesIO

from PIL import Image
import json
import os
import shutil
import sys

from hud04_slots import DETAIL, PLACE, TEXTURE

try:
    from hud04_plate_art import PLATE_ART
except ImportError:
    PLATE_ART = {}

HERE = os.path.dirname(os.path.abspath(__file__))
ART = os.path.join(HERE, "KayKitUIKit", "HUD04")
# 폴더와 파일 이름은 영문으로 둔다.
#
# 한글로 두었더니 UnrealBuildTool 이 죽었다. git status 는 한글 경로를 따옴표로
# 감싸고 이스케이프해서 내놓는데(êµ¬...), UBT 의 적응형 빌드가 그
# 줄을 그대로 경로로 쓰다 "invalid directory separators" 로 터진다.
#
# 빌드가 안 되는 이유를 이 폴더에서 찾기까지 한참 걸렸다.
OUT = os.path.join(HERE, "_hud04_zones")

#: 판 이름 -> 사람이 읽는 이름. 쪽 목록에 뜬다.
LABEL = {
    "top_left_parchment": "라운드",
    "top_center_turn_order": "턴 순서",
    "top_right_parchment": "목표 (→ 메뉴 아이콘)",
    "upper_right_enemy_panel": "적 정보",
    # 명령 카드 여섯은 구역이 한 벌로 같다. 그러니 "위/왼위/오른아래" 로
    # 부르면 자리가 다른 만큼 구역도 다르다고 읽힌다. 번호만 붙인다.
    "action_top": "디폴트_1",
    "action_left_upper": "디폴트_2",
    "action_right_upper": "디폴트_3",
    "action_left_lower": "디폴트_4",
    "action_right_lower": "디폴트_5",
    "action_bottom": "디폴트_6",
    "bottom_status_left": "아군 1",
    "bottom_status_center": "아군 2",
    "bottom_status_right": "아군 3",
    "bottom_right_button": "턴 종료",
}

#: 한 벌로 맞춰 둔 판 묶음. 쪽에서 한 장만 고치면 나머지가 따라온다.
#:
#: prepare_hud04.py 가 이 여섯을 같은 값으로 통일해 둔다. 쪽에서 따로 고칠 수
#: 있게 두면 통일이 조용히 깨진다 -- 여섯 장이 조금씩 다른 것이 눈으로는 안
#: 보이고, 구운 뒤 카드마다 글자가 반 칸씩 어긋나서야 알게 된다.
FAMILY = [
    ["action_top", "action_left_upper", "action_right_upper",
     "action_left_lower", "action_right_lower", "action_bottom"],
    ["bottom_status_left", "bottom_status_center", "bottom_status_right"],
]

#: 묶음 안에서는 **첫 판의 그림**을 다 같이 쓴다.
#:
#: 굽는 쪽이 그렇게 한다 -- 카드 여섯이 action_top 한 장을, 아군 셋이
#: bottom_status_left 한 장을 나눠 쓴다. 그런데 쪽은 판마다 제 그림을 띄우고
#: 있어서, 자리를 다 맞춰 놓아도 배경이 달라 "안 맞았다" 로 보였다.
TEMPLATE_OF = {plate: group[0] for group in FAMILY for plate in group}

#: 목록에서 묶어 보여 줄 차례. 쓰임이 같은 것끼리 붙여야 견주기 쉽다.
GROUPS = (
    ("상단", "라운드 · 턴 순서 · 목표", (
        "top_left_parchment", "top_center_turn_order", "top_right_parchment")),
    ("명령 카드 6", "CommandCard_0~5", (
        "action_top", "action_left_upper", "action_right_upper",
        "action_left_lower", "action_right_lower", "action_bottom")),
    ("아군 3", "PartyCard_0~2", (
        "bottom_status_left", "bottom_status_center", "bottom_status_right")),
    ("그 밖", "적 정보 · 턴 종료 · AP 막대", (
        "upper_right_enemy_panel", "bottom_right_button",
        "bottom_center_ap_bar")),
)


def zone_art_data():
    """구역에 걸어 둔 그림을 쪽이 읽을 수 있게 data URL 로 싼다.

    쪽은 지금까지 **올린 것만** 알았다. 코드에서 걸어 둔 그림은 게임에는
    나오는데 쪽에서는 빈 칸이라, 무엇이 들어갈 자리인지 보고 맞출 수가
    없었다 -- AP 보석 열 칸이 특히 그랬다.

    @return "판|구역" -> {png, fit}
    """
    try:
        from hud04_zone_art import ZONE_ART
    except ImportError:
        return {}
    out = {}
    for plate, rows in ZONE_ART.items():
        for element, entry in rows.items():
            path = os.path.join(ART, "%s.png" % entry["texture"])
            if not os.path.exists(path):
                continue
            # 미리보기라 줄여서 싣는다. 원본 그대로 싣더니 쪽이 60MB 가
            # 됐다 -- 같은 보석이 열 칸에 열 번 들어간다.
            with Image.open(path) as image:
                image = image.convert("RGBA")
                image.thumbnail((192, 192))
                buffer = BytesIO()
                image.save(buffer, "PNG")
            out["%s|%s" % (plate, element)] = {
                "png": "data:image/png;base64,"
                       + base64.b64encode(buffer.getvalue()).decode(),
                "fit": entry.get("fit") or "contain",
                # 파일이 실어 준 것이라는 표. 다시 올라오면 apply_zones 가
                # 건너뛴다 -- 안 그러면 줄여 실은 미리보기가 원본을 덮는다.
                "seed": True,
            }
    return out


def local(plate):
    """화면 기준 자리를 판 안 자리로. 판 왼쪽 위를 뺀다."""
    ox, oy = PLACE[plate][0], PLACE[plate][1]
    moved = {}
    for element, rect in DETAIL.get(plate, {}).items():
        if not isinstance(rect, (list, tuple)) or len(rect) < 4:
            moved[element] = rect
            continue
        moved[element] = [rect[0] - ox, rect[1] - oy, rect[2], rect[3]]
    return moved


def main():
    sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding="utf-8")

    if os.path.isdir(OUT):
        shutil.rmtree(OUT)
    os.makedirs(os.path.join(OUT, "png"))

    table = {plate: local(plate) for plate in PLACE}

    # 목록에 안 적힌 판이 있으면 뒤에 붙인다.
    #
    # 판을 새로 세워 놓고 여기 줄을 안 늘리면, 자리는 만들어졌는데 쪽에서는
    # 그 판이 아예 안 보인다 -- AP 막대가 그랬다. 조용히 빠지느니 이름만
    # 붙어서라도 뜨는 편이 낫다.
    listed = {plate for _, _, plates in GROUPS for plate in plates}
    groups = list(GROUPS)
    rest = tuple(sorted(set(PLACE) - listed))
    if rest:
        groups.append(("목록에 없던 판", "새로 세운 것", rest))

    rows, missing = [], []
    for title, users, plates in groups:
        items = []
        for plate in plates:
            # 묶인 판은 본의 그림을 띄운다. 굽는 결과와 같은 것을 봐야 한다.
            #
            # 갈아 끼운 그림이 가장 앞선다. 안 그러면 쪽에서는 옛 판이 보이고
            # 게임에서는 새 그림이 나온다 -- 그 위에서 구역을 맞출 수가 없다.
            name = (PLATE_ART.get(plate)
                    or TEXTURE.get(TEMPLATE_OF.get(plate, plate)))
            src = os.path.join(ART, "%s.png" % name) if name else None
            x, y, w, h = PLACE[plate]
            note = "%s  %dx%d @ %d,%d" % (plate, w, h, x, y)
            if src is None or not os.path.exists(src):
                missing.append(plate)
                items.append({"name": LABEL.get(plate, plate),
                              "path": name or plate, "note": note,
                              "png": None, "role": plate})
                continue
            # 판 크기에 맞춰 줄여서 깐다.
            #
            # 그냥 복사하면 그림이 원본 크기로 깔린다. 시안에서 오려 낸 판은
            # 그림 크기와 판 크기가 같아 그래도 됐는데, 갈아 끼운 판은 다르다 --
            # 영웅 칸은 그림이 1354 인데 판은 292 라, 구역이 왼쪽 위 구석에
            # 오글오글 몰려 보였다.
            with Image.open(src) as image:
                image = image.convert("RGBA").resize((int(w), int(h)),
                                                     Image.LANCZOS)
                image.save(os.path.join(OUT, "png", name + ".png"))
            items.append({"name": LABEL.get(plate, plate), "path": name,
                          "note": note, "png": "png/%s.png" % name,
                          "role": plate})
        rows.append({"title": title, "users": users, "items": items})

    # 쪽에 자료를 박아 넣는다. file:// 에서는 같은 폴더 파일도 fetch 가 막혀
    # 있어 따로 두면 못 읽는다.
    page = io.open(os.path.join(HERE, "art_list_page.html"),
                   encoding="utf-8").read()
    page = (page
            .replace("/*DATA*/null", json.dumps(rows, ensure_ascii=False))
            .replace("/*TABLE*/null", json.dumps(table, ensure_ascii=False))
            .replace("/*FAMILY*/null", json.dumps(FAMILY, ensure_ascii=False))
            # 구역에 얹는 그림은 쪽에서 손으로 넣는다. 기본은 없음이다 --
            # 자동으로 얹어 두면 무엇이 진짜 들어갈 그림인지 알 수 없다.
            .replace("/*ZONEART*/null", json.dumps(zone_art_data()))
            # 저장 칸을 시안1 쪽과 갈라 둔다. 같이 쓰면 판 이름이 안 맞는
            # 표가 얹혀 구역이 아예 안 뜬다.
            .replace('/*KEEPKEY*/"rd.slots.01"', '"rd.hud04"')
            .replace("시안1 배치안이 쓰는 그림 · 구역 조정",
                     "시안4 전투 HUD · 구역 조정")
            .replace('\'    "01": {\'', '\'DETAIL = {\''))
    io.open(os.path.join(OUT, "zones.html"), "w",
            encoding="utf-8").write(page)

    for group in rows:
        print("%-12s %2d장  <- %s" % (group["title"], len(group["items"]),
                                      group["users"]))
    print("\n구역 %d개" % sum(len(v) for v in table.values()))

    if missing:
        print("그림 못 찾음:", ", ".join(missing))
    print("적었다:", os.path.join(OUT, "zones.html"))


main()
