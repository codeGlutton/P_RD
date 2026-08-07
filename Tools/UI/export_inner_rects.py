"""Write the measured part rects out as a C++ header.

왜
--
자리를 재는 값은 한 곳에만 있어야 한다. 파이썬 빌더는 ``kit_manifest_a.INNER``
를 보는데, 몬스터탭·용병탭 빌더는 C++ 이라 그 표를 못 읽는다. 그래서 그쪽은
``LeftCell.X + 24.0f`` 처럼 숫자를 손으로 적어 두고 있었고, 그 24 는 아무도
재 본 적이 없는 값이었다.

여기서 헤더 하나를 뽑아 두면 두 쪽이 같은 값을 본다. 사람이 목록 페이지에서
칸을 고치면 ``apply_user_rects.py`` 가 파이썬 표를 고치고, 이 스크립트가 그걸
헤더로 옮긴다.

무엇을 넣나
-----------
C++ 빌더가 실제로 쓰는 그림만 넣는다. 154개를 다 넣으면 어느 것이 쓰이는지
헤더만 봐서는 알 수 없다.

Run with plain python:
    python Tools/UI/export_inner_rects.py
"""

import json
import re
from pathlib import Path

ROOT = Path("D:/UnrealProjects/P_RD_develop_20260803")
USER = ROOT / "Tools/UI/mockups/rects_user.json"
MEASURED = ROOT / "Tools/UI/mockups/assets.json"
HEADER = ROOT / "Source/P_RDEditor/UI/UIPartRects.h"
BUILDERS = [
    ROOT / "Source/P_RDEditor/UI/MonsterTabWidgetBuilder.cpp",
    ROOT / "Source/P_RDEditor/UI/MarchboundHireWidgetBuilder.cpp",
    ROOT / "Source/P_RDEditor/UI/CombatHUDWidgetBuilder.cpp",
    ROOT / "Source/P_RDEditor/UI/RewardSettlementWidgetBuilder.cpp",
    ROOT / "Source/P_RDEditor/UI/CombatDefeatWidgetBuilder.cpp",
]

