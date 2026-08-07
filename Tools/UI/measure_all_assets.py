"""Measure every exported UI texture, and make thumbnails for the catalogue.

무엇을 재나 (앞서 만든 세 도구를 한 번에)
------------------------------------------
    실루엣      투명 여백을 뺀 진짜 그림 범위
    구멍 전부   속이 빈 칸. 하나가 아닌 그림이 많다(3열 틀은 셋)
    구멍 안쪽   그 구멍에 들어가는 가장 큰 사각. 둥근 구멍이면 겉보다 훨씬 작다
    9-slice     늘려도 장식이 안 뭉개지는 가장 작은 마진. 없으면 "부적합"

왜 numpy 인가
-------------
부품 하나 재는 데 쓰던 순수 파이썬은 597장에 몇 시간 걸린다. 마진 찾기가
후보 쌍마다 넓은 구역의 평균을 다시 구하기 때문이다.

**합 배열(적분 영상)** 을 한 번 만들어 두면 어떤 사각의 합도 네 번 더하기로
끝난다. 후보를 아무리 많이 봐도 구역 크기와 무관해진다.

결과는 JSON 한 장. HTML 목록이 이걸 읽는다.
"""

import json
import sys
from pathlib import Path

import numpy as np
from PIL import Image

ROOT = Path("D:/UnrealProjects/P_RD_develop_20260803")
DUMP = ROOT / "Saved/UIKit/AssetDump"
THUMBS = ROOT / "Tools/UI/mockups/assets"
OUT = ROOT / "Tools/UI/mockups/assets.json"

CLEAR = 40          # 이보다 옅으면 빈 것으로 본다
TOL = 16.0          # 채널 평균 차이가 이보다 크면 한결같지 않다
RUN = 3             # 기준을 넘는 칸이 이만큼 이어지면 장식이다
SAFETY = 1.15       # 줄여서 재느라 작게 나오는 만큼 넉넉히
MAX_FRAC = 0.42
WORK = 192          # 잴 때 줄이는 크기. 합 배열 덕에 이 정도로 충분하다
THUMB = 320         # 목록에 보여 줄 크기
MIN_HOLE = 0.005    # 실루엣의 이 비율보다 작은 빈 자리는 틈이다


def integral(array):
    """왼위부터의 누적합. 어떤 사각의 합이든 네 번 더하기로 나온다."""
    return np.pad(array.cumsum(0).cumsum(1), ((1, 0), (1, 0)))


def box_sum(table, y0, y1, x0, x1):
    return table[y1, x1] - table[y0, x1] - table[y1, x0] + table[y0, x0]


def _label(mask):
    """이어진 덩어리에 번호를 매긴다. scipy 없이 가로 런 방식으로."""
    height, width = mask.shape
    labels = np.zeros((height, width), np.int32)
    parent = [0]

    def find(node):
        while parent[node] != node:
            parent[node] = parent[parent[node]]
            node = parent[node]
        return node

    def join(a, b):
        a, b = find(a), find(b)
        if a != b:
            parent[b] = a

    for y in range(height):
        row = mask[y]
        changes = np.flatnonzero(np.diff(np.concatenate(([0], row.view(np.int8), [0]))))
        for start, end in zip(changes[::2], changes[1::2]):
            above = labels[y - 1, start:end] if y else np.zeros(0, np.int32)
            found = np.unique(above[above > 0])
            if found.size:
                label = int(found[0])
                for other in found[1:]:
                    join(label, int(other))
            else:
                parent.append(len(parent))
                label = len(parent) - 1
            labels[y, start:end] = label
    if len(parent) <= 1:
        return labels, 0
    roots = np.array([find(i) for i in range(len(parent))], np.int32)
    remap = {root: index for index, root in enumerate(sorted(set(roots[1:])), start=1)}
    lookup = np.zeros(len(parent), np.int32)
    for i in range(1, len(parent)):
        lookup[i] = remap[roots[i]]
    return lookup[labels], len(remap)


