# -*- coding: utf-8 -*-
"""자리표와 역할표를 합쳐 배치안 생성기가 읽을 표 하나로 만든다.

## 세 갈래로 나뉜 이유

  자리   match_cutouts.py   조각을 시안 위에 밀어 보며 찾는다. 기계가 정확하다
  역할   cutout_roles.py    무엇인지. 눈으로 보고 적었다 -- 규칙으로는 안 됐다
  구멍   여기               초상과 카드가 뚫린 자리. 내용 좌표가 여기서 나온다

구멍이 중요하다. 조각을 통째로 놓으면 껍데기는 시안과 똑같아지지만, 그
안에 얼굴과 이름과 막대를 어디에 얹을지는 따로 정해야 한다. 뚫린 자리를
읽으면 그 좌표가 기계적으로 나온다 -- 눈대중으로 옮기지 않아도 된다.

    python classify_cutouts.py
"""
import io
import json
import os
import sys

import numpy as np
from PIL import Image
from scipy import ndimage

from cutout_roles import BAND_CONTENTS, MISSING, ROLES

CUTOUTS = (r"C:/Users/2009e/.codex/generated_images"
           r"/019fa031-cbf8-7d41-944b-2727570617e9")
CHROME = r"D:/UnrealProjects/P_RD_develop/Tools/UI/KayKitUIKit/Chrome"


def holes_of(path):
    """조각에 뚫린 자리. 초상과 카드와 막대 홈이 여기 걸린다.

    알파는 있는데 어두운 곳을 찾는다. 시안 아트는 판이 밝고 뚫린 자리가
    어두워서, 밝기 하나로 갈린다.
    """
    arr = np.asarray(Image.open(path).convert("RGBA"), dtype=float)
    rgb, alpha = arr[:, :, :3], arr[:, :, 3]
    solid = alpha > 200
    if solid.sum() < 200:
        return None, []
    mean = [int(v) for v in rgb[solid].mean(axis=0)]

    dark = solid & (rgb.mean(axis=2) < 95)
    dark = ndimage.binary_opening(dark, np.ones((3, 3)))
    labels, count = ndimage.label(dark)
    found = []
    if count:
        sizes = ndimage.sum(dark, labels, range(1, count + 1))
        for i in np.argsort(sizes)[::-1][:12]:
            if sizes[i] < 400:
                break
            ys, xs = np.where(labels == i + 1)
            found.append([int(xs.min()), int(ys.min()),
                          int(xs.max() - xs.min() + 1),
                          int(ys.max() - ys.min() + 1)])
    found.sort(key=lambda e: (e[1], e[0]))
    return mean, found


