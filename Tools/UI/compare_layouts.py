# -*- coding: utf-8 -*-
"""구운 화면을 시안과 견준다. 두 가지를 따로 묻는다.

## 조각이 제자리에 놓였나

명세에 적힌 자리와 배치안에 넣은 자리를 견주는 것은 검사가 아니다 -- 같은
숫자를 읽어 넣었으니 항상 같다. 캡처 그림에서 조각을 다시 찾아 시안 자리와
견준다. 그 사이에 배율 규칙과 앵커와 부모 캔버스가 끼어 있고, 이 작업에서
틀린 것의 대부분이 거기서 생겼다.

다만 이 검사는 내가 놓은 그림을 내가 놓은 자리에서 찾는 것이라, 판이 밀린
경우만 잡는다. 6안이 18/18을 받고도 카드가 검고 이름이 엉뚱한 데 있었다.

## 내용이 시안과 같은 자리에 있나

그래서 하나 더 본다. 캡처에서 빈 판을 빼면 우리가 얹은 것들의 자리가 나오고,
시안에서 빼면 시안이 얹은 것들의 자리가 나온다. 두 자리를 견준다.

픽셀로 통째로 견주던 것은 그만뒀다. 초상을 얼굴로 바꿔 화면이 분명히
좋아졌는데 점수는 떨어졌기 때문이다 -- 얼굴이 커져 화면을 더 많이 차지하는데,
우리 초상은 KayKit 렌더이고 시안 초상은 생성 그림이라 애초에 같아질 수가
없다. 그림이 같아지는 것은 목표가 아니다. 물어야 할 것은 이름과 막대와
얼굴이 시안과 같은 자리에 있느냐다.

    python compare_layouts.py            전부
    python compare_layouts.py --mockup 13
"""
import argparse
import io
import json
import os
import sys

from concurrent.futures import ProcessPoolExecutor

import numpy as np
from PIL import Image
from scipy import ndimage

CHROME = r"D:/UnrealProjects/P_RD_develop/Tools/UI/KayKitUIKit/Chrome"
CUTOUTS = r"D:/UnrealProjects/P_RD_develop/Tools/UI/KayKitUIKit/Cutouts"
MOCKUPS = r"D:/UnrealProjects/P_RD_develop/Tools/UI/KayKitHUDMockups/Raw"
SHOTS = (r"D:/UnrealProjects/P_RD_develop_20260726"
         r"/Saved/UI/CombatLayouts")

#: 배치안 파일 이름. 굽기 목록과 같은 순서다.
ASSETS = [
    "WBP_CombatLayout_01_ClassicCRPG", "WBP_CombatLayout_02_LeftParty",
    "WBP_CombatLayout_03_ActiveUnit", "WBP_CombatLayout_04_Radial",
    "WBP_CombatLayout_05_BottomBar", "WBP_CombatLayout_06_Mirrored",
    "WBP_CombatLayout_07_CardHand", "WBP_CombatLayout_08_Minimal",
    "WBP_CombatLayout_09_SplitBands", "WBP_CombatLayout_10_Targeting",
    "WBP_CombatLayout_11_RightGrid", "WBP_CombatLayout_12_TurnQueue",
    "WBP_CombatLayout_13_RightList", "WBP_CombatLayout_14_FloatingBar",
    "WBP_CombatLayout_15_UnifiedDock", "WBP_CombatLayout_16_FullFrame",
    "WBP_CombatLayout_17_RightDock", "WBP_CombatLayout_18_RightFan",
    "WBP_CombatLayout_19_TopRail", "WBP_CombatLayout_20_CommandMode",
]

#: 이 점수를 넘으면 조각을 못 찾은 것으로 본다. 잘 맞은 것은 10 안쪽이다.
FOUND = 22.0

#: 이 픽셀 수를 넘게 어긋나면 눈에 띈다. 시안 1672 폭에서 잰 값이다.
TOLERANCE = 6

#: 기대 자리에서 이만큼까지만 찾는다.
#:
#: 화면 전체를 훑으면 빈 카드끼리 서로의 칸에 붙는다 -- 똑같이 생겼으니
#: 당연하다. 13안에서 카드가 220px 어긋난 것으로 나왔는데, 실은 옆 칸에
#: 붙은 것이었다. 검사가 묻는 것은 제자리에 있느냐이지 어디든 있느냐가
#: 아니므로, 제자리 둘레만 본다.
WINDOW = 48

#: 두 상자가 이만큼 겹치면 같은 자리로 본다.
OVERLAP = 0.35


