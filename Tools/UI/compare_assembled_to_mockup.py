# -*- coding: utf-8 -*-
"""조립된 1920x1080 화면을 시안과 패널 단위로 비교한다.

## 왜 이게 필요한가

지금까지의 검수는 조각 하나하나를 재서 통과시켰다 -- 평균색, 채도, 대비,
이음매, 알파 쌍. 전부 통과했는데 화면은 시안과 달랐다. 그 검수로는 이런
것들이 전부 "합격"이기 때문이다:

  * 평균색은 같지만 16px 체크무늬인 면
  * 대비는 같지만 몰딩이 없는 레일
  * 이음매는 0이지만 복사·붙여넣기로 읽히는 직선 구간
  * 캔버스 규격은 맞지만 화면에서는 절반 크기인 초상화
  * 조각은 멀쩡한데 조립하면 비례가 다른 패널

조각이 아니라 "완성 화면"이 시안과 같은지를 봐야 한다. 이 파일이 그것이다.

## 무엇을 재는가

패널마다 여섯 축을 시안과 캡처에서 똑같이 재고 비율을 낸다.

  상자      패널의 폭·높이
  프레임    레일 두께 / 패널 최소변  (프레임이 패널을 얼마나 먹는가)
  면        밝기, 그리고 결의 요동 (16px 타일 표준편차의 중앙값)
  블록      16px 격자 경계가 튀는 정도 (줄였다 키운 흔적)
  내용      면색에서 벗어난 픽셀 비율 (패널이 얼마나 차 있는가)
  글자      밝은 글자 행의 기준선 위치 (패널 높이 대비)

## --elements 는 신호일 뿐, 숫자를 그대로 믿지 말 것

요소 비교는 시안 덩어리와 캡처 덩어리를 "가장 가까운 것끼리" 짝짓는다.
덩어리가 합쳐지거나 쪼개지면 엉뚱한 짝이 생기고, 그러면 위치 차이가
그럴듯한 숫자로 나온다. 실제로 이 도구가 "아이콘이 8%p 위로 18%p 크다,
보석이 20%p 위다" 라고 했는데, 상자를 제대로 잡고 색으로 특정해 다시 재니
아이콘은 51.8% 대 56.2% 로 거의 같았고 보석은 위가 아니라 8%p 아래였다.
그대로 고쳤으면 멀쩡한 아이콘을 줄이고 보석을 반대로 내릴 뻔했다.

**"여기가 다른 것 같다"는 신호로만 쓰고, 고치기 전에 그 요소만 색으로
특정해서 다시 재라.** 특히 "빠짐/남음" 은 데이터 차이(아군 수, AP 0)로도
생기므로 같은 미리보기 데이터끼리 비교할 때만 의미가 있다.

## 자기 검증

좌표를 손으로 적으면 엉뚱한 데를 재고도 숫자가 나온다 -- 실제로 그랬다.
그래서 --boxes 로 두 이미지에 잰 영역을 그려 저장한다. 숫자를 믿기 전에
그 그림을 먼저 봐야 한다.

    python compare_assembled_to_mockup.py --boxes out.png
"""
import argparse
import io
import json
import os
import sys

import numpy as np
from PIL import Image, ImageDraw
from scipy import ndimage

MOCKUP = (r"D:/UnrealProjects/P_RD_develop/Tools/UI/KayKitHUDMockups"
          r"/KK_HUD_Polish_01.png")
CAPTURE = (r"D:/UnrealProjects/P_RD_develop_20260726/Saved/UI/CombatLayouts"
           r"/WBP_CombatLayout_01_ClassicCRPG.png")

#: 시안에서 눈으로 확인하고 잡은 패널 상자. 캡처 쪽은 손으로 적지 않고
#: 검은 배경에서 덩어리를 찾아 이 상자와 겹치는 것을 고른다 -- 배치안마다
#: 패널이 조금씩 움직이므로 좌표를 두 벌 관리하면 반드시 어긋난다.
PANELS = {
    "라운드 판":    (24, 18, 268, 104),
    "턴 칸(첫째)":  (620, 8, 748, 132),
    "목표 현판":    (1528, 18, 1896, 104),
    "파티 선두":    (8, 672, 542, 812),
    "파티 둘째":    (8, 814, 542, 950),
    "스킬 카드1":   (536, 702, 702, 1058),
    "적 패널":      (1556, 756, 1900, 1002),
    "턴 종료":      (1586, 962, 1896, 1054),
}


