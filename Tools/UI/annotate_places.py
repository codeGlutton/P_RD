# -*- coding: utf-8 -*-
"""자리를 찾은 조각에 번호를 그려 시안 위에 얹는다.

## 왜

조각이 무엇인지 색과 크기로 판정하려 했더니, 한 시안을 맞추면 다른 시안이
틀어졌다 -- 1안은 아군판이 적 패널로 잡히고 4안은 아래가 전부 아군으로
잡혔다. 스무 장이 서로 다르게 생겼으므로 규칙 하나로 덮이지 않는다.

자리는 이미 기계가 정확히 찾았다. 남은 것은 이름뿐이고, 이름은 보면 안다.
번호를 그려 두면 한 장을 한 번 보고 열 개를 한꺼번에 적을 수 있다.

    python annotate_places.py
"""
import io
import json
import os
import sys

from PIL import Image, ImageDraw

CHROME = r"D:/UnrealProjects/P_RD_develop/Tools/UI/KayKitUIKit/Chrome"
MOCKUPS = r"D:/UnrealProjects/P_RD_develop/Tools/UI/KayKitHUDMockups/Raw"
OUT = r"D:/UnrealProjects/P_RD_develop/Tools/UI/KayKitHUDMockups/Annotated"


def main():
    sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding="utf-8")
    with io.open(os.path.join(CHROME, "cutout_places.json"),
                 encoding="utf-8") as handle:
        places = json.load(handle)
    os.makedirs(OUT, exist_ok=True)

    for number in sorted(places, key=int):
        path = os.path.join(MOCKUPS, "KK_HUD_Polish_%02d.png" % int(number))
        image = Image.open(path).convert("RGB")
        draw = ImageDraw.Draw(image)
        for index, item in enumerate(places[number], 1):
            box = (item["x"], item["y"],
                   item["x"] + item["w"] - 1, item["y"] + item["h"] - 1)
            draw.rectangle(box, outline=(0, 255, 255), width=4)
            # 번호는 흰 바탕에 검게. 시안이 어둡고 밝은 곳이 섞여 있어
            # 글자만 얹으면 반쯤 안 보인다.
            label = str(index)
            tx, ty = item["x"] + 6, item["y"] + 6
            draw.rectangle((tx - 3, ty - 3, tx + 9 * len(label) + 3, ty + 20),
                           fill=(255, 255, 255))
            draw.text((tx, ty), label, fill=(0, 0, 0))
        out = os.path.join(OUT, "%02d.png" % int(number))
        image.save(out)
        print("시안%s: 조각 %d개 -> %s" % (number, len(places[number]), out))


if __name__ == "__main__":
    main()
