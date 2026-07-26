# -*- coding: utf-8 -*-
"""시안에서 완성된 판을 9슬라이스 부품으로 뜬다.

## 왜 이 방식인가

조각을 따로 만들어 이어 붙이는 방식으로는 손그림 품질에 못 닿았다. 프레임
직선 구간의 진행 방향 요동이 0.02~0.25 인데 시안은 9.67~12.50 이다 -- 몰딩이
없어 길게 뽑은 플라스틱 막대로 읽힌다. 조각을 아무리 다듬어도 이음과 반복이
남는다.

시안은 이미 완성 품질이다. 그걸 부품으로 만들면 품질이 보장된다.

## 어떻게

판 하나를 통째로 뜨고, BOX 브러시로 쓴다. 네 모서리는 원본 그대로 남고
가운데만 늘어난다 -- 몰딩·리벳·베벨이 그린 그대로 살아 있다.

문제는 시안 판에 내용(초상·글자·바)이 그려져 있다는 것이다. 가운데를 그대로
늘리면 글자가 번진다. 그래서 테두리(프레임)만 남기고 안쪽은 그 판의 빈
자리에서 뜬 면으로 덮는다.

## 5차-B 실패와 다른 점

그때는 코덱스가 마젠타 크로마로 칠한 '빈 판'을 만들고 내가 그걸 잘랐다.
크로마 헤일로가 4px 번져 나무 레일에 보라 징이 남았다. 여기서는 크로마가
없다 -- 완성 그림에서 직접 뜬다.
"""
import io
import os
import sys

import numpy as np
from PIL import Image

sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding="utf-8")
MOCK = r"D:/UnrealProjects/P_RD_develop/Tools/UI/KayKitHUDMockups/Raw"
OUT = r"D:/UnrealProjects/P_RD_develop/Tools/UI/KayKitUIKit/Slices"

#: (이름, 시안 번호, 판 상자, 프레임 두께)
#:
#: 판 상자와 프레임 두께는 눈으로 확인하고 적었다. 면을 뜰 자리는 적지 않는다
#: -- 내가 고르면 글자가 섞이고, 그걸 반복해 채우면 판 전체에 글자가 깔린다.
#: 실제로 그렇게 나와서 자동으로 찾도록 바꿨다.
TARGETS = [
    ("Slice_Card_Stone", "01", (466, 615, 628, 923), 22),
    # 나무 카드는 초상 링과 이름이 위 20px 안에 걸린다. 프레임을 12 로
    # 줄여 그만큼 더 덮는다 -- 남는 12px 은 순수 레일이다.
    ("Slice_Card_Wood",  "01", (14, 816, 467, 923),  12),
    ("Slice_Panel_Red",  "01", (1357, 616, 1664, 814), 22),
    ("Slice_Plate_Parch", "01", (13, 10, 261, 119),   20),
    ("Slice_Button",     "01", (1345, 812, 1664, 912), 18),
]


def quietest(panel, frame, side=26):
    """판 안에서 가장 조용한 정사각 자리. 글자·아이콘이 없는 순수 면이다.

    윤곽 에너지가 가장 낮은 곳을 고른다. 손으로 고르면 반드시 글자가 섞인다.
    """
    grey = panel.astype(float).mean(axis=2)
    gx = np.abs(np.diff(grey, axis=1, append=grey[:, -1:]))
    gy = np.abs(np.diff(grey, axis=0, append=grey[-1:, :]))
    energy = gx + gy
    h, w = grey.shape
    lo, hi_y, hi_x = frame + 2, h - frame - 2, w - frame - 2
    best, pos = None, None
    step = 4
    for y in range(lo, hi_y - side, step):
        for x in range(lo, hi_x - side, step):
            score = float(energy[y:y + side, x:x + side].mean())
            if best is None or score < best:
                best, pos = score, (x, y)
    if pos is None:
        return None, None
    x, y = pos
    return panel[y:y + side, x:x + side], best


