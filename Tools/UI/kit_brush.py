"""Apply a Concept A kit part to a UMG Image, with the right 9-slice margin.

왜 헬퍼가 필요한가
------------------
텍스처만 갈아 끼우면 안 된다. 나무 틀에 금 모서리가 박힌 부품을 통짜 Image 로
늘리면 모서리 장식까지 같이 늘어나 뭉개진다. Slate 의 Box 브러시로 그려야
모서리는 그대로 두고 가운데만 늘어난다.

**투명 여백을 잊으면 마진이 어긋난다.** `slice_component_kit.py` 는 잘라낸
그림 둘레에 8px 투명 여백을 남겼다. 그래서 임포트된 텍스처는 실측한 그림보다
가로세로 16px 씩 크다(748x148 -> 764x164). 9-slice 마진은 **여백까지 포함한
자리**를 가리켜야 하므로

    마진픽셀 = 여백 8 + 실측 테두리
    마진값   = 마진픽셀 / 텍스처크기          (Box 브러시 마진은 0~1 비율이다)

이걸 안 하면 테두리가 8px 씩 밀려 모서리 장식 한 줄이 늘어나는 쪽에 들어간다.
"""

import sys
from pathlib import Path

import unreal

sys.path.insert(0, str(Path(__file__).resolve().parent))

from kit_manifest_a import (  # noqa: E402
    DIR_BY_NAME, FRAME_PARTS, PAD_BY_NAME, PANEL_PARTS, PARTS,
    inner_ratio, inner_source)

KIT_DIR = "/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/KitA"

# 프로젝트가 쓰는 글꼴.
#
# 전투 HUD 는 이걸 쓰는데(RoundText · MercenaryDetailName …) 파이썬으로 만든
# 화면은 엔진 기본 Roboto 로 남아 있었다. 한 게임에서 글꼴이 둘이면 같은 값도
# 다른 화면처럼 읽힌다.
PROJECT_FONT = "/Game/SVN/OutSideAsset/Fonts/F_HUD_Oswald.F_HUD_Oswald"


def project_font(font_info, size, face="Bold"):
    """글꼴 구조체에 프로젝트 글꼴과 크기를 넣는다. 못 찾으면 크기만 바꾼다."""
    font_object = unreal.load_object(None, PROJECT_FONT)
    if font_object is not None:
        font_info.set_editor_property("font_object", font_object)
        font_info.set_editor_property("typeface_font_name", face)
    font_info.set_editor_property("size", size)
    return font_info


PAD = 8.0           # slice_component_kit.py 가 남긴 투명 여백


def _set_size(brush, width, height):
    """브러시의 ImageSize 를 원본 크기로 적어 둔다.

    **이 값은 테두리 두께와 아무 상관이 없다.** 9-slice 테두리는 실제 텍스처
    크기로 그려진다(ElementBatcher.cpp:857 · ResourceProxy->ActualSize).
    한 번 이 값으로 테두리를 얇게 만들 수 있다고 믿고 고쳤다가 틀렸다.
    ImageSize 는 슬롯이 크기를 스스로 정할 때(auto size) 쓰는 기본값이다.

    5.7 에서 이 값은 ``DeprecateSlateVector2D`` 라 ``Vector2D`` 를 넣으면
    "Cannot nativize" 로 죽고, 생성자도 인자를 안 받는다. 구조체를 읽어
    **글로 채워 넣는 것**만 된다.
    """
    size = brush.get_editor_property("image_size")
    size.import_text(f"(X={float(width):.6f},Y={float(height):.6f})")
    brush.set_editor_property("image_size", size)


# 테두리가 놓을 자리의 짧은 변에서 이만큼을 넘으면 "그 자리에 안 맞는 그림" 이다.
BORDER_SHARE = 0.10
_FAT = []


def border_report():
    """자리에 비해 테두리가 두꺼웠던 것들. 빌더가 끝에 찍어 보라고 남긴다."""
    return list(_FAT)


def check_border(name, target):
    """이 그림을 이 크기에 놓으면 테두리가 몇 %인가.

    **9-slice 테두리는 못 얇게 만든다.** 엔진이 그리는 두께는

        테두리픽셀 = 텍스처크기 x 마진비율            (ElementBatcher.cpp:857)

    이고, 여기 텍스처크기는 브러시의 ImageSize 가 아니라 **실제 텍스처 크기**다
    (ResourceProxy->ActualSize). ImageSize 를 줄여도 테두리는 안 줄어든다 --
    한 번 그렇게 믿고 고쳤다가 틀렸다. 마진을 줄이면 얇아지긴 하는데, 그건
    모서리 장식을 잘라 내는 것이라 더 못생겨진다.

    그러니 얇게 하는 길은 하나뿐이다: **테두리가 원래 얇은 그림을 고르는 것.**
    1024px 짜리 판의 55px 테두리는 371px 열에 놓으면 열 폭의 15% 가 된다.
    그런 자리에는 22px 짜리 그림을 가져와야 한다.
    """
    entry = _BY_NAME.get(name)
    if entry is None or target is None:
        return 0.0
    draw, margin = entry
    if draw != "box":
        return 0.0
    pad = PAD_BY_NAME.get(name, PAD)
    thickest = max(pad + value for value in margin)
    share = thickest / max(1.0, min(target))
    if share > BORDER_SHARE:
        _FAT.append(f"{name} 을 {target[0]:.0f}x{target[1]:.0f} 에 놓으면 "
                    f"테두리가 {thickest:.0f}px -- 짧은 변의 {share * 100:.0f}%")
    return share


