# -*- coding: utf-8 -*-
"""배치안 열 개를 각자의 시안과 대조해 판 단위 어긋남을 낸다.

1안은 손으로 좌표를 잡아 가며 맞췄지만, 아홉 개를 같은 방식으로 하면 사람이
좌표를 적다가 틀린다 -- 실제로 여러 번 그랬다. 그래서 재는 자리를 손으로
적지 않는다. 재질 색으로 화면 전체에서 가장 큰 덩어리를 찾고, 두 그림에서
같은 방식으로 찾은 것끼리 견준다.

배치안마다 있는 판이 다르다(미니멀은 적 패널이 없고, 방사형은 스킬 카드가
원을 그린다). 한쪽에만 있으면 "없음"으로 적고 넘어간다 -- 없는 것을 억지로
맞추면 배치안을 서로 닮게 만들어 버린다. 열 개를 만든 이유가 사라진다.

    python diagnose_all_layouts.py            # 전부
    python diagnose_all_layouts.py --only 03  # 하나만
"""
import argparse
import io
import os
import sys

import numpy as np
from PIL import Image
from scipy import ndimage

CAPS = r"D:/UnrealProjects/P_RD_develop_20260726/Saved/UI/CombatLayouts"
MOCKS = r"D:/UnrealProjects/P_RD_develop/Tools/UI/KayKitHUDMockups/Raw"

LAYOUTS = [
    ("01", "WBP_CombatLayout_01_ClassicCRPG"),
    ("02", "WBP_CombatLayout_02_LeftParty"),
    ("03", "WBP_CombatLayout_03_ActiveUnit"),
    ("04", "WBP_CombatLayout_04_Radial"),
    ("05", "WBP_CombatLayout_05_BottomBar"),
    ("06", "WBP_CombatLayout_06_Mirrored"),
    ("07", "WBP_CombatLayout_07_CardHand"),
    ("08", "WBP_CombatLayout_08_Minimal"),
    ("09", "WBP_CombatLayout_09_SplitBands"),
    ("10", "WBP_CombatLayout_10_Targeting"),
]

#: 재질별 판정. 배경(잔디·하늘·나무 지형)에 걸리지 않게 좁게 잡았다.
MATERIALS = [
    ("양피지", lambda r, g, b: (r > 150) & (g > 125) & (b > 85)
     & (r > b + 22) & (r - b < 95) & (r >= g) & (r - g < 52)),
    ("적 석재", lambda r, g, b: (r > 75) & (r < 205) & (r > g + 45) & (r > b + 42)),
    ("주황 버튼", lambda r, g, b: (r > 95) & (r > g + 32) & (g > b + 14)
     & (g > 38) & (r < 235)),
    ("나무", lambda r, g, b: (r > 65) & (r < 195) & (r > b + 30)
     & (g > b + 5) & (g < r)),
    ("청회 석재", lambda r, g, b: (np.abs(r - g) < 30) & (b >= g - 12)
     & (r > 42) & (r < 142)),
]


def blobs(img, test, min_area=2500):
    """조건에 맞는 덩어리들. 큰 것부터."""
    r, g, b = img[:, :, 0], img[:, :, 1], img[:, :, 2]
    mask = ndimage.binary_closing(test(r, g, b), np.ones((7, 7)))
    lab, count = ndimage.label(mask)
    if count == 0:
        return []
    sizes = ndimage.sum(mask, lab, range(1, count + 1))
    out = []
    for i in np.argsort(sizes)[::-1]:
        if sizes[i] < min_area:
            break
        ys, xs = np.where(lab == i + 1)
        out.append((int(xs.min()), int(ys.min()),
                    int(xs.max()) + 1, int(ys.max()) + 1, int(sizes[i])))
    return out


def match(mock_list, ours_list):
    """가장 많이 겹치는 것끼리 짝짓는다. 짝이 없으면 None."""
    pairs, left = [], list(ours_list)
    for m in mock_list:
        best, score = None, 0
        for c in left:
            ix = max(0, min(m[2], c[2]) - max(m[0], c[0]))
            iy = max(0, min(m[3], c[3]) - max(m[1], c[1]))
            if ix * iy > score:
                best, score = c, ix * iy
        if best is not None and score > 0:
            left.remove(best)
        pairs.append((m, best))
    return pairs, left


def run(number, stem, tol_pos=12, tol_size=18):
    cap = os.path.join(CAPS, stem + ".png")
    mock = os.path.join(MOCKS, "KK_HUD_Polish_%s.png" % number)
    if not (os.path.exists(cap) and os.path.exists(mock)):
        print("%s 파일 없음" % stem)
        return 0, 0
    C = np.array(Image.open(cap).convert("RGB")).astype(float)
    M = np.array(Image.open(mock).convert("RGB")).astype(float)
    if M.shape != C.shape:
        M = np.array(Image.open(mock).convert("RGB")
                     .resize((C.shape[1], C.shape[0]), Image.LANCZOS)).astype(float)

    print()
    print("=== %s ===" % stem)
    ok = total = 0
    for label, test in MATERIALS:
        mb, cb = blobs(M, test), blobs(C, test)
        # 큰 것 두 개까지만 본다. 배경 조각이 섞이면 아래로 밀린다.
        pairs, extra = match(mb[:2], cb[:2])
        for m, c in pairs:
            total += 1
            if c is None:
                print("  %-10s 시안 (%4d,%3d) %4dx%-3d   우리에게 없음"
                      % (label, m[0], m[1], m[2] - m[0], m[3] - m[1]))
                continue
            dx, dy = c[0] - m[0], c[1] - m[1]
            dw = (c[2] - c[0]) - (m[2] - m[0])
            dh = (c[3] - c[1]) - (m[3] - m[1])
            good = (max(abs(dx), abs(dy)) <= tol_pos
                    and max(abs(dw), abs(dh)) <= tol_size)
            ok += good
            print("  %-10s (%4d,%3d)%4dx%-3d (%4d,%3d)%4dx%-3d %+4d,%+3d/%+4d,%+3d %s"
                  % (label, m[0], m[1], m[2] - m[0], m[3] - m[1],
                     c[0], c[1], c[2] - c[0], c[3] - c[1], dx, dy, dw, dh,
                     "OK" if good else "<<"))
        for c in extra:
            print("  %-10s 시안에 없는데 우리에게 있음 (%4d,%3d) %4dx%-3d"
                  % (label, c[0], c[1], c[2] - c[0], c[3] - c[1]))
    print("  -- %d/%d" % (ok, total))
    return ok, total


def main():
    sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding="utf-8")
    ap = argparse.ArgumentParser()
    ap.add_argument("--only")
    args = ap.parse_args()
    grand_ok = grand_total = 0
    for number, stem in LAYOUTS:
        if args.only and args.only != number:
            continue
        ok, total = run(number, stem)
        grand_ok += ok
        grand_total += total
    print()
    print("전체 %d/%d" % (grand_ok, grand_total))


if __name__ == "__main__":
    main()
