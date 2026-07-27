# -*- coding: utf-8 -*-
"""구운 화면에서 조각이 실제로 어디 놓였는지 재어 시안과 견준다.

## 왜 캡처를 다시 재나

명세에 적힌 자리와 배치안에 넣은 자리는 같을 수밖에 없다 -- 같은 숫자를
읽어 넣었으니까. 그걸 견주는 것은 검사가 아니다.

물어야 할 것은 "화면에 나온 자리가 시안의 그 자리냐"다. 그 사이에 배율
규칙과 앵커와 부모 캔버스가 끼어 있고, 이 작업에서 틀린 것의 대부분이
거기서 생겼다 -- 앵커를 안 줘서 구역이 겹쳤고, 캡처를 1920 으로 떠서
15% 어긋난 적도 있다.

그래서 캡처 그림에서 조각을 다시 찾는다. 캡처도 시안과 같은 1672x941 이라
찾은 자리를 시안 자리에서 빼면 그게 어긋난 양이다.

## 어떻게 찾나

조각 그림을 캡처 위에서 밀어 보며 가장 잘 맞는 곳을 고른다. 자리표를 만들
때 시안을 상대로 했던 것과 같은 방법이고, 상대만 캡처로 바뀐다.

가장 잘 맞는 점수가 낮으면(=잘 맞으면) 그 자리를 믿고, 높으면 "못 찾음"
으로 적는다. 안 그리는 것과 엉뚱한 데 그리는 것은 다른 문제인데, 억지로
자리를 붙이면 둘이 섞여 보인다.

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

CHROME = r"D:/UnrealProjects/P_RD_develop/Tools/UI/KayKitUIKit/Chrome"
CUTOUTS = r"D:/UnrealProjects/P_RD_develop/Tools/UI/KayKitUIKit/Cutouts"
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

#: 이 점수를 넘으면 못 찾은 것으로 본다. 잘 맞은 것은 10 안쪽이다.
FOUND = 26.0

#: 이 픽셀 수를 넘게 어긋나면 눈에 띈다. 시안 1672 폭에서 잰 값이다.
TOLERANCE = 6


#: 기대 자리에서 이만큼까지만 찾는다.
#:
#: 화면 전체를 훑으면 빈 카드끼리 서로의 칸에 붙는다 -- 똑같이 생겼으니
#: 당연하다. 13안에서 카드가 220px 어긋난 것으로 나왔는데, 실은 옆 칸에
#: 붙은 것이었다. 검사가 묻는 것은 "제자리에 있느냐"이지 "어디든 있느냐"가
#: 아니므로, 제자리 둘레만 본다. 여기서 못 찾으면 그게 답이다.
WINDOW = 48


def find(shot_grey, part_path, want_x, want_y):
    """기대 자리 둘레에서 조각이 가장 잘 맞는 곳과 그때의 평균차."""
    arr = np.asarray(Image.open(part_path).convert("RGBA"), dtype=float)
    grey = arr[:, :, :3].mean(axis=2)
    alpha = arr[:, :, 3] > 200
    th, tw = grey.shape
    mh, mw = shot_grey.shape
    if th >= mh or tw >= mw or alpha.sum() < 400:
        return None

    best, pos = None, None
    for y in range(max(0, want_y - WINDOW),
                   min(mh - th, want_y + WINDOW) + 1):
        for x in range(max(0, want_x - WINDOW),
                       min(mw - tw, want_x + WINDOW) + 1):
            win = shot_grey[y:y + th, x:x + tw]
            score = float(np.abs(win[alpha] - grey[alpha]).mean())
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
    if score > FOUND:
        return {"role": role, "state": "못찾음", "score": round(score, 1)}
    return {"role": role, "state": "찾음", "dx": x - want_x, "dy": y - want_y,
            "score": round(score, 1)}


def run(number, manifest, pool):
    shot_path = os.path.join(SHOTS, ASSETS[int(number) - 1] + ".png")
    if not os.path.exists(shot_path):
        print("시안%s: 캡처 없음" % number)
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
    lost = [r for r in results if r["state"] != "찾음"]
    ok = len(results) - len(off) - len(lost)
    print("시안%s  %-28s  맞음 %2d  어긋남 %2d  못찾음 %2d" % (
        number, ASSETS[int(number) - 1][17:], ok, len(off), len(lost)))
    for r in sorted(off, key=lambda e: -max(abs(e["dx"]), abs(e["dy"]))):
        print("    %-10s dx=%+4d dy=%+4d" % (r["role"], r["dx"], r["dy"]))
    for r in lost:
        print("    %-10s %s%s" % (r["role"], r["state"],
                                  "  차 %s" % r.get("score", "")))
    return ok, len(off), len(lost)


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
    total = [0, 0, 0]
    with ProcessPoolExecutor(max_workers=workers) as pool:
        for number in numbers:
            got = run(number, manifest, pool)
            if got:
                total = [a + b for a, b in zip(total, got)]

    print()
    print("=== 합계 ===")
    print("  맞음 %d  어긋남 %d  못찾음 %d  (허용 %dpx)"
          % (total[0], total[1], total[2], TOLERANCE))


if __name__ == "__main__":
    main()
