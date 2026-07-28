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
import io
import json
import os
import shutil
import sys

from hud04_slots import DETAIL, PLACE, TEXTURE

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
FAMILY = [[
    "action_top", "action_left_upper", "action_right_upper",
    "action_left_lower", "action_right_lower", "action_bottom",
]]

#: 목록에서 묶어 보여 줄 차례. 쓰임이 같은 것끼리 붙여야 견주기 쉽다.
GROUPS = (
    ("상단", "라운드 · 턴 순서 · 목표", (
        "top_left_parchment", "top_center_turn_order", "top_right_parchment")),
    ("명령 카드 6", "CommandCard_0~5", (
        "action_top", "action_left_upper", "action_right_upper",
        "action_left_lower", "action_right_lower", "action_bottom")),
    ("아군 3", "PartyCard_0~2", (
        "bottom_status_left", "bottom_status_center", "bottom_status_right")),
    ("그 밖", "적 정보 · 턴 종료", (
        "upper_right_enemy_panel", "bottom_right_button")),
)


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

    rows, missing = [], []
    for title, users, plates in GROUPS:
        items = []
        for plate in plates:
            name = TEXTURE.get(plate)
            src = os.path.join(ART, "%s.png" % name) if name else None
            x, y, w, h = PLACE[plate]
            note = "%s  %dx%d @ %d,%d" % (plate, w, h, x, y)
            if src is None or not os.path.exists(src):
                missing.append(plate)
                items.append({"name": LABEL.get(plate, plate),
                              "path": name or plate, "note": note,
                              "png": None, "role": plate})
                continue
            shutil.copyfile(src, os.path.join(OUT, "png", name + ".png"))
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
            .replace("/*ZONEART*/null", "null")
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