TEMPLATE = '''// 이 파일은 만들어진 것이다. 손으로 고치지 말 것.
//   Tools/UI/export_inner_rects.py 가 다시 만든다.
//
// 값의 출처는 사람이 목록 페이지(Tools/UI/mockups/assets.html)에서 눈으로 맞춘
// **칸**이다 -- 그림 안에 글자·아이콘을 놓아도 테두리를 안 밟는 자리.
// 안 맞춘 것은 잰 값(measure_all_assets.py)이다.

#pragma once

#include "CoreMinimal.h"

namespace UIPartRects
{{
\t/** @brief 그림 한 장의 잰 값. 비율은 **원본 텍스처 기준**이다. */
\tstruct FPart
\t{{
\t\tconst TCHAR* Name;
\t\tint32 Index;                      // 칸이 여럿인 그림이 있다(머리칸 + 몸통)
\t\tfloat Left, Top, Right, Bottom;   // 쓸 수 있는 칸 (0~1)
\t\tfloat SourceW, SourceH;           // 그 비율을 잰 원본 크기
\t}};

\tstatic const FPart Parts[] = {{
{rows}
\t}};

\t/**
\t * @brief 그림을 (X,Y,W,H) 에 놓았을 때 **안에 글자를 넣어도 되는 사각**.
\t *
\t * @param bSliced true 면 9-slice(Box)로 그린다는 뜻이다. 그때 테두리는
\t *                늘어나지 않고 **원본 픽셀 크기 그대로** 그려지므로
\t *                (ElementBatcher.cpp:857), 안쪽 자리도 가장자리에서 늘 같은
\t *                픽셀만큼 떨어져 있다. 비율을 그대로 곱하면 크게 틀린다.
\t *                통짜(Image)로 그리면 전체가 같은 배율이라 비율이 맞다.
\t * @return 못 찾으면 사방 12% 를 들인 사각. 짐작이지만 한 곳에만 둔다.
\t */
\tinline FBox2D Inner(const TCHAR* Name, const FVector2D Position,
\t\tconst FVector2D Size, const bool bSliced = true, const int32 Which = 0)
\t{{
\t\tfor (const FPart& Part : Parts)
\t\t{{
\t\t\tif (FCString::Strcmp(Part.Name, Name) != 0 || Part.Index != Which)
\t\t\t{{
\t\t\t\tcontinue;
\t\t\t}}
\t\t\tif (bSliced == false)
\t\t\t{{
\t\t\t\treturn FBox2D(
\t\t\t\t\tPosition + FVector2D(Size.X * Part.Left, Size.Y * Part.Top),
\t\t\t\t\tPosition + FVector2D(Size.X * Part.Right, Size.Y * Part.Bottom));
\t\t\t}}
\t\t\tFVector2D Near(Part.Left * Part.SourceW, Part.Top * Part.SourceH);
\t\t\tFVector2D Far((1.f - Part.Right) * Part.SourceW,
\t\t\t\t(1.f - Part.Bottom) * Part.SourceH);
\t\t\t// 놓을 자리가 원본보다 좁으면 테두리끼리 겹친다. 엔진도 그때는
\t\t\t// 반씩으로 자르므로 여기서도 같은 비율로 줄인다.
\t\t\tif (Near.X + Far.X > Size.X * 0.8f)
\t\t\t{{
\t\t\t\tconst float Shrink = Size.X * 0.8f / (Near.X + Far.X);
\t\t\t\tNear.X *= Shrink;
\t\t\t\tFar.X *= Shrink;
\t\t\t}}
\t\t\tif (Near.Y + Far.Y > Size.Y * 0.8f)
\t\t\t{{
\t\t\t\tconst float Shrink = Size.Y * 0.8f / (Near.Y + Far.Y);
\t\t\t\tNear.Y *= Shrink;
\t\t\t\tFar.Y *= Shrink;
\t\t\t}}
\t\t\treturn FBox2D(Position + Near, Position + Size - Far);
\t\t}}
\t\tconst FVector2D Guess = Size * 0.12f;
\t\treturn FBox2D(Position + Guess, Position + Size - Guess);
\t}}

\t/** @brief 이 그림에 사람이 그어 둔 칸이 몇 개인가. */
\tinline int32 Count(const TCHAR* Name)
\t{{
\t\tint32 Found = 0;
\t\tfor (const FPart& Part : Parts)
\t\t{{
\t\t\tif (FCString::Strcmp(Part.Name, Name) == 0)
\t\t\t{{
\t\t\t\t++Found;
\t\t\t}}
\t\t}}
\t\treturn Found;
\t}}

\t/**
\t * @brief 여러 칸으로 나눠 쓰는 그림의 Which 번째 칸.
\t *
\t * @details
\t * **그어 둔 칸이 있으면 그것을 쓴다.** 그림에 칸막이가 그려져 있으면 사람이
\t * 그 수만큼 그어 두었을 것이고, 그게 정본이다. 기계가 그림에서 칸막이를
\t * 찾아내는 짓은 하지 않는다 -- 재는 곳이 둘이 되면 어느 쪽이 맞는지 따지게
\t * 되고, 사람이 손으로 다 맞춰 둔 뜻이 없어진다.
\t *
\t * 하나만 그어져 있으면 그 하나를 Total 등분한다. 칸막이가 없는 그림이거나,
\t * 아직 안 나눠 그은 그림이다. 어느 쪽이든 칸을 더 그으면 여기가 따라온다.
\t */
\tinline FBox2D Cell(const TCHAR* Name, const FVector2D Position,
\t\tconst FVector2D Size, const bool bSliced, const int32 Which,
\t\tconst int32 Total, const bool bHorizontal = true)
\t{{
\t\tif (Count(Name) >= Total)
\t\t{{
\t\t\treturn Inner(Name, Position, Size, bSliced, Which);
\t\t}}
\t\tconst FBox2D Whole = Inner(Name, Position, Size, bSliced, 0);
\t\tconst FVector2D Span = Whole.GetSize();
\t\tif (bHorizontal)
\t\t{{
\t\t\tconst float Step = Span.X / Total;
\t\t\treturn FBox2D(Whole.Min + FVector2D(Step * Which, 0.f),
\t\t\t\tWhole.Min + FVector2D(Step * (Which + 1), Span.Y));
\t\t}}
\t\tconst float Step = Span.Y / Total;
\t\treturn FBox2D(Whole.Min + FVector2D(0.f, Step * Which),
\t\t\tWhole.Min + FVector2D(Span.X, Step * (Which + 1)));
\t}}
}}
'''


def main():
    user = json.loads(USER.read_text(encoding="utf-8")) if USER.is_file() else {}
    measured = {a["name"]: a for a in json.loads(MEASURED.read_text(encoding="utf-8"))}

    wanted = set()
    for path in BUILDERS:
        if not path.is_file():
            continue
        text = path.read_text(encoding="utf-8")
        wanted.update(re.findall(r"(T_[A-Za-z0-9_]+)\.T_", text))

    rows, missing = [], []
    for name in sorted(wanted):
        entry = measured.get(name)
        if entry is None:
            missing.append(f"{name} -- 목록에 없음")
            continue
        holes = user.get(name) or entry.get("holes") or []
        rects = [h.get("rect") or h.get("inner") or h.get("box") for h in holes]
        rects = [r for r in rects if r is not None]
        if not rects:
            missing.append(f"{name} -- 칸이 없음")
            continue
        width, height = entry["size"]
        source = "사람" if name in user else "잼"
        # 칸이 둘 이상인 그림이 있다(용병탭 파티 틀 = 머리칸 + 몸통).
        # 첫 칸만 쓰면 그런 그림은 머리칸을 못 쓴다.
        for index, rect in enumerate(rects):
            rows.append(f'		{{ TEXT("{name}"), {index}, {rect[0]:.4f}f, '
                        f'{rect[1]:.4f}f, {rect[2]:.4f}f, {rect[3]:.4f}f, '
                        f'{width}.f, {height}.f }},   // {source}')

    HEADER.write_text(TEMPLATE.format(rows="\n".join(rows)), encoding="utf-8")
    print(f"{HEADER.name}: {len(rows)}개")
    for line in missing:
        print("  자리 없음:", line)


if __name__ == "__main__":
    main()