def full_span_rect(mask, keep=0.90):
    """구멍에서 **모서리에 안 끌리는** 안쪽 사각.

    전에는 구멍 안에 들어가는 가장 큰 사각(내접 사각)을 썼다. 그런데 모서리가
    둥글거나 가장자리에 장식 홈이 있으면 그 한 점 때문에 사각이 확 줄어든다.
    실제로 프레임 여럿이 쓸 수 있는 자리보다 훨씬 작게 나왔다.

    사람이 보는 "쓸 수 있는 자리" 는 그런 뜻이 아니다. **거의 다 뚫려 있는 구간**
    이다. 그래서 줄마다 뚫린 폭을 세고, 가장 넓은 줄의 keep(9할) 이상인 구간만
    남긴다. 둥근 모서리가 만드는 좁아지는 끝부분만 잘려 나간다.

    한 점 때문에 사각이 줄지 않으므로 모서리 모양에 안 끌린다.
    """
    height, width = mask.shape
    row_open = mask.sum(axis=1)
    col_open = mask.sum(axis=0)
    if row_open.max() == 0 or col_open.max() == 0:
        return (0, 0, width, height)

    def span(counts, keep_ratio):
        limit = counts.max() * keep_ratio
        good = np.flatnonzero(counts >= limit)
        return (int(good[0]), int(good[-1]) + 1) if good.size else (0, len(counts))

    top, bottom = span(row_open, keep)
    left, right = span(col_open, keep)
    return (left, top, right, bottom)


def largest_rect(mask):
    """이진 배열 안의 가장 큰 사각. 히스토그램 방식.

    둥근 구멍에서는 이쪽이 더 넓다. 네모난 구멍에서는 full_span_rect 가 낫다.
    """
    height, width = mask.shape
    heights = np.zeros(width, np.int32)
    best = (0, None)
    for y in range(height):
        heights = np.where(mask[y], heights + 1, 0)
        stack = []
        for x in range(width + 1):
            current = heights[x] if x < width else 0
            while stack and heights[stack[-1]] >= current:
                top = stack.pop()
                left = stack[-1] + 1 if stack else 0
                area = int(heights[top]) * (x - left)
                if area > best[0]:
                    best = (area, (left, y - int(heights[top]) + 1, x, y + 1))
            stack.append(x)
    return best[1]