def content_boxes(shown_path, blank_path, crop=None):
    """글자 있는 판에서 빈 판을 빼면 내용이 있던 자리가 남는다.

    ## 왜 빼서 구하나

    판 안에 이름과 막대와 얼굴을 어디에 놓을지, 지금까지는 내가 비율로
    골랐다 -- `h * 0.42`, `w * 0.05` 같은 숫자다. 그 숫자는 어떤 검사도
    통과한 적이 없다. 판이 놓인 자리는 재면서 판 안은 한 번도 안 쟀고,
    그래서 6안 카드에서 이름이 가운데로 가고 아이콘이 구석에 작게 붙었다.

    두 판은 글자와 아이콘만 빼고 같다. 그러니 다른 곳이 곧 그것들이 있던
    자리다. 내가 고를 숫자가 없어진다.

    ## 무엇이 걸러지나

    판 가장자리에 닿은 덩어리는 버린다. 두 판을 오릴 때 몇 px 어긋나면
    테두리를 따라 얇은 띠가 남는데, 그건 내용이 아니다.
    """
    # crop 을 주면 채워진 시안 전체에서 그 칸만 잘라 쓴다. 명세에 자리가
    # 적혀 있는 경우라, 조각을 따로 오려 둘 필요가 없다.
    art = Image.open(shown_path).convert("RGBA")
    if crop:
        art = art.crop(crop)
    shown = np.asarray(art, dtype=float)
    blank = np.asarray(Image.open(blank_path).convert("RGBA"), dtype=float)
    if crop:
        # 잘라 낸 칸에는 알파가 없다(시안은 배경까지 그려져 있다). 빈 판의
        # 알파를 그대로 쓴다 -- 판 모양이 같으니 그것이 맞다.
        shown = shown.copy()
        edge = min(shown.shape[0], blank.shape[0])
        side = min(shown.shape[1], blank.shape[1])
        shown[:edge, :side, 3] = blank[:edge, :side, 3]
    h = min(shown.shape[0], blank.shape[0])
    w = min(shown.shape[1], blank.shape[1])
    if h < 24 or w < 24:
        return []

    gap = np.abs(shown[:h, :w, :3] - blank[:h, :w, :3]).mean(axis=2)
    solid = (shown[:h, :w, 3] > 200) & (blank[:h, :w, 3] > 200)

    # 가장자리를 안쪽으로 깎는다. 두 판을 따로 오려서 테두리가 몇 px 어긋나
    # 있고, 그 얇은 띠가 판을 빙 둘러 하나의 큰 고리로 잡힌다 -- 그게 판
    # 전체를 감싸는 상자가 되어 초상이 화면을 삼켰다.
    solid = ndimage.binary_erosion(solid, np.ones((9, 9)))
    mask = ndimage.binary_closing(solid & (gap > 34), np.ones((3, 5)))

    labels, count = ndimage.label(mask)
    boxes = []
    if count:
        # 몇 개까지 가져올지는 판 크기에 맞춘다. 열 개로 못 박아 두었더니
        # 밴드가 텅 비었다 -- 9안 아래 띠는 1655px 인데 상자가 0개였다.
        # 작은 카드는 여전히 서너 개면 되고, 큰 띠는 수십 개가 필요하다.
        keep = int(min(80, max(10, (w * h) / 4000.0)))
        floor = max(120, int(w * h / 900.0))
        sizes = ndimage.sum(mask, labels, range(1, count + 1))
        for i in np.argsort(sizes)[::-1][:keep]:
            if sizes[i] < floor:
                break
            ys, xs = np.where(labels == i + 1)
            x0, y0 = int(xs.min()), int(ys.min())
            bw, bh = int(xs.max()) - x0 + 1, int(ys.max()) - y0 + 1
            # 판 전체를 덮는 상자는 내용이 아니라 남은 테두리다.
            if bw > w * 0.92 and bh > h * 0.92:
                continue
            # 실오라기는 글자가 아니다. 깎고 남은 테두리 조각인데, 이걸
            # 이름 자리로 골라 6안 적 이름이 판 오른쪽 끝에 붙었다.
            if bw < 9 or bh < 9:
                continue
            boxes.append([x0, y0, bw, bh])
    # 다른 상자 둘 이상을 품은 상자는 그것들이 뭉친 것이다. 얼굴과 이름과
    # 막대가 붙어 있으면 하나로 이어져 판 절반을 덮는 상자가 나오는데,
    # 그걸 얼굴로 잡으면 초상이 글자를 가린다 -- 6안 적 패널이 그랬다.
    def holds(big, small):
        return (big[0] <= small[0] + 2 and big[1] <= small[1] + 2
                and big[0] + big[2] >= small[0] + small[2] - 2
                and big[1] + big[3] >= small[1] + small[3] - 2
                and big is not small)

    boxes = [b for b in boxes
             if sum(1 for other in boxes if holds(b, other)) < 2]
    boxes.sort(key=lambda e: (e[1], e[0]))
    return boxes


def main():
    sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding="utf-8")
    with io.open(os.path.join(CHROME, "cutout_places.json"),
                 encoding="utf-8") as handle:
        places = json.load(handle)

    out, tally = {}, {}
    for number in sorted(places, key=int):
        table = ROLES.get(number, {})
        rows = []
        for index, item in enumerate(places[number], 1):
            role = table.get(index)
            if role is None:
                # 표에 없는 번호. 자리표가 바뀌었다는 뜻이므로 조용히
                # 넘기지 않고 드러낸다 -- 번호가 밀리면 표 전체가 어긋난다.
                print("  !! 시안%s 조각 %d 가 역할표에 없음: %s"
                      % (number, index, item["file"]))
                continue
            if role == "skip":
                continue
            # 구멍은 글자 있는 조각에서 읽는다. 빈 판은 나뭇결이 고르게
            # 밝아 초상 자리가 어둡지 않다 -- 빈 판에서 읽었더니 구멍이
            # 하나도 안 잡혀 얼굴이 빠지고 턴 다섯 칸이 하나로 뭉쳤다.
            source = item["file"]
            mean, found = holes_of(os.path.join(
                CUTOUTS, "시안%d" % int(number), source))
            if mean is None:
                continue
            row = dict(item, index=index, role=role, colour=mean,
                       holes=found)
            if item.get("blank"):
                row["boxes"] = content_boxes(
                    os.path.join(CUTOUTS, "시안%d" % int(number),
                                 item["file"]),
                    os.path.join(CUTOUTS, "시안%d" % int(number),
                                 "텍스트_아이콘_제거", item["blank"]))
            if role == "band":
                row["contents"] = BAND_CONTENTS.get((number, index), [])
            rows.append(row)
            tally[role] = tally.get(role, 0) + 1
        out[number] = rows

        kinds = {}
        for row in rows:
            kinds[row["role"]] = kinds.get(row["role"], 0) + 1
        gap = MISSING.get(number)
        print("시안%s: %s%s" % (
            number,
            "  ".join("%s %d" % (k, v) for k, v in sorted(kinds.items())),
            ("   [빈 곳: %s]" % ", ".join(gap)) if gap else ""))

    path = os.path.join(CHROME, "cutout_roles.json")
    with io.open(path, "w", encoding="utf-8") as handle:
        json.dump(out, handle, ensure_ascii=False, indent=1)
    print()
    print("=== 합계 ===")
    for role, count in sorted(tally.items(), key=lambda e: -e[1]):
        print("  %-10s %d" % (role, count))
    print("표: %s" % path)


if __name__ == "__main__":
    main()