# ─── 조각이 제자리에 놓였나 ────────────────────────────────────────────────────

def find(shot_grey, part_path, want_x, want_y):
    """기대 자리 둘레에서 조각이 가장 잘 맞는 곳과 그때의 평균차."""
    arr = np.asarray(Image.open(part_path).convert("RGBA"), dtype=float)
    grey = arr[:, :, :3].mean(axis=2)
    alpha = arr[:, :, 3] > 200
    th, tw = grey.shape
    mh, mw = shot_grey.shape
    # 화면 폭을 꽉 채우는 판이 있다(20안 상단 띠는 1672px).
    if th > mh or tw > mw or alpha.sum() < 400:
        return None
    tpl = grey[alpha]
    tpl = tpl - tpl.mean()

    best, pos = None, None
    for y in range(max(0, min(want_y - WINDOW, mh - th)),
                   min(mh - th, want_y + WINDOW) + 1):
        for x in range(max(0, min(want_x - WINDOW, mw - tw)),
                       min(mw - tw, want_x + WINDOW) + 1):
            win = shot_grey[y:y + th, x:x + tw][alpha]
            # 밝기를 맞춘 뒤 견준다. 선택된 판에는 금빛 덮개가 얹혀 밝기가
            # 통째로 올라가는데, 그대로 견주면 제자리에 있어도 점수가 튄다.
            score = float(np.abs((win - win.mean()) - tpl).mean())
            if best is None or score < best:
                best, pos = score, (y, x)
    if pos is None:
        return None
    return pos[1], pos[0], best


def _one(job):
    """조각 하나를 캡처에서 찾아 시안 자리와 견준다."""
    shot_path, part_path, want_x, want_y, role = job
    shot = np.asarray(Image.open(shot_path).convert("RGB"),
                      dtype=float).mean(axis=2)
    got = find(shot, part_path, want_x, want_y)
    if got is None:
        return {"role": role, "state": "못읽음"}
    x, y, score = got
    dx, dy = x - want_x, y - want_y
    if score <= FOUND:
        return {"role": role, "state": "찾음", "dx": dx, "dy": dy}
    # 점수가 높아도 제자리를 짚었으면 "없다"가 아니라 "덮였다"다. 작은 판은
    # 얼굴과 막대와 글자가 판 대부분을 가려서 결이 통째로 달라진다.
    if max(abs(dx), abs(dy)) <= TOLERANCE:
        return {"role": role, "state": "덮임", "dx": dx, "dy": dy}
    return {"role": role, "state": "못찾음", "score": round(score, 1)}


def plates(number, manifest, pool):
    shot_path = os.path.join(SHOTS, ASSETS[int(number) - 1] + ".png")
    if not os.path.exists(shot_path):
        return None

    jobs = []
    for row in manifest[number]:
        for asset, ax, ay, _aw, _ah in row["arts"]:
            path = os.path.join(CUTOUTS, asset + ".png")
            if os.path.exists(path):
                jobs.append((shot_path, path, row["rect"][0] + ax,
                             row["rect"][1] + ay, row["role"]))
    results = list(pool.map(_one, jobs, chunksize=1))

    off = [r for r in results if r["state"] == "찾음"
           and max(abs(r["dx"]), abs(r["dy"])) > TOLERANCE]
    veiled = [r for r in results if r["state"] == "덮임"]
    lost = [r for r in results if r["state"] not in ("찾음", "덮임")]
    ok = len(results) - len(off) - len(lost) - len(veiled)
    for item in off:
        print("    %-10s dx=%+4d dy=%+4d" % (item["role"], item["dx"],
                                             item["dy"]))
    for item in lost:
        print("    %-10s %s" % (item["role"], item["state"]))
    return ok, len(veiled), len(off), len(lost)


# ─── 내용이 시안과 같은 자리에 있나 ────────────────────────────────────────────

def _content(area, blank):
    """빈 판과 다른 곳. 곧 글자와 얼굴이 놓인 자리다."""
    h = min(area.shape[0], blank.shape[0])
    w = min(area.shape[1], blank.shape[1])
    if h < 24 or w < 24:
        return []
    gap = np.abs(area[:h, :w] - blank[:h, :w])
    inner = ndimage.binary_erosion(np.ones((h, w), bool), np.ones((9, 9)))
    mask = ndimage.binary_closing(inner & (gap > 34), np.ones((3, 5)))
    labels, count = ndimage.label(mask)
    boxes = []
    if count:
        sizes = ndimage.sum(mask, labels, range(1, count + 1))
        for i in np.argsort(sizes)[::-1][:10]:
            if sizes[i] < 200:
                break
            ys, xs = np.where(labels == i + 1)
            bw = int(xs.max() - xs.min() + 1)
            bh = int(ys.max() - ys.min() + 1)
            if bw < 9 or bh < 9:
                continue
            if bw > w * 0.92 and bh > h * 0.92:
                continue
            boxes.append((int(xs.min()), int(ys.min()), bw, bh))
    return boxes