def uniform(series):
    if len(series) < 2:
        return True
    middle = series[len(series) // 2]
    gap = np.abs(series - middle).mean(axis=1)
    over = gap > TOL
    if not over.any():
        return True
    # 이어진 구간 길이. 결은 짧고 장식은 길다.
    run, longest = 0, 0
    for flag in over:
        run = run + 1 if flag else 0
        longest = max(longest, run)
    return longest < RUN


def slice_margin(rgba):
    height, width = rgba.shape[:2]
    tables = [integral(rgba[:, :, c].astype(np.float64)) for c in range(4)]

    def cols(x0, x1, y0, y1):
        """[x0,x1) 각 열의 채널 평균. 세로로 눌러 만든 가로 신호."""
        if y1 <= y0 or x1 <= x0:
            return np.zeros((0, 4))
        out = np.empty((x1 - x0, 4))
        for c in range(4):
            table = tables[c]
            line = table[y1, x0 + 1:x1 + 1] - table[y0, x0 + 1:x1 + 1] \
                - table[y1, x0:x1] + table[y0, x0:x1]
            out[:, c] = line / (y1 - y0)
        return out

    def rows(x0, x1, y0, y1):
        if y1 <= y0 or x1 <= x0:
            return np.zeros((0, 4))
        out = np.empty((y1 - y0, 4))
        for c in range(4):
            table = tables[c]
            line = table[y0 + 1:y1 + 1, x1] - table[y0 + 1:y1 + 1, x0] \
                - table[y0:y1, x1] + table[y0:y1, x0]
            out[:, c] = line / (x1 - x0)
        return out

    def passes(mx, my):
        for y0, y1 in ((0, my), (height - my, height)):
            if not uniform(cols(mx, width - mx, y0, y1)):
                return False
        for x0, x1 in ((0, mx), (width - mx, width)):
            if not uniform(rows(x0, x1, my, height - my)):
                return False
        if not uniform(cols(mx, width - mx, my, height - my)):
            return False
        if not uniform(rows(mx, width - mx, my, height - my)):
            return False
        return True

    limit_x, limit_y = max(3, int(width * MAX_FRAC)), max(3, int(height * MAX_FRAC))
    for total in range(4, limit_x + limit_y + 1):
        for mx in range(2, min(limit_x, total - 2) + 1):
            my = total - mx
            if my < 2 or my > limit_y:
                continue
            if passes(mx, my):
                return mx, my
    return None


def calm_region(array, box, margin):
    """속이 꽉 찬 틀에서 **안쪽 판**을 찾는다.

    왜 필요한가
    -----------
    구멍은 알파가 비어 있어야 찾을 수 있다. 그런데 단추·카드·띠처럼 속이 채워진
    틀은 알파가 꽉 차 있어 구멍이 하나도 안 잡힌다. 실제로 프레임 176개 중
    154개가 그랬다 -- 안쪽 박스가 아예 안 그려졌다.

    어떻게 찾나
    -----------
    장식은 무늬가 세고 채움면은 평평하다. 그래서 **이웃 픽셀과 얼마나 다른가**
    (기울기)를 재서, 조용한 구간만 남긴다. 9-slice 마진이 이미 "장식이 끝나는
    자리" 를 알려 주므로 그 안쪽에서만 본다.

    돌려주는 것은 (겉, 안) 이다.
        겉  장식이 끝나는 자리 = 9-slice 마진 안쪽
        안  그 안에서 무늬가 잦아든 구간. 글자를 놓아도 되는 자리
    """
    x0, y0, x1, y1 = box
    mx, my = margin
    # 겉: 장식 안쪽. 마진이 실루엣보다 크면 쓸 수 없다.
    bx0, by0 = x0 + mx, y0 + my
    bx1, by1 = x1 - mx, y1 - my
    if bx1 - bx0 < 8 or by1 - by0 < 8:
        return None
    # 장식이 그림의 대부분을 차지하면 틀이 아니라 그림 자체다. 아이콘에서
    # 그랬다 -- T_icon_gold 는 마진이 42%씩이라 가운데 16%만 남아, 뜻 없는
    # 자리를 안쪽이라고 내놨다. 남는 속이 절반도 안 되면 짐작하지 않는다.
    # 문턱은 뜻 없는 짐작만 막을 만큼만 둔다. 절반으로 뒀더니 얇은 단추 72개가
    # 막혔고, 1/4 로도 특이한 모양 18개가 남았다. 자리를 하나도 안 주는 것보다
    # 대충이라도 주고 사람이 고치는 편이 낫다 -- 고칠 창은 이미 있다.
    if (bx1 - bx0) < (x1 - x0) * 0.12 or (by1 - by0) < (y1 - y0) * 0.12:
        return None

    grey = array[..., :3].astype(np.float32).mean(axis=2)
    # 기울기: 가로·세로 이웃과의 차이. 장식 가장자리에서 커진다.
    grad = np.zeros_like(grey)
    grad[:, 1:] += np.abs(np.diff(grey, axis=1))
    grad[1:, :] += np.abs(np.diff(grey, axis=0))
    inside = grad[by0:by1, bx0:bx1]
    if inside.size == 0:
        return None

    # 조용함의 기준은 그림마다 다르다. 안쪽 자신의 중앙값에서 잡는다 --
    # 고정값을 쓰면 나뭇결 있는 판이 통째로 시끄럽다고 잡힌다.
    # 조용함의 기준. 빡빡하면 무늬 있는 채움면이 통째로 시끄럽다고 잡혀
    # 안쪽을 못 찾는다. 널널하게 잡고 틀리면 사람이 고친다.
    limit = float(np.median(inside)) * 3.0 + 8.0
    calm = inside <= limit
    span = full_span_rect(calm, keep=0.85)
    inner = (bx0 + span[0], by0 + span[1], bx0 + span[2], by0 + span[3])
    if inner[2] - inner[0] < 4 or inner[3] - inner[1] < 4:
        inner = (bx0, by0, bx1, by1)
    return (bx0, by0, bx1, by1), inner


def measure(path):
    image = Image.open(path).convert("RGBA")
    full_w, full_h = image.size
    scale = min(1.0, WORK / max(full_w, full_h))
    work = image if scale >= 1.0 else image.resize(
        (max(8, int(full_w * scale)), max(8, int(full_h * scale))), Image.LANCZOS)
    array = np.asarray(work)
    alpha = array[:, :, 3]
    solid = alpha >= CLEAR
    if not solid.any():
        return dict(empty=True)

    ys, xs = np.nonzero(solid)
    box = (int(xs.min()), int(ys.min()), int(xs.max()) + 1, int(ys.max()) + 1)
    bw, bh = box[2] - box[0], box[3] - box[1]

    # 구멍: 실루엣 안에서 빈 덩어리. 실루엣 테두리에 닿으면 그림 밖이다.
    inside = ~solid[box[1]:box[3], box[0]:box[2]]
    labels, count = _label(inside)
    holes = []
    for index in range(1, count + 1):
        spot = labels == index
        hy, hx = np.nonzero(spot)
        if hx.size == 0:
            continue
        a, b, c, d = int(hx.min()), int(hy.min()), int(hx.max()) + 1, int(hy.max()) + 1
        if a == 0 or b == 0 or c >= bw or d >= bh:
            continue
        if (c - a) * (d - b) < bw * bh * MIN_HOLE:
            continue
        # 두 방식을 다 재고 넓은 쪽을 쓴다.
        #
        # 9할 규칙은 **네모난 구멍**에 맞는다. 둥근 모서리 한 점 때문에 사각이
        # 줄지 않는다. 하지만 **원형 구멍**에서는 가로로 넓고 세로로 얇은 띠가
        # 나와 오히려 더 작아진다(배지에서 21%까지 떨어졌다).
        # 내접 사각은 그 반대다. 그래서 둘을 다 재고 넓은 쪽을 쓴다.
        piece = spot[b:d, a:c]
        span = full_span_rect(piece)
        fit = largest_rect(piece) or span
        area = lambda r: (r[2] - r[0]) * (r[3] - r[1])
        rect = span if area(span) >= area(fit) else fit
        inner = (a + rect[0], b + rect[1], a + rect[2], b + rect[3])
        # 사각은 하나다. 남기는 것은 **쓸 수 있는 속**이다.
        holes.append(dict(rect=[
            round((box[0] + inner[0]) / work.width, 4),
            round((box[1] + inner[1]) / work.height, 4),
            round((box[0] + inner[2]) / work.width, 4),
            round((box[1] + inner[3]) / work.height, 4)]))
    holes.sort(key=lambda h: (round(h["rect"][1], 2), h["rect"][0]))

    margin = slice_margin(array)

    # 알파로 구멍을 못 찾았으면 속이 찬 틀이다. 색으로 안쪽 판을 찾는다.
    # 이걸 안 하면 단추·카드·띠에 안쪽 박스가 아예 안 생긴다(실측 154개).
    estimated = False
    if not holes:
        # 9-slice 마진이 없으면(늘리면 안 되는 그림) 기댈 자리가 없다. 그래도
        # 틀이라면 자리가 있어야 하므로 실루엣의 1/8 을 테두리로 보고 시작한다.
        guess = margin
        if guess is None:
            guess = (max(2, int((box[2] - box[0]) * 0.12)),
                     max(2, int((box[3] - box[1]) * 0.12)))
        found = calm_region(array, box, guess)
        if found is not None:
            outer, inner = found
            estimated = True
            holes.append(dict(rect=[
                round(inner[0] / work.width, 4), round(inner[1] / work.height, 4),
                round(inner[2] / work.width, 4), round(inner[3] / work.height, 4)]))
    result = dict(
        size=[full_w, full_h],
        # 알파가 아니라 색으로 짐작한 자리라는 표시. 눈으로 확인할 자리다.
        estimated=estimated,
        silhouette=[round(box[0] / work.width, 4), round(box[1] / work.height, 4),
                    round(box[2] / work.width, 4), round(box[3] / work.height, 4)],
        holes=holes)
    if margin is None:
        result["slice"] = None
    else:
        mx, my = margin
        result["slice"] = [round(mx / work.width * SAFETY, 4),
                           round(my / work.height * SAFETY, 4)]
    return result


def main():
    THUMBS.mkdir(parents=True, exist_ok=True)
    index = json.loads((DUMP / "_index.json").read_text(encoding="utf-8"))
    out, failed = [], 0
    for position, entry in enumerate(index):
        path = DUMP / entry["png"]
        if not path.is_file():
            continue
        try:
            data = measure(path)
        except Exception as error:  # noqa: BLE001
            failed += 1
            data = dict(error=str(error))
        image = Image.open(path).convert("RGBA")
        thumb = image.copy()
        thumb.thumbnail((THUMB, THUMB), Image.LANCZOS)
        thumb.save(THUMBS / entry["png"], optimize=True)
        out.append({**entry, **data})
        if position % 50 == 0:
            print(f"{position}/{len(index)}", flush=True)

    OUT.write_text(json.dumps(out, ensure_ascii=False), encoding="utf-8")
    print(f"잰 텍스처 {len(out)}개 / 실패 {failed}개 -> {OUT}")


if __name__ == "__main__":
    main()
