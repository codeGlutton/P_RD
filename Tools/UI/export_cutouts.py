# -*- coding: utf-8 -*-
"""오려 낸 조각을 엔진이 읽을 이름으로 옮기고 배치 명세를 뽑는다.

## 왜 옮기나

조각은 생성기가 뱉은 폴더에 `call_YIoH5m4kbn6flboEN4y3uBdp_cutouts_04.png`
같은 이름으로 있다. 이 이름은 아무것도 알려 주지 않고, 다시 뽑으면 바뀐다.
에셋 이름이 바뀌면 배치안이 통째로 깨지므로, 시안 번호와 역할로 이름을
다시 짓는다 -- `KK_Cut_01_Skill` 처럼.

## 명세에 무엇이 담기나

  asset   엔진 에셋 이름
  rect    시안 1672x941 에서의 자리. 늘리지 않고 이 자리에 그대로 놓는다
  anchor  화면이 넓거나 좁아질 때 붙어 있을 모서리. 자리에서 정한다
  holes   조각에 뚫린 자리. 얼굴과 카드와 막대가 여기 들어간다

앵커는 조각이 놓인 곳으로 정한다. 왼쪽 것은 왼쪽에, 가운데 것은 가운데에
붙어야 화면이 넓어질 때 구역이 벌어지고 좁아질 때 겹치지 않는다.

    python export_cutouts.py
"""
import io
import json
import os
import shutil
import sys

CUTOUTS = (r"C:/Users/2009e/.codex/generated_images"
           r"/019fa031-cbf8-7d41-944b-2727570617e9")
CHROME = r"D:/UnrealProjects/P_RD_develop/Tools/UI/KayKitUIKit/Chrome"
DEST = r"D:/UnrealProjects/P_RD_develop/Tools/UI/KayKitUIKit/Cutouts"

SCREEN_W, SCREEN_H = 1672.0, 941.0

#: 역할 -> 에셋 이름에 쓸 말
WORD = {"round": "Round", "objective": "Objective", "turn": "Turn",
        "party": "Party", "skill": "Skill", "enemy": "Enemy",
        "endturn": "EndTurn", "band": "Band"}


def anchor_of(x, y, w, h):
    """조각이 붙어 있을 모서리. 조각 한가운데가 화면 어디냐로 정한다.

    화면 너비의 3분의 1보다 왼쪽이면 왼쪽에 붙이고, 3분의 2보다 오른쪽이면
    오른쪽에 붙인다. 나머지는 가운데다. 세로도 같다.

    가운데 3분의 1을 넓게 잡은 이유가 있다. 좁게 잡으면 상단 중앙의 턴
    순서처럼 화면 폭을 크게 차지하는 판이 왼쪽으로 판정되어, 넓은 화면에서
    가운데를 벗어난다.
    """
    cx, cy = (x + w / 2.0) / SCREEN_W, (y + h / 2.0) / SCREEN_H
    across = "l" if cx < 1 / 3.0 else ("r" if cx > 2 / 3.0 else "c")
    down = "t" if cy < 1 / 3.0 else ("b" if cy > 2 / 3.0 else "m")
    if down == "m":
        # 화면 한가운데 높이에 걸린 것은 위아래로 늘어난 판이다. 위에 붙인다
        # -- 세로가 짧아질 때 위에서부터 보이는 편이 낫다.
        down = "t"
    return {"l": "l", "c": "c", "r": "r"}[across], down


def reading_order(rows):
    """한 줄에 늘어선 것끼리는 왼쪽부터 센다.

    자리표는 y 로 먼저 정렬돼 있는데, 한 줄에 놓인 카드도 1~2px 씩 어긋나
    있어 y 가 작은 것이 먼저 온다. 2안에서 세 번째 카드(y=711)가 첫 칸
    (y=712)보다 앞서 매겨졌고, 그래서 "이동" 글자가 방패강타 판에 얹혔다.

    y 가 판 높이의 절반 안에 들면 같은 줄로 보고 x 로 다시 센다.
    """
    ordered, pool = [], sorted(rows, key=lambda r: (r["y"], r["x"]))
    while pool:
        head = pool.pop(0)
        band = [head]
        limit = head["y"] + max(head["h"] * 0.5, 12)
        for other in list(pool):
            if other["y"] <= limit and other["role"] == head["role"]:
                band.append(other)
                pool.remove(other)
        ordered.extend(sorted(band, key=lambda r: r["x"]))
    return ordered


def main():
    sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding="utf-8")
    with io.open(os.path.join(CHROME, "cutout_roles.json"),
                 encoding="utf-8") as handle:
        table = json.load(handle)

    if os.path.isdir(DEST):
        # 이름이 역할과 순번으로 정해지므로, 조각이 빠지면 옛 이름이 남아
        # 다음 굽기에서 엉뚱한 그림이 붙는다. 매번 비우고 다시 채운다.
        shutil.rmtree(DEST)
    os.makedirs(DEST)

    manifest, total = {}, 0
    for number in sorted(table, key=int):
        seen, rows = {}, []
        for row in reading_order(table[number]):
            role = row["role"]
            ordinal = seen.get(role, 0)
            seen[role] = ordinal + 1
            asset = "KK_Cut_%s_%s%s" % (
                number, WORD[role],
                "" if role in ("round", "objective", "endturn") and ordinal == 0
                else "_%d" % ordinal)
            # 글자 없는 판이 있으면 그것을 내보낸다. 자리는 글자 있는
            # 조각으로 찾았고 그림만 바꾼다.
            src = os.path.join(CUTOUTS, "시안%d" % int(number),
                               row.get("blank") or row["file"])
            shutil.copyfile(src, os.path.join(DEST, asset + ".png"))

            across, down = anchor_of(row["x"], row["y"], row["w"], row["h"])
            rows.append({
                "asset": asset, "role": role, "index": row["index"],
                "rect": [row["x"], row["y"], row["w"], row["h"]],
                "anchor": down + across, "holes": row["holes"],
                "contents": row.get("contents", []),
            })
            total += 1
        manifest[number] = rows
        print("시안%s: %2d장" % (number, len(rows)))

    path = os.path.join(CHROME, "cutout_manifest.json")
    with io.open(path, "w", encoding="utf-8") as handle:
        json.dump(manifest, handle, ensure_ascii=False, indent=1)
    print()
    print("조각 %d장 -> %s" % (total, DEST))
    print("명세: %s" % path)


if __name__ == "__main__":
    main()
