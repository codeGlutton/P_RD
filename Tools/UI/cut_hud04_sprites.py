# -*- coding: utf-8 -*-
"""시안4 판에서 아이콘·초상·보석·선택 테두리를 떼어낸다.

## 어떻게 떼나

같은 판이 두 벌 있다. 하나는 비어 있고 하나는 내용이 들어차 있다.

    클롭_투명/*.png                    빈 판 (둘레 4픽셀 여유)
    individual_filled_generated/*.png  내용이 들어간 판 (여유 없음)

두 장을 겹쳐 다른 자리가 곧 얹힌 것이다. 색이나 모양으로 아이콘을 찾아내는
기계를 만들 필요가 없다 -- 무엇이 얹혔는지는 뺄셈이 이미 안다.

떼어낼 자리는 detail_coordinates 가 이름과 함께 알려 준다. 아이콘, 초상,
보석 묶음, 선택 테두리만 뗀다. 글자는 안 뗀다 -- 런타임이 다른 글자를 쓸
것이라 시안의 글자 그림은 쓸모가 없다.

## 원점이 둘이다

빈 판은 source_box 에서 사방 4픽셀 넓혀 잘렸고(crop_origin), 채워진 판은
source_box 그대로다. 둘을 같은 자리로 놓으려면 화면 좌표로 환산해야 한다.
이걸 안 맞추면 4픽셀 어긋난 채로 빼서, 아이콘 둘레에 유령 테두리가 남는다.

    python cut_hud04_sprites.py
"""
import io
import json
import os

import numpy as np
from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
SOURCE = r"D:/UnrealProjects/P_RD_develop/시안4"
BLANKS = os.path.join(SOURCE, "클롭_투명")
FILLED = os.path.join(SOURCE, "individual_filled_generated")
MANIFEST = os.path.join(SOURCE, "cropped_ui-v2", "manifest.json")
DETAIL = os.path.join(SOURCE, "detail_coordinates", "detail_coordinates.json")
OUT = os.path.join(HERE, "KayKitUIKit", "HUD04")

#: 떼어낼 것. 글자는 뺀다.
KINDS = ("icon", "portrait", "icon_group", "state_outline")

#: 다르다고 볼 문턱. 그림 생성이라 같은 자리도 한두 단계 흔들린다.
REACH = 26


def blank_by_stem():
    """빈 판을 이름으로 찾을 수 있게 모은다. 파일 앞의 번호는 뗀다."""
    found = {}
    for entry in sorted(os.listdir(BLANKS)):
        if not entry.lower().endswith(".png"):
            continue
        stem = os.path.splitext(entry)[0]
        found[stem.split("_", 1)[1] if stem[:2].isdigit() else stem] = \
            os.path.join(BLANKS, entry)
    return found


def cut(filled, blank, canvas_rect, filled_origin, blank_origin):
    """한 자리를 떼어 낸다. 빈 판과 다른 픽셀만 남긴다."""
    x, y, w, h = canvas_rect

    fx, fy = x - filled_origin[0], y - filled_origin[1]
    bx, by = x - blank_origin[0], y - blank_origin[1]
    if fx < 0 or fy < 0 or fx + w > filled.shape[1] or fy + h > filled.shape[0]:
        return None
    if bx < 0 or by < 0 or bx + w > blank.shape[1] or by + h > blank.shape[0]:
        return None

    patch = filled[fy:fy + h, fx:fx + w, :3].astype(float)
    base = blank[by:by + h, bx:bx + w, :3].astype(float)
    base_alpha = blank[by:by + h, bx:bx + w, 3].astype(float)

    # 빈 판이 비어 있던 자리는 그냥 다 살린다. 판 밖으로 삐져나온 그림이다.
    changed = np.abs(patch - base).sum(axis=2) > REACH
    changed |= base_alpha < 10

    if changed.mean() < 0.02:
        return None

    out = np.zeros((h, w, 4), dtype=np.uint8)
    out[..., :3] = patch.astype(np.uint8)
    out[..., 3] = np.where(changed, 255, 0).astype(np.uint8)
    return out


def trim(rgba):
    """투명한 둘레를 깎는다. 남겨 두면 자리 잡을 때 중심이 어긋난다."""
    rows = np.any(rgba[..., 3] > 0, axis=1)
    cols = np.any(rgba[..., 3] > 0, axis=0)
    if not rows.any() or not cols.any():
        return None
    top, bottom = np.nonzero(rows)[0][[0, -1]]
    left, right = np.nonzero(cols)[0][[0, -1]]
    return rgba[top:bottom + 1, left:right + 1], (int(left), int(top))


manifest = json.load(io.open(MANIFEST, encoding="utf-8"))
detail = json.load(io.open(DETAIL, encoding="utf-8"))
blanks = blank_by_stem()
origins = {os.path.splitext(os.path.basename(g["file"]))[0]: g["source_box"][:2]
           for g in manifest["groups"]}

os.makedirs(OUT, exist_ok=True)
offsets = {}
made = 0
for asset in detail["assets"]:
    stem = os.path.splitext(asset["file"])[0]
    filled_path = os.path.join(FILLED, asset["file"])
    if not os.path.exists(filled_path) or stem not in blanks:
        print("건너뜀(짝이 없음):", stem)
        continue

    filled = np.asarray(Image.open(filled_path).convert("RGB"))
    blank = np.asarray(Image.open(blanks[stem]).convert("RGBA"))

    for element in asset["elements"]:
        if element["type"] not in KINDS:
            continue
        piece = cut(filled, blank, element["canvas_bbox"],
                    origins[stem], asset["crop_origin"])
        if piece is None:
            print("건너뜀(다른 데가 없음): %s.%s" % (stem, element["id"]))
            continue
        trimmed = trim(piece)
        if trimmed is None:
            continue
        image, shift = trimmed
        name = "KK_HUD04_%s__%s" % (stem, element["id"])
        Image.fromarray(image).save(os.path.join(OUT, name + ".png"))
        # 깎은 만큼 자리도 옮겨야 한다. 깎기 전 상자에 그대로 놓으면 왼쪽 위로
        # 쏠린다.
        x, y, _, _ = element["canvas_bbox"]
        offsets[name] = (x + shift[0], y + shift[1],
                         image.shape[1], image.shape[0])
        made += 1

io.open(os.path.join(HERE, "hud04_sprites.py"), "w",
        encoding="utf-8", newline="\n").write(
    "# -*- coding: utf-8 -*-\n"
    '"""시안4 에서 떼어낸 스프라이트의 자리. cut_hud04_sprites.py 가 만든다.\n\n'
    "손으로 고치지 마라. 다시 뜨면 사라진다.\n"
    '"""\n\n#: 스프라이트 이름 -> 화면에서의 (x, y, w, h)\nSPRITE = {\n'
    + "".join('    "%s": %s,\n' % (k, offsets[k]) for k in sorted(offsets))
    + "}\n")
print("스프라이트 %d장" % made)
