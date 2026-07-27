# -*- coding: utf-8 -*-
"""배치안이 쓰는 그림을 전부 모아 보고 고를 수 있는 쪽을 만든다.

## 왜

배치안 하나에 그림이 마흔 장 넘게 들어간다 -- 오려 낸 판 열여덟, 얼굴 아홉,
스킬 아이콘 여섯, 보석과 배지와 막대와 상태 표시. 어느 위젯이 어느 그림을
쓰는지 코드를 따라가야 알 수 있고, 그림을 갈아 끼우려면 그 목록이 먼저
있어야 한다.

## 무엇을 모으나

굽는 코드가 실제로 거는 경로를 그대로 읽는다 -- 명세(오려 낸 판)와
combat_layout_kit 의 상수(얼굴·아이콘·보석·막대). 눈으로 세지 않는다.

원본 PNG가 어디 있는지도 같이 찾는다. 엔진 에셋은 브라우저가 못 읽으므로,
쪽에서 보여 줄 그림은 임포트 전 PNG 쪽이다.

    python list_layout_art.py --mockup 1
"""
import argparse
import io
import json
import os
import shutil
import sys

from slot_table import SLOTS

CHROME = r"D:/UnrealProjects/P_RD_develop/Tools/UI/KayKitUIKit/Chrome"
KIT = r"D:/UnrealProjects/P_RD_develop/Tools/UI/KayKitUIKit"
OUT = r"D:/UnrealProjects/P_RD_develop/시안1/_아트목록"

#: 엔진 경로 앞머리 -> 원본 PNG 폴더. 쪽에 보여 줄 그림을 여기서 찾는다.
WHERE = (
    ("/Game/SVN/OutSideAsset/UI/KayKit/Cutouts/", os.path.join(KIT, "Cutouts")),
    ("/Game/SVN/OutSideAsset/UI/KayKit/Heads/", os.path.join(KIT, "Heads")),
    ("/Game/SVN/OutSideAsset/UI/KayKit/Chrome/", os.path.join(KIT, "Chrome")),
    ("/Game/SVN/OutSideAsset/UI/KayKit/Slices/", os.path.join(KIT, "Slices")),
    ("/Game/SVN/OutSideAsset/UI/KayKit/", os.path.join(KIT, "Processed")),
)

KK = "/Game/SVN/OutSideAsset/UI/KayKit"
HEADS = KK + "/Heads"

#: 판이 아닌 그림들. 굽는 코드가 거는 경로를 그대로 옮겨 적었다.
FIXED = [
    ("아군 초상", "PartyPortrait_0~2", [
        HEADS + "/KK_Face_Knight_HeadV2",
        HEADS + "/KK_Face_Ranger_HeadV2",
        HEADS + "/KK_Face_Mage_HeadV2"]),
    ("턴 순서 초상", "TurnPortrait_0~4", [
        HEADS + "/KK_Face_Knight_HeadV2",
        HEADS + "/KK_Face_Enemy_Eagle_HeadV2",
        HEADS + "/KK_Face_Ranger_HeadV2",
        HEADS + "/KK_Face_Enemy_Eagle_HeadV2",
        HEADS + "/KK_Face_Mage_HeadV2"]),
    ("적 초상", "EnemyPortrait", [HEADS + "/KK_Face_Enemy_Eagle_HeadV2"]),
    ("스킬 아이콘", "CommandIcon_0~5", [
        KK + "/KK_Icon_Move", KK + "/KK_Icon_BasicAttack",
        KK + "/KK_Icon_ShieldBash", KK + "/KK_Icon_PinSlash",
        KK + "/KK_Icon_Breakthrough", KK + "/KK_Icon_Riposte"]),
    ("값 배지", "CommandCostPlate_0~5", [KK + "/KK_Badge_Round"]),
    ("AP 보석", "PartyAPPip / PartyAPPipBg", [
        KK + "/KK_Gem_Blue_On", KK + "/KK_Gem_Blue_Off"]),
    ("체력 막대", "PartyHPBar / EnemyHPBar", [
        KK + "/KK_Bar_Link", KK + "/KK_Bar_Track_Link"]),
    ("목표 아이콘", "ObjectiveIcon", [KK + "/KK_Icon_Objective"]),
    ("상태 표시", "PartyStatusIcon / EnemyDefenseIcon / EnemyForecastIcon", [
        KK + "/KK_Tag_Poison", KK + "/KK_Tag_Defense", KK + "/KK_Tag_Damage",
        KK + "/KK_Tag_Cooldown"]),
    ("선택 표시", "PartySelected / CommandSelected / TurnCurrent", [
        KK + "/Chrome/KK_Chrome_SelectParty",
        KK + "/Chrome/KK_Chrome_SelectSkill",
        KK + "/Chrome/KK_Chrome_SelectTurn"]),
]