# 이름 -> (그리는 방식, 실측 여백(좌,상,우,하))
_BY_NAME = {name: (draw, margin) for _index, name, draw, margin, _use in PARTS}
_BY_NAME.update({name: (draw, margin) for name, draw, margin, _pad, _use in FRAME_PARTS})
_BY_NAME.update({name: (draw, margin)
                 for name, draw, margin, _pad, _dir, _use in PANEL_PARTS})


def kit_texture(name):
    """부품 텍스처. 없으면 None -- 부르는 쪽이 옛 그림을 그대로 두면 된다."""
    folder = DIR_BY_NAME.get(name, KIT_DIR)
    return unreal.load_object(None, f"{folder}/{name}.{name}")


def apply_kit(image, name, target=None):
    """Image 위젯에 부품을 입힌다. 9-slice 여부와 마진까지 같이 잡는다.

    @param target 이 부품을 그릴 크기 (w, h). 주면 테두리가 그 자리에 맞게
                  얇아진다. 안 주면 원본 두께 그대로다.
    @return 입혔으면 True. 텍스처가 없으면 False 를 주고 아무것도 안 바꾼다.
    """
    entry = _BY_NAME.get(name)
    texture = kit_texture(name)
    if entry is None or texture is None:
        return False

    draw, margin = entry
    pad = PAD_BY_NAME.get(name, PAD)
    width = float(texture.blueprint_get_size_x())
    height = float(texture.blueprint_get_size_y())
    brush = image.get_editor_property("brush")
    brush.set_editor_property("resource_object", texture)
    _set_size(brush, width, height)
    check_border(name, target)

    if draw == "box" and width > 0.0 and height > 0.0:
        left, top, right, bottom = margin
        brush.set_editor_property("draw_as", unreal.SlateBrushDrawType.BOX)
        brush.set_editor_property("margin", unreal.Margin(
            (pad + left) / width, (pad + top) / height,
            (pad + right) / width, (pad + bottom) / height))
    else:
        # 늘리지 않는 것들. 마진을 0 으로 되돌려야 옛 값이 안 남는다.
        brush.set_editor_property("draw_as", unreal.SlateBrushDrawType.IMAGE)
        brush.set_editor_property("margin", unreal.Margin(0.0, 0.0, 0.0, 0.0))

    brush.set_editor_property("tint_color", unreal.SlateColor(
        unreal.LinearColor(1.0, 1.0, 1.0, 1.0)))
    image.set_editor_property("brush", brush)
    image.set_color_and_opacity(unreal.LinearColor(1.0, 1.0, 1.0, 1.0))
    return True


def make_kit_brush(name):
    """부품 하나를 새 브러시로. 없으면 None.

    ``apply_kit`` 은 Image 위젯이 이미 들고 있는 브러시를 고치지만, 버튼
    스타일에는 넣을 브러시가 따로 필요하다. 그리는 방식과 마진을 정하는
    규칙은 하나뿐이어야 하므로 여기 모아 둔다.
    """
    entry = _BY_NAME.get(name)
    texture = kit_texture(name)
    if entry is None or texture is None:
        return None

    draw, margin = entry
    pad = PAD_BY_NAME.get(name, PAD)
    width = float(texture.blueprint_get_size_x())
    height = float(texture.blueprint_get_size_y())

    brush = unreal.SlateBrush()
    brush.set_editor_property("resource_object", texture)
    if draw == "box" and width > 0.0 and height > 0.0:
        left, top, right, bottom = margin
        brush.set_editor_property("draw_as", unreal.SlateBrushDrawType.BOX)
        brush.set_editor_property("margin", unreal.Margin(
            (pad + left) / width, (pad + top) / height,
            (pad + right) / width, (pad + bottom) / height))
    else:
        brush.set_editor_property("draw_as", unreal.SlateBrushDrawType.IMAGE)
        brush.set_editor_property("margin", unreal.Margin(0.0, 0.0, 0.0, 0.0))
    brush.set_editor_property("tint_color", unreal.SlateColor(
        unreal.LinearColor(1.0, 1.0, 1.0, 1.0)))
    return brush