def build(name, number, box, frame):
    src = Image.open(os.path.join(MOCK, "KK_HUD_Polish_%s.png" % number))
    panel = np.array(src.convert("RGB").crop(box)).astype(np.uint8)
    h, w = panel.shape[:2]
    patch, energy = quietest(panel, frame)
    if patch is None:
        print("  %-20s 조용한 자리를 못 찾음" % name)
        return None

    # 안쪽을 그 판의 면으로 덮는다. 좌우·상하를 뒤집어 이어 붙여 반복 무늬가
    # 눈에 띄지 않게 한다 -- 같은 패치를 그냥 깔면 격자가 읽힌다.
    out = panel.copy()
    block = np.concatenate([patch, patch[:, ::-1]], axis=1)
    block = np.concatenate([block, block[::-1, :]], axis=0)
    ph, pw = block.shape[:2]
    inner = out[frame:h - frame, frame:w - frame]
    ih, iw = inner.shape[:2]
    tiled = np.tile(block, (ih // ph + 2, iw // pw + 2, 1))[:ih, :iw]
    # 프레임과 맞닿는 몇 px 은 원본을 남겨 이음이 튀지 않게 한다.
    feather = 4
    mask = np.ones((ih, iw, 1), dtype=float)
    for i in range(feather):
        v = (i + 1) / float(feather + 1)
        mask[i, :] = np.minimum(mask[i, :], v)
        mask[ih - 1 - i, :] = np.minimum(mask[ih - 1 - i, :], v)
        mask[:, i] = np.minimum(mask[:, i], v)
        mask[:, iw - 1 - i] = np.minimum(mask[:, iw - 1 - i], v)
    blended = inner * (1 - mask) + tiled * mask
    out[frame:h - frame, frame:w - frame] = blended.astype(np.uint8)

    # 어느 변의 띠에 내용이 걸려 있으면 반대쪽 깨끗한 띠를 뒤집어 덮는다.
    #
    # 아군 카드가 그랬다 -- 초상 링과 이름이 위 12px 안까지 들어와, 프레임을
    # 아무리 줄여도 깨끗한 위 레일이 안 나온다. 판의 위아래 레일은 같은 아트를
    # 뒤집어 그린 것이므로 아래를 뒤집어 쓰면 원본과 다르지 않다.
    def energy_of(band):
        g = band.astype(float).mean(axis=2)
        return float((np.abs(np.diff(g, axis=1, append=g[:, -1:]))
                      + np.abs(np.diff(g, axis=0, append=g[-1:, :]))).mean())

    top, bot = out[:frame], out[h - frame:]
    et, eb = energy_of(top), energy_of(bot)
    if et > eb * 1.15:
        out[:frame] = bot[::-1]
        print("     위 레일을 아래에서 뒤집어 덮음 (에너지 %.1f -> %.1f)" % (et, eb))
    elif eb > et * 1.15:
        out[h - frame:] = top[::-1]
        print("     아래 레일을 위에서 뒤집어 덮음 (에너지 %.1f -> %.1f)" % (eb, et))
    left, right = out[:, :frame], out[:, w - frame:]
    el, er = energy_of(left), energy_of(right)
    if el > er * 1.15:
        out[:, :frame] = right[:, ::-1]
        print("     왼쪽 레일을 오른쪽에서 뒤집어 덮음 (%.1f -> %.1f)" % (el, er))
    elif er > el * 1.15:
        out[:, w - frame:] = left[:, ::-1]
        print("     오른쪽 레일을 왼쪽에서 뒤집어 덮음 (%.1f -> %.1f)" % (er, el))

    os.makedirs(OUT, exist_ok=True)
    Image.fromarray(out).save(os.path.join(OUT, name + ".png"))
    margin = frame / float(min(w, h))
    print("  %-20s %3dx%-3d  프레임 %2dpx  BOX 여백 %.4f  면 에너지 %.2f"
          % (name, w, h, frame, margin, energy))
    return name, out, frame


def build_wood_from_parch():
    """아군 카드는 양피지 판의 나무 프레임을 빌리고 안쪽만 나무로 채운다.

    파티 행은 초상 링과 이름이 프레임 띠 안까지 들어와, 프레임을 아무리
    줄이거나 반대쪽 레일을 뒤집어 덮어도 깨끗한 위 레일이 나오지 않았다.
    시안의 양피지 판(ROUND 1)은 같은 나무 프레임에 금속 모서리를 쓰므로
    프레임 언어가 어긋나지 않는다. 안쪽 면만 파티 행의 나무에서 뜬다.
    """
    src = Image.open(os.path.join(MOCK, "KK_HUD_Polish_01.png")).convert("RGB")
    frame_panel = np.array(src.crop((13, 10, 261, 119))).astype(np.uint8)
    party = np.array(src.crop((20, 819, 461, 920))).astype(np.uint8)
    patch, energy = quietest(party, 12)
    if patch is None:
        print("  Slice_Card_Wood 나무 면을 못 찾음")
        return None
    frame = 20
    out = frame_panel.copy()
    h, w = out.shape[:2]
    block = np.concatenate([patch, patch[:, ::-1]], axis=1)
    block = np.concatenate([block, block[::-1, :]], axis=0)
    ph, pw = block.shape[:2]
    ih, iw = h - 2 * frame, w - 2 * frame
    tiled = np.tile(block, (ih // ph + 2, iw // pw + 2, 1))[:ih, :iw]
    # 여기서는 깃털을 두지 않는다. 빌려 온 프레임의 안쪽이 양피지라, 조금만
    # 남겨도 나무 판 아래에 크림색이 비친다.
    out[frame:h - frame, frame:w - frame] = tiled
    os.makedirs(OUT, exist_ok=True)
    Image.fromarray(out).save(os.path.join(OUT, "Slice_Card_Wood.png"))
    print("  %-20s %3dx%-3d  프레임 %2dpx  BOX 여백 %.4f  (양피지 프레임 + 나무 면)"
          % ("Slice_Card_Wood", w, h, frame, frame / float(min(w, h))))
    return "Slice_Card_Wood", out, frame


rows = []
for target in TARGETS:
    if target[0] == "Slice_Card_Wood":
        continue
    got = build(*target)
    if got:
        rows.append(got)
got = build_wood_from_parch()
if got:
    rows.insert(1, got)

# 확인 시트: 원본 그대로 + 두 배로 늘린 것. 늘려서 깨지면 여백이 틀린 것이다.
if rows:
    from PIL import ImageDraw
    wmax = max(max(a.shape[1], a.shape[1] * 2) for _, a, _ in rows)
    htot = sum(a.shape[0] * 2 + 26 for _, a, _ in rows) + 8
    sheet = Image.new("RGB", (wmax + 12, htot), (40, 40, 46))
    draw = ImageDraw.Draw(sheet)
    y = 4
    for name, arr, frame in rows:
        draw.text((6, y), name, fill=(240, 240, 245))
        y += 15
        im = Image.fromarray(arr)
        sheet.paste(im, (6, y))
        # 폭만 두 배로 늘려 붙인다 (BOX 브러시가 하는 일과 같다)
        wide = im.resize((im.width * 2, im.height), Image.LANCZOS)
        sheet.paste(wide.crop((0, 0, min(wide.width, wmax), wide.height)),
                    (6, y + im.height + 3))
        y += im.height * 2 + 8
    sheet.save("slice_result.png")
    print("확인 시트: slice_result.png %s" % (sheet.size,))

# 여백 값을 표로 남긴다. 손으로 옮겨 적으면 슬라이스를 다시 뜰 때마다 틀린다
# -- 이 작업에서 좌표를 손으로 옮겨 적다가 여러 번 그랬다.
import json
table = {}
for name, arr, frame in rows:
    h, w = arr.shape[:2]
    table[name] = {"w": int(w), "h": int(h), "frame": int(frame),
                   "margin_x": round(frame / float(w), 4),
                   "margin_y": round(frame / float(h), 4)}
with io.open(os.path.join(OUT, "slices.json"), "w", encoding="utf-8") as handle:
    json.dump(table, handle, ensure_ascii=False, indent=1)
print("여백 표: %s" % os.path.join(OUT, "slices.json"))
for k, v in sorted(table.items()):
    print("  %-20s %3dx%-3d  x=%.4f y=%.4f" % (k, v["w"], v["h"],
                                               v["margin_x"], v["margin_y"]))