def load(path):
    if not os.path.exists(path):
        raise SystemExit("파일 없음: " + path)
    return np.array(Image.open(path).convert("RGB")).astype(float)


def capture_boxes(img, floor=26.0):
    """검은 배경 위의 덩어리들. 캡처는 배경이 비어 있어 이게 확실하다."""
    lit = img.mean(axis=2) > floor
    rows = np.where(lit.any(axis=1))[0]
    if not len(rows):
        return []
    seen = np.zeros(lit.shape, dtype=bool)
    boxes = []
    # 굳이 연결요소를 다 돌 필요는 없다. 행 구간 -> 열 구간으로 두 번 쪼갠다.
    def spans(mask, minlen):
        out, start = [], None
        for i, v in enumerate(list(mask) + [False]):
            if v and start is None:
                start = i
            elif not v and start is not None:
                if i - start >= minlen:
                    out.append((start, i))
                start = None
        return out

    for y0, y1 in spans(lit.any(axis=1), 24):
        band = lit[y0:y1]
        for x0, x1 in spans(band.any(axis=0), 24):
            sub = band[:, x0:x1]
            ys = np.where(sub.any(axis=1))[0]
            boxes.append((x0, y0 + ys.min(), x1, y0 + ys.max() + 1))
    del seen
    return boxes


def pick(boxes, want, img, margin=12, floor=26.0):
    """시안 상자에 대응하는 캡처 패널의 상자.

    덩어리를 그대로 쓰면 안 된다 -- 턴 칸 다섯이 맞붙어 하나로 잡히고 파티
    카드 셋도 한 덩어리가 된다(실측 4.4배, 2.8배). 시안 상자 근방으로 한 번
    자른 뒤, 그 안에서 다시 내용에 맞춰 조인다.

    여유(margin)는 좁게 잡는다. 파티 행처럼 4px 간격으로 붙은 패널은 여유가
    조금만 커도 이웃을 삼켜 높이가 1.7배로 나오고, 그러면 내용 비율이 이웃의
    빈 공간으로 희석돼 "덜 찼다"는 잘못된 결론이 나온다.
    """
    wx0, wy0, wx1, wy1 = want
    best, score = None, 0.0
    for b in boxes:
        ix = max(0, min(b[2], wx1) - max(b[0], wx0))
        iy = max(0, min(b[3], wy1) - max(b[1], wy0))
        if ix * iy > score:
            best, score = b, ix * iy
    if best is None:
        return None
    x0 = max(best[0], wx0 - margin)
    y0 = max(best[1], wy0 - margin)
    x1 = min(best[2], wx1 + margin)
    y1 = min(best[3], wy1 + margin)
    if x1 - x0 < 16 or y1 - y0 < 16:
        return None
    lit = img[y0:y1, x0:x1].mean(axis=2) > floor
    ys = np.where(lit.any(axis=1))[0]
    xs = np.where(lit.any(axis=0))[0]
    if not len(ys) or not len(xs):
        return None
    return (x0 + xs.min(), y0 + ys.min(), x0 + xs.max() + 1, y0 + ys.max() + 1)


