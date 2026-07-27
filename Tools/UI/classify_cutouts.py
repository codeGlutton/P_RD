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