def apply_kit_button(button, normal, hovered=None, disabled=None):
    """버튼에 부품 그림을 입힌다.

    안 입히면 슬레이트 기본 스타일(밝은 회색 사각)이 그려진다. 런타임에
    만든 버튼이 상세창 오른쪽에 회색 판으로 떠 있던 원인이다.

    @return 입혔으면 True.
    """
    base = make_kit_brush(normal)
    if base is None:
        return False
    over = make_kit_brush(hovered) if hovered else None
    off = make_kit_brush(disabled) if disabled else None

    style = button.get_editor_property("widget_style")
    style.set_editor_property("normal", base)
    style.set_editor_property("hovered", over if over is not None else base)
    # 눌림은 따로 그림이 없다. 고른 칸 그림을 조금 어둡게 써서 눌린 티만 낸다.
    pressed = make_kit_brush(hovered or normal)
    pressed.set_editor_property("tint_color", unreal.SlateColor(
        unreal.LinearColor(0.80, 0.80, 0.78, 1.0)))
    style.set_editor_property("pressed", pressed)
    style.set_editor_property("disabled", off if off is not None else base)
    button.set_editor_property("widget_style", style)
    return True


def stretchable(name):
    """늘려도 되는 그림인가.

    부품 시트가 ``box`` 로 적어 둔 것만 늘려도 된다 -- 9-slice 라 가운데만
    늘어난다. ``image`` 로 적힌 것(체크박스 · 슬라이더 손잡이 · 칩 테두리 ·
    명패)은 통짜라, 칸에 그대로 맞추면 눌리거나 늘어난다.
    """
    entry = _BY_NAME.get(name)
    return entry is not None and entry[0] == "box"


def kit_source_size(name):
    """그림의 원본 크기. 비율을 지켜 놓을 때 쓴다."""
    source = inner_source(name)
    if source is not None:
        return (float(source[0]), float(source[1]))
    texture = kit_texture(name)
    if texture is None:
        return None
    width = float(texture.blueprint_get_size_x())
    height = float(texture.blueprint_get_size_y())
    return (width, height) if width > 0.0 and height > 0.0 else None


def inner_rect(name, x, y, width, height, fallback=0.12):
    """부품을 (x,y,width,height) 에 놓았을 때 **안에 글자를 넣어도 되는 사각**.

    실측한 비율(measure_inner_rect.py)을 그대로 쓴다. 못 잰 부품은 사방
    fallback 비율만큼 안으로 들인다 -- 짐작이지만, 짐작이라는 걸 여기 한 곳에만
    두려는 것이다. 전에는 부르는 자리마다 제각각 짐작하고 있었다.
    """
    ratio = inner_ratio(name)
    if ratio is None:
        pad_x, pad_y = width * fallback, height * fallback
        return (x + pad_x, y + pad_y, width - pad_x * 2, height - pad_y * 2)

    left, top, right, bottom = ratio
    draw = _BY_NAME.get(name, ("image", None))[0]
    source = inner_source(name)

    if draw != "box" or source is None:
        # 통짜로 그리는 것들. 그림 전체가 같은 배율로 늘어나므로 비율이 맞다.
        return (x + width * left, y + height * top,
                width * (right - left), height * (bottom - top))

    # 9-slice 는 **테두리를 원본 픽셀 크기로** 그린다. 가운데만 늘어난다.
    # 그래서 안쪽 자리도 늘어나는 게 아니라 가장자리에서 늘 같은 픽셀만큼
    # 떨어져 있다. 비율을 그대로 곱하면 크게 틀린다 --  526x140 짜리 받침을
    # 411x875 열에 늘려 쓸 때, 재 본 23.5%~76.5% 를 곱하면 위아래로 206px 씩
    # 비우지만 실제 나무는 33px 뿐이다. 반대로 좌우는 66px 로 계산해 놓고
    # 나무는 70px 이라 글자가 나무를 밟는다.
    source_w, source_h = source
    insets = [left * source_w, top * source_h,
              (1.0 - right) * source_w, (1.0 - bottom) * source_h]

    # 놓을 자리가 원본보다 좁으면 테두리끼리 겹친다. 그때는 Slate 도 줄여
    # 그리므로 같은 비율로 줄인다 -- 안 그러면 폭이 음수가 된다.
    for axis, span in ((0, width), (1, height)):
        total = insets[axis] + insets[axis + 2]
        if total > span * 0.8 and total > 0.0:
            shrink = span * 0.8 / total
            insets[axis] *= shrink
            insets[axis + 2] *= shrink

    return (x + insets[0], y + insets[1],
            width - insets[0] - insets[2], height - insets[1] - insets[3])
