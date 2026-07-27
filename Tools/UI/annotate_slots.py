# -*- coding: utf-8 -*-
"""판 안에서 뽑아 낸 내용 자리에 번호를 그려 눈으로 이름 붙일 수 있게 한다.

## 왜

채워진 시안에서 빈 판을 빼면 글자와 얼굴이 있던 자리가 나온다. 자리는
정확하다. 그런데 그중 어느 것이 이름이고 어느 것이 체력 막대인지는 아직
생김새로 짐작하고 있다 -- 가로로 길면 막대, 네모나면 얼굴.

그 짐작이 판마다 어긋난다. 아군 줄마다 AP 보석 크기가 달라지고, 적 초상
자리가 비고, 카드 글자 간격이 들쭉날쭉하다.

역할을 정할 때도 같은 벽에 부딪혔고, 그때는 시안에 번호를 그려 눈으로 보고
표에 적어 해결했다. 여기서도 같다. 판 종류는 대여섯 가지뿐이고 나머지는
그것의 되풀이라, 한 장씩만 보면 된다.

    python annotate_slots.py
"""
import io
import json
import os
import sys

from PIL import Image, ImageDraw

CHROME = r"D:/UnrealProjects/P_RD_develop/Tools/UI/KayKitUIKit/Chrome"
MOCKUPS = r"D:/UnrealProjects/P_RD_develop/Tools/UI/KayKitHUDMockups/Raw"
OUT = r"D:/UnrealProjects/P_RD_develop/Tools/UI/KayKitHUDMockups/Slots"

#: 한 시안에서 종류마다 한 장씩만 본다. 나머지는 같은 생김새의 되풀이다.
ONE_OF_EACH = ("round", "objective", "turn", "party", "skill", "enemy",
               "endturn")

ZOOM = 3


def main():
    sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding="utf-8")
    number = sys.argv[1] if len(sys.argv) > 1 else "01"
    with io.open(os.path.join(CHROME, "cutout_manifest.json"),
                 encoding="utf-8") as handle:
        manifest = json.load(handle)

    mock = Image.open(os.path.join(
        MOCKUPS, "KK_HUD_Polish_%s.png" % number)).convert("RGB")
    os.makedirs(OUT, exist_ok=True)

    seen = set()
    for row in manifest[number]:
        role = row["role"]
        if role in seen or role not in ONE_OF_EACH:
            continue
        seen.add(role)

        x, y, w, h = row["rect"]
        crop = mock.crop((x, y, x + w, y + h))
        crop = crop.resize((w * ZOOM, h * ZOOM), Image.NEAREST)
        draw = ImageDraw.Draw(crop)
        for index, (bx, by, bw, bh) in enumerate(row["boxes"], 1):
            box = (bx * ZOOM, by * ZOOM,
                   (bx + bw) * ZOOM - 1, (by + bh) * ZOOM - 1)
            draw.rectangle(box, outline=(0, 255, 255), width=3)
            label = str(index)
            tx, ty = bx * ZOOM + 4, by * ZOOM + 4
            draw.rectangle((tx - 3, ty - 3, tx + 10 * len(label) + 3, ty + 16),
                           fill=(255, 255, 255))
            draw.text((tx, ty), label, fill=(0, 0, 0))

        path = os.path.join(OUT, "%s_%s.png" % (number, role))
        crop.save(path)
        print("%-10s %3dx%-3d  자리 %2d개 -> %s" % (
            role, w, h, len(row["boxes"]), os.path.basename(path)))


if __name__ == "__main__":
    main()
