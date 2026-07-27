# -*- coding: utf-8 -*-
"""전신 렌더에서 얼굴·어깨만 잘라 초상으로 만든다.

## 왜

포트레이트 원본은 전신이다. 그걸 초상 구멍에 그대로 넣으면 사람이 아주
작게 보인다 -- 캡처를 시안과 견주는 검사에서 턴 칸이 차 45~48로 가장 나쁘게
나왔고, 화면을 보니 큰 원 안에 전신이 콩알만 하게 들어 있었다.

시안의 초상은 얼굴과 어깨가 원을 꽉 채운다. 그러니 잘라야 한다. 자리를
아무리 맞춰도 그림이 작으면 닮을 수가 없다.

## 어떻게 자르나

알파로 인물이 차지한 칸을 구하고, 그 위쪽에서 정사각형을 딴다. 한 변은
인물 너비에 맞추되 머리가 잘리지 않게 위로 여유를 준다.

가로 중심은 인물 전체가 아니라 위쪽 3분의 1의 중심으로 잡는다. 팔을 옆으로
뻗은 자세(독수리는 날개를 편다)에서 전체 중심을 쓰면 얼굴이 한쪽으로
치우친다.

    python crop_portraits.py
"""
import io
import os
import sys
import glob

import numpy as np
from PIL import Image

SOURCE = (r"D:/UnrealProjects/P_RD_develop_20260726"
          r"/SourceArt/UI/KayKitCharacterPortraitsV2/Processed")
DEST = r"D:/UnrealProjects/P_RD_develop/Tools/UI/KayKitUIKit/Heads"

#: 잘라 낼 정사각형 한 변 = 인물 너비 * 이 값.
#:
#: 1.0 이면 어깨가 딱 맞고 얼굴이 커진다. 시안 초상은 어깨가 조금 남으므로
#: 살짝 넓게 딴다.
SPAN = 1.15

#: 머리 위 여유. 정사각형 위쪽을 인물 꼭대기보다 이만큼 올린다.
HEAD_ROOM = 0.08

OUT_SIZE = 256


def crop(path):
    """전신 그림에서 얼굴·어깨 정사각형."""
    art = Image.open(path).convert("RGBA")
    alpha = np.asarray(art)[:, :, 3]
    ys, xs = np.where(alpha > 24)
    if len(xs) < 50:
        return None

    top, bottom = int(ys.min()), int(ys.max())
    left, right = int(xs.min()), int(xs.max())
    body_w = right - left + 1
    body_h = bottom - top + 1

    # 위쪽 3분의 1의 가로 중심. 날개나 팔을 편 자세에서 얼굴이 치우치지
    # 않게 한다.
    head_band = ys < top + body_h / 3.0
    cx = float(xs[head_band].mean()) if head_band.any() else (left + right) / 2.0

    side = min(body_w * SPAN, art.size[0], art.size[1])
    x0 = cx - side / 2.0
    y0 = top - side * HEAD_ROOM
    x0 = min(max(x0, 0), art.size[0] - side)
    y0 = min(max(y0, 0), art.size[1] - side)

    box = (int(round(x0)), int(round(y0)),
           int(round(x0 + side)), int(round(y0 + side)))
    return art.crop(box).resize((OUT_SIZE, OUT_SIZE), Image.LANCZOS)


def main():
    sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding="utf-8")
    files = sorted(glob.glob(os.path.join(SOURCE, "*.png")))
    if not files:
        raise SystemExit("원본 없음: %s" % SOURCE)
    os.makedirs(DEST, exist_ok=True)

    sheet = Image.new("RGBA", (OUT_SIZE * 6, OUT_SIZE * 4), (24, 26, 32, 255))
    made = 0
    for path in files:
        head = crop(path)
        if head is None:
            print("  건너뜀(빈 그림): %s" % os.path.basename(path))
            continue
        stem = os.path.splitext(os.path.basename(path))[0]
        name = stem.replace("_DynamicV2", "_HeadV2")
        head.save(os.path.join(DEST, name + ".png"))
        sheet.paste(head, ((made % 6) * OUT_SIZE, (made // 6) * OUT_SIZE),
                    head)
        made += 1

    sheet.save(os.path.join(DEST, "contact_sheet.png"))
    print("얼굴 %d장 -> %s" % (made, DEST))


if __name__ == "__main__":
    main()