def _iou(a, b):
    ix = max(0, min(a[0] + a[2], b[0] + b[2]) - max(a[0], b[0]))
    iy = max(0, min(a[1] + a[3], b[1] + b[3]) - max(a[1], b[1]))
    hit = ix * iy
    return hit / float(max(a[2] * a[3] + b[2] * b[3] - hit, 1))


def _zone(job):
    """구역 하나에서 우리 내용이 시안 내용과 같은 자리에 있는지."""
    shot_path, mock_path, blank_path, rect, role = job
    x, y, w, h = rect
    crop = (x, y, x + w, y + h)

    def grey(path):
        art = Image.open(path).convert("RGB").crop(crop)
        return np.asarray(art, dtype=float).mean(axis=2)

    blank = np.asarray(Image.open(blank_path).convert("RGB"),
                       dtype=float).mean(axis=2)
    want = _content(grey(mock_path), blank)
    if not want:
        return None
    ours = _content(grey(shot_path), blank)

    hit = sum(1 for target in want
              if max([_iou(target, got) for got in ours] or [0.0]) > OVERLAP)
    return {"role": role, "rect": rect, "hit": hit, "want": len(want)}


def zones(number, manifest, pool):
    shot_path = os.path.join(SHOTS, ASSETS[int(number) - 1] + ".png")
    mock_path = os.path.join(MOCKUPS, "KK_HUD_Polish_%s.png" % number)
    if not (os.path.exists(shot_path) and os.path.exists(mock_path)):
        return None

    jobs = []
    for row in manifest[number]:
        arts = row.get("arts") or []
        # 여러 장으로 덮은 자리는 빈 판 한 장으로 뺄 수 없다.
        if len(arts) != 1:
            continue
        blank = os.path.join(CUTOUTS, arts[0][0] + ".png")
        if os.path.exists(blank):
            jobs.append((shot_path, mock_path, blank, row["rect"],
                         row["role"]))
    found = [r for r in pool.map(_zone, jobs, chunksize=1) if r]
    if not found:
        return None

    hit = sum(r["hit"] for r in found)
    want = sum(r["want"] for r in found)
    bad = sorted([r for r in found if r["hit"] < r["want"]],
                 key=lambda r: r["hit"] - r["want"])
    print("시안%s  %-24s  자리 맞음 %3d / %3d" % (
        number, ASSETS[int(number) - 1][17:], hit, want))
    for item in bad[:3]:
        print("    %-10s %d/%d  (x=%d y=%d %dx%d)" % (
            item["role"], item["hit"], item["want"], item["rect"][0],
            item["rect"][1], item["rect"][2], item["rect"][3]))
    return hit, want


def main():
    sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding="utf-8")
    ap = argparse.ArgumentParser()
    ap.add_argument("--mockup", default=None, help="비우면 1~20 전부")
    args = ap.parse_args()

    with io.open(os.path.join(CHROME, "cutout_manifest.json"),
                 encoding="utf-8") as handle:
        manifest = json.load(handle)

    numbers = ([("%02d" % int(args.mockup))] if args.mockup
               else sorted(manifest, key=int))
    workers = max(1, min(16, (os.cpu_count() or 4) - 2))

    print("=== 내용이 시안과 같은 자리에 있나 ===")
    hit, want = 0, 0
    with ProcessPoolExecutor(max_workers=workers) as pool:
        for number in numbers:
            got = zones(number, manifest, pool)
            if got:
                hit += got[0]
                want += got[1]
    print("  시안이 얹은 것 %d개 중 %d개가 같은 자리에 있다" % (want, hit))

    print()
    print("=== 조각이 제자리에 놓였나 ===")
    total = [0, 0, 0, 0]
    with ProcessPoolExecutor(max_workers=workers) as pool:
        for number in numbers:
            got = plates(number, manifest, pool)
            if got:
                total = [a + b for a, b in zip(total, got)]
    print("  맞음 %d  덮임 %d  어긋남 %d  못찾음 %d  (허용 %dpx)"
          % (total[0], total[1], total[2], total[3], TOLERANCE))


if __name__ == "__main__":
    main()
