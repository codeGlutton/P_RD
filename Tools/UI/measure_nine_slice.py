"""Measure the 9-slice margins of each sliced UI part.

왜 재나
-------
나무 틀에 금 모서리가 박힌 부품을 통짜 Image 로 늘리면 모서리 장식까지 같이
늘어나 뭉개진다. Slate 의 Box 브러시(9-slice)로 그리면 모서리는 그대로 두고
가운데만 늘어난다. 그러려면 **모서리가 어디서 끝나는지** 픽셀로 알아야 한다.

어떻게 재나
-----------
가운데 가로줄과 세로줄의 색이 얼마나 변하는지를 본다. 모서리·테두리 구간은
색이 크게 흔들리고, 늘려도 되는 가운데는 거의 평평하다. 바깥에서 안쪽으로
들어가며 **변화가 잦아드는 첫 지점**을 테두리의 끝으로 잡는다.

기계가 재는 값이라 완벽하지 않다. 그래서 결과에 신뢰도를 같이 적는다 --
가운데가 충분히 평평하지 않으면(무늬가 강하면) 사람이 봐야 한다.

결과는 ``Saved/UIKit/<컨셉>/_nineslice.txt`` 로 나가고, 그 숫자가 곧
``frame_registry.py`` 와 빌더가 쓰는 브러시 마진이 된다.

쓰는 법:
    python Tools/UI/measure_nine_slice.py Saved/UIKit/ConceptA
"""

import argparse
import sys
from pathlib import Path

try:
    from PIL import Image
except ImportError:  # pragma: no cover
    sys.exit("PIL 이 없다: python -m pip install pillow")

PAD = 8                 # slice_component_kit.py 가 남긴 투명 여백
FLAT = 10.0             # 이웃 픽셀 차이가 이보다 작으면 평평하다고 본다
RUN = 6                 # 평평한 상태가 이만큼 이어져야 테두리가 끝난 것으로 본다


def diffs(pixels):
    """이웃한 두 픽셀의 RGB 차이 합. 색이 튀는 곳을 찾는 데 쓴다."""
    out = []
    for index in range(len(pixels) - 1):
        left, right = pixels[index], pixels[index + 1]
        out.append(sum(abs(left[channel] - right[channel]) for channel in range(3)))
    return out


def edge_end(series):
    """바깥에서 안으로 들어가며 변화가 잦아드는 첫 지점을 돌려준다."""
    calm = 0
    for index, value in enumerate(series):
        if value <= FLAT:
            calm += 1
            if calm >= RUN:
                return max(0, index - RUN + 1)
        else:
            calm = 0
    return len(series) // 3          # 못 찾으면 1/3 로 둔다(늘 안전한 쪽)


def measure(path):
    image = Image.open(path).convert("RGBA")
    width, height = image.size
    # 투명 여백을 뺀 진짜 그림 영역만 본다.
    box = (PAD, PAD, width - PAD, height - PAD)
    art = image.crop(box)
    art_w, art_h = art.size
    if art_w < 12 or art_h < 12:
        return None

    row = [art.getpixel((x, art_h // 2)) for x in range(art_w)]
    column = [art.getpixel((art_w // 2, y)) for y in range(art_h)]

    left = edge_end(diffs(row))
    right = edge_end(diffs(row[::-1]))
    top = edge_end(diffs(column))
    bottom = edge_end(diffs(column[::-1]))

    # 대칭으로 맞춘다.
    #
    # 채움면에 무늬(양피지 결·나뭇결)가 있으면 그쪽 변에서 "평평해지는 지점"을
    # 한참 안쪽에서야 찾는다. 실제로 part_11 은 왼쪽 여백이 폭의 68% 로 나왔다 --
    # 그대로 쓰면 늘릴 가운데가 거의 없어져 9-slice 가 무의미해진다.
    # 이 부품들은 눈으로 봐도 좌우·상하가 같은 모양이므로 **작은 쪽**을 택한다.
    # 작게 잡아 틀리면 모서리가 조금 늘어날 뿐이지만, 크게 잡아 틀리면 그림이
    # 통째로 뭉개진다.
    left = right = min(left, right)
    top = bottom = min(top, bottom)

    # 가운데는 최소 40% 를 남긴다. 여백이 이보다 크면 늘릴 데가 없다.
    left = right = min(left, int(art_w * 0.30))
    top = bottom = min(top, int(art_h * 0.30))

    # 정사각에 가까운 작은 부품은 9-slice 대상이 아니다. 체크박스·칩 테두리·
    # 슬라이더 손잡이는 늘 같은 크기로 그리고, 늘리면 동그라미가 타원이 된다.
    fixed = abs(art_w - art_h) <= max(art_w, art_h) * 0.12 and max(art_w, art_h) <= 200

    middle = row[left:art_w - right]
    spread = max(diffs(middle)) if len(middle) > 2 else 999
    return dict(size=(art_w, art_h), margin=(left, top, right, bottom),
                flat=spread <= FLAT * 3, spread=spread, fixed=fixed)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("folder")
    args = parser.parse_args()
    folder = Path(args.folder)

    lines = [f"# {folder.name} 9-slice 실측",
             "# 여백은 그림 영역 기준(투명 패딩 제외). Slate Box 브러시 Margin 은",
             "# 이 값을 그림 크기로 나눈 비율이다.",
             "# 부품  크기        여백(좌,상,우,하)   가운데평평  최대변화"]
    for path in sorted(folder.glob("part_*.png")):
        result = measure(path)
        if result is None:
            lines.append(f"{path.stem}  너무 작아 못 잼")
            continue
        w, h = result["size"]
        l, t, r, b = result["margin"]
        kind = "고정(Image)" if result["fixed"] else "Box"
        lines.append(
            f"{path.stem}  {w:4d}x{h:4d}  ({l:3d},{t:3d},{r:3d},{b:3d})"
            f"  {kind:12s} 평평={'예' if result['flat'] else '아니오':4s}"
            f"  margin=({l/w:.3f},{t/h:.3f},{r/w:.3f},{b/h:.3f})")

    out = folder / "_nineslice.txt"
    out.write_text("\n".join(lines) + "\n", encoding="utf-8")
    print("\n".join(lines))


if __name__ == "__main__":
    main()
