# -*- coding: utf-8 -*-
"""마젠타를 깔고 뽑은 빈 시안에서 조각을 잘라 빈 판으로 쓴다.

## 왜

9안은 빈 판이 제구실을 못했다. 글자 있는 판에서 빼도 남는 것이 없어 내용
자리가 0개로 나왔고, 그래서 띠 안이 통째로 어림짐작으로 채워졌다.

마젠타를 깔고 다시 뽑은 빈 시안이 있으면 이야기가 달라진다. 칸 구조가
그림에 그대로 그려져 있고 -- 초상 구멍도 글자 홈도 판에 파여 있다 -- 배경만
지우면 곧바로 빈 판이 된다.

## 어떻게

마젠타에서 얼마나 먼지로 가른다. 정확히 한 색이 아니라 압축 때문에 조금씩
번져 있어서, 문턱을 두고 본다.

자를 자리는 자리표에서 읽는다. 글자 있는 조각이 놓인 그 자리를 그대로
잘라 내면 크기가 맞으므로, 나머지 배관이 손댈 것 없이 짝을 짓는다.

    python key_blank_mockup.py --mockup 9 --image <경로>
"""
import argparse
import io
import json
import os
import sys

import numpy as np
from PIL import Image

CUTOUTS = (r"C:/Users/2009e/.codex/generated_images"
           r"/019fa031-cbf8-7d41-944b-2727570617e9")
CHROME = r"D:/UnrealProjects/P_RD_develop/Tools/UI/KayKitUIKit/Chrome"
BLANK_DIR = "텍스트_아이콘_제거"

#: 이 색에서 이만큼 안쪽이면 배경으로 본다.
KEY = np.array([243.0, 8.0, 230.0])
REACH = 90.0


def unkey(image):
    """마젠타를 지워 알파로 만든다."""
    rgb = np.asarray(image.convert("RGB"), dtype=float)
    far = np.sqrt(((rgb - KEY) ** 2).sum(axis=2))
    alpha = np.where(far < REACH, 0, 255).astype(np.uint8)
    out = np.dstack([rgb.astype(np.uint8), alpha])
    return Image.fromarray(out, "RGBA")


def main():
    sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding="utf-8")
    ap = argparse.ArgumentParser()
    ap.add_argument("--mockup", required=True)
    ap.add_argument("--image", required=True)
    args = ap.parse_args()

    number = "%02d" % int(args.mockup)
    with io.open(os.path.join(CHROME, "cutout_places.json"),
                 encoding="utf-8") as handle:
        places = json.load(handle)
    rows = places.get(number)
    if not rows:
        raise SystemExit("자리표에 시안%s 가 없다" % number)

    clean = unkey(Image.open(args.image))
    folder = os.path.join(CUTOUTS, "시안%d" % int(args.mockup), BLANK_DIR)
    os.makedirs(folder, exist_ok=True)

    for index, item in enumerate(rows, 1):
        box = (item["x"], item["y"],
               item["x"] + item["w"], item["y"] + item["h"])
        part = clean.crop(box)
        solid = np.asarray(part)[:, :, 3] > 200
        if solid.mean() < 0.2:
            print("  건너뜀(대부분 배경): 조각 %d" % index)
            continue
        name = "keyed_%02d.png" % index
        part.save(os.path.join(folder, name))
        print("  조각 %d  %dx%d -> %s" % (index, item["w"], item["h"], name))
    print("시안%s 빈 판을 마젠타 원본에서 잘랐다" % number)


if __name__ == "__main__":
    main()