def tile_std(gray, tile=16):
    """타일별 표준편차의 중앙값. 글자가 든 타일은 위로 밀려 걸러진다."""
    h, w = gray.shape
    if h < tile or w < tile:
        return 0.0
    blocks = gray[:h // tile * tile, :w // tile * tile]
    blocks = blocks.reshape(h // tile, tile, w // tile, tile)
    return float(np.median(blocks.std(axis=(1, 3))))


def block_step(gray, tile=16):
    """16px 격자 경계가 내부보다 얼마나 튀는가. 줄였다 키운 흔적."""
    if gray.shape[1] < tile * 3:
        return 0.0
    dx = np.abs(np.diff(gray, axis=1))
    edge = dx[:, tile - 1::tile]
    inner = np.delete(dx, np.s_[tile - 1::tile], axis=1)
    if not inner.size or not edge.size:
        return 0.0
    return float(edge.mean() / max(inner.mean(), 0.01))


def rail_thickness(img, box):
    """패널 세로 중앙에서 왼쪽 가장자리의 밝은 띠 두께."""
    x0, y0, x1, y1 = box
    ym = (y0 + y1) // 2
    reach = min(48, (x1 - x0) // 3)
    line = img[ym, x0:x0 + reach].mean(axis=1)
    face = np.median(img[ym, x0 + reach:x1 - reach].mean(axis=1))
    hot = line > face + 22
    run = 0
    for v in hot:
        if v:
            run += 1
        elif run:
            break
    return run


def text_baseline(img, box):
    """가장 아래쪽 글자 행의 중심이 패널 높이의 몇 %에 있는가."""
    x0, y0, x1, y1 = box
    reg = img[y0:y1, x0:x1].mean(axis=2)
    rows = (reg > 185).sum(axis=1)
    ys = np.where(rows > 2)[0]
    if not len(ys):
        return None
    return float(100.0 * ys.max() / max(y1 - y0, 1))


def measure(img, box):
    x0, y0, x1, y1 = box
    reg = img[y0:y1, x0:x1]
    gray = reg.mean(axis=2)
    inset = reg[8:-8, 8:-8] if min(reg.shape[:2]) > 24 else reg
    ig = inset.mean(axis=2)
    base = np.median(inset.reshape(-1, 3), axis=0)
    dev = np.abs(inset - base).mean(axis=2)
    rail = rail_thickness(img, box)
    return {
        "폭": int(x1 - x0),
        "높이": int(y1 - y0),
        "프레임점유": float(100.0 * rail / max(min(x1 - x0, y1 - y0), 1)),
        "면밝기": float(ig.mean()),
        "결요동": tile_std(ig),
        "블록계단": block_step(ig),
        "내용픽셀": float(100.0 * (dev > 25).mean()),
        "글자기준선": text_baseline(img, box),
    }


def elements(img, box, min_area=140):
    """패널 안의 낱개 요소들. 아이콘·글자덩이·바·보석이 각각 하나로 잡힌다.

    패널 중앙값 색에서 벗어난 픽셀을 이어 붙여 덩어리를 만든다. 프레임은
    패널 테두리를 따라 길게 눕는 한 덩어리가 되므로, 가장자리에 닿은
    덩어리는 프레임으로 보고 뺀다 -- 안에 있는 것만 요소다.
    """
    x0, y0, x1, y1 = box
    reg = img[y0:y1, x0:x1]
    h, w = reg.shape[:2]
    base = np.median(reg.reshape(-1, 3), axis=0)
    fg = np.abs(reg - base).mean(axis=2) > 26
    # 한두 픽셀 구멍은 메워서 글자 획이 낱자로 흩어지지 않게 한다.
    fg = ndimage.binary_closing(fg, structure=np.ones((3, 5)))
    lab, count = ndimage.label(fg)
    out = []
    for i in range(1, count + 1):
        ys, xs = np.where(lab == i)
        if len(ys) < min_area:
            continue
        bx0, bx1, by0, by1 = xs.min(), xs.max(), ys.min(), ys.max()
        if bx0 <= 2 or by0 <= 2 or bx1 >= w - 3 or by1 >= h - 3:
            continue  # 테두리에 닿았다 -- 프레임이다
        out.append({
            "x": 100.0 * bx0 / w, "y": 100.0 * by0 / h,
            "w": 100.0 * (bx1 - bx0 + 1) / w,
            "h": 100.0 * (by1 - by0 + 1) / h,
            "픽셀": int(len(ys)),
        })
    out.sort(key=lambda e: (e["y"], e["x"]))
    return out


def match_elements(a, b, tol=14.0):
    """시안 요소와 캡처 요소를 위치로 짝짓는다. 짝이 없으면 빠지거나 남는 것."""
    left = list(b)
    pairs, missing = [], []
    for e in a:
        best, dist = None, 1e9
        for f in left:
            d = abs(e["x"] - f["x"]) + abs(e["y"] - f["y"])
            if d < dist:
                best, dist = f, d
        if best is not None and dist <= tol * 2:
            pairs.append((e, best))
            left.remove(best)
        else:
            missing.append(e)
    return pairs, missing, left


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--mockup", default=MOCKUP)
    ap.add_argument("--capture", default=CAPTURE)
    ap.add_argument("--boxes", help="잰 영역을 그려 저장할 경로")
    ap.add_argument("--json", help="수치를 JSON으로 저장할 경로")
    ap.add_argument("--elements", action="store_true",
                    help="패널 안 요소 하나하나까지 비교")
    args = ap.parse_args()

    M, C = load(args.mockup), load(args.capture)
    if M.shape != C.shape:
        print("경고: 크기가 다르다 %s vs %s" % (M.shape[:2], C.shape[:2]))
    found = capture_boxes(C)

    report = {}
    used = {}
    for label, box in PANELS.items():
        cbox = pick(found, box, C)
        if cbox is None:
            print("%-12s 캡처에서 못 찾음" % label)
            continue
        used[label] = cbox
        report[label] = {"시안": measure(M, box), "캡처": measure(C, cbox)}

    # 블록계단은 여기서 빼둔다. 면은 화면에서 타일로 반복되므로 원단의
    # 16px 격자가 4px 로 줄어 이 해상도에서는 안 걸린다 -- 그 검사는 원단
    # 파일에서 해야 한다(verify 쪽 소관).
    keys = ["폭", "높이", "프레임점유", "면밝기", "결요동",
            "내용픽셀", "글자기준선"]
    print("%-12s %-11s %8s %8s %8s" % ("패널", "항목", "시안", "캡처", "비"))
    print("-" * 54)
    for label, pair in report.items():
        for k in keys:
            a, b = pair["시안"][k], pair["캡처"][k]
            if a is None or b is None:
                continue
            ratio = b / a if a else 0.0
            flag = "" if 0.8 <= ratio <= 1.25 else "  <<"
            print("%-12s %-11s %8.1f %8.1f %7.2f배%s"
                  % (label, k, a, b, ratio, flag))
        print()

    if args.elements:
        print()
        print("=== 요소 단위 (패널 안에서의 위치·크기, 패널 대비 %) ===")
        for label, box in PANELS.items():
            if label not in used:
                continue
            ma = elements(M, box)
            ca = elements(C, used[label])
            pairs, missing, extra = match_elements(ma, ca)
            print()
            print("[%s] 시안 %d개 / 캡처 %d개" % (label, len(ma), len(ca)))
            for e, f in pairs:
                dx, dy = f["x"] - e["x"], f["y"] - e["y"]
                dw, dh = f["w"] - e["w"], f["h"] - e["h"]
                bad = max(abs(dx), abs(dy)) > 6 or max(abs(dw), abs(dh)) > 8
                print("   %s 위치(%5.1f,%5.1f)->(%5.1f,%5.1f) 어긋남(%+5.1f,%+5.1f)"
                      "  크기(%5.1f x%5.1f) 차(%+5.1f,%+5.1f)"
                      % ("!!" if bad else "  ", e["x"], e["y"], f["x"], f["y"],
                         dx, dy, e["w"], e["h"], dw, dh))
            for e in missing:
                print("   빠짐  시안에 있는데 캡처에 없음 (%5.1f,%5.1f) %5.1fx%5.1f"
                      % (e["x"], e["y"], e["w"], e["h"]))
            for f in extra:
                print("   남음  캡처에만 있음 (%5.1f,%5.1f) %5.1fx%5.1f"
                      % (f["x"], f["y"], f["w"], f["h"]))

    if args.json:
        with io.open(args.json, "w", encoding="utf-8") as handle:
            json.dump(report, handle, ensure_ascii=False, indent=2)
        print("수치 저장:", args.json)

    if args.boxes:
        sheet = Image.new("RGB", (1920, 2170), (16, 16, 20))
        sheet.paste(Image.fromarray(M.astype(np.uint8)), (0, 0))
        sheet.paste(Image.fromarray(C.astype(np.uint8)), (0, 1090))
        draw = ImageDraw.Draw(sheet)
        for label, box in PANELS.items():
            draw.rectangle(box, outline=(0, 255, 120), width=3)
            if label in used:
                x0, y0, x1, y1 = used[label]
                draw.rectangle((x0, y0 + 1090, x1, y1 + 1090),
                               outline=(255, 90, 0), width=3)
        sheet.save(args.boxes)
        print("영역 그림 저장:", args.boxes, "-- 숫자보다 이걸 먼저 볼 것")


if __name__ == "__main__":
    sys.exit(main())