ROLE_LABEL = {"round": "라운드 판", "objective": "목표 현판",
              "turn": "턴 순서 칸", "party": "아군 줄",
              "skill": "스킬 카드", "enemy": "적 패널",
              "endturn": "턴 종료 버튼"}


def source_of(asset_path):
    """엔진 경로에 해당하는 원본 PNG. 없으면 None."""
    for prefix, folder in WHERE:
        if asset_path.startswith(prefix):
            name = asset_path[len(prefix):]
            if "/" in name:
                continue
            path = os.path.join(folder, name + ".png")
            if os.path.exists(path):
                return path
    return None


def main():
    sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding="utf-8")
    ap = argparse.ArgumentParser()
    ap.add_argument("--mockup", default="1")
    args = ap.parse_args()
    number = "%02d" % int(args.mockup)

    with io.open(os.path.join(CHROME, "cutout_manifest.json"),
                 encoding="utf-8") as handle:
        manifest = json.load(handle)

    # 판에는 그 위에 잡아 놓은 구역을 같이 달아 준다. 판만 보여 주면 어디에
    # 무엇이 오는지 알 수 없어, 그림을 갈 때 기준을 잡을 수가 없다.
    table = SLOTS.get(number, {})
    groups = []
    plates = []
    seen_role = {}
    for row in manifest[number]:
        seen_role[row["role"]] = seen_role.get(row["role"], -1) + 1
        for asset, _ax, _ay, _aw, _ah in row["arts"]:
            plates.append({
                "path": "%s/%s" % (
                    "/Game/SVN/OutSideAsset/UI/KayKit/Cutouts", asset),
                "note": ROLE_LABEL.get(row["role"], row["role"]),
                # 판마다 구역이 따로다. 판을 하나씩 생성해 안쪽이 몇 px 씩
                # 다르므로, 묶어 두면 각자 맞출 수가 없다.
                #
                # 표에 판별 키가 있으면 그것을, 없으면 역할 키를 가리킨다.
                # 한 장뿐인 판(적 정보)은 번호를 안 붙여 두었다.
                "role": ("%s_%d" % (row["role"], seen_role[row["role"]])
                         if ("%s_%d" % (row["role"],
                                        seen_role[row["role"]])) in table
                         else row["role"]),
            })
    groups.append(("오려 낸 판", "Chrome_* / *Plate_* 의 _Art0", plates))
    for title, users, paths in FIXED:
        groups.append((title, users,
                       [{"path": p, "note": "", "role": None} for p in paths]))

    if os.path.isdir(OUT):
        shutil.rmtree(OUT)
    os.makedirs(os.path.join(OUT, "png"))

    rows, missing = [], []
    for title, users, entries in groups:
        items = []
        for entry in entries:
            path, note, role = entry["path"], entry["note"], entry["role"]
            src = source_of(path)
            name = path.rsplit("/", 1)[-1]
            if src is None:
                missing.append(path)
                items.append({"name": name, "path": path, "note": note,
                              "png": None, "role": role})
                continue
            shutil.copyfile(src, os.path.join(OUT, "png", name + ".png"))
            items.append({"name": name, "path": path, "note": note,
                          "png": "png/%s.png" % name, "role": role})
        rows.append({"title": title, "users": users, "items": items})

    # 쪽에 자료를 박아 넣는다. 따로 두고 fetch 로 읽게 했더니 브라우저가
    # 막았다 -- file:// 에서는 같은 폴더 파일도 fetch 가 안 된다(그림 태그는
    # 된다). 자료가 몇 십 줄이라 박아 넣는 편이 낫다.
    page = io.open(os.path.join(os.path.dirname(os.path.abspath(__file__)),
                                "art_list_page.html"),
                   encoding="utf-8").read()
    payload = json.dumps(rows, ensure_ascii=False)
    with io.open(os.path.join(OUT, "아트목록.html"), "w",
                 encoding="utf-8") as handle:
        handle.write(page
                     .replace("/*DATA*/null", payload)
                     .replace("/*TABLE*/null",
                              json.dumps(table, ensure_ascii=False)))

    total = sum(len(g["items"]) for g in rows)
    for group in rows:
        print("%-12s %2d장  <- %s" % (group["title"], len(group["items"]),
                                      group["users"]))
    print()
    print("그림 %d장 -> %s" % (total, OUT))
    if missing:
        print("원본 PNG를 못 찾음 %d장:" % len(missing))
        for path in missing:
            print("   %s" % path)


if __name__ == "__main__":
    main()
