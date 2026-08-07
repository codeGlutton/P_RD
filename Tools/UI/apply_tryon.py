"""브라우저에서 갈아 끼워 본 것을 **실제 WBP 에 물린다.**

무엇을 읽나
-----------
``mockups/tryon.json`` 이 사람이 고른 결과다. 열쇠는

    /Game/UI/WBP_TitleMenu/StartButtonFrameImage__base_16_9  ->  T_Hire_TitleBoard_V13

이고, 그림 이름은 ``mockups/assets.json`` 이 실제 경로와 9-slice 여백까지
들고 있다. 여백은 그림에서 **뚫린 자리**(가운데 빈 칸)를 재서 구한 값이라,
손으로 짐작해 적을 일이 없다.

Button 을 골랐으면 그 밑의 FrameImage 에 넣는다
-----------------------------------------------
타이틀은 같은 자리에 Button 과 FrameImage 가 겹쳐 있다. 그림은 FrameImage 가
지고 Button 은 누르는 자리만 맡는다. 브라우저에서는 Button 이 위에 있어
그쪽이 눌리므로, 여기서 다시 밑의 FrameImage 로 돌려준다 -- Button 스타일에
넣으면 밑의 FrameImage 와 두 장이 겹친다.

FrameImage 가 없는 Button 은 스타일에 직접 넣는다(눌림은 조금 어둡게).

Run headless:
    UnrealEditor-Cmd.exe <project> -run=pythonscript
        -script="Tools/UI/apply_tryon.py"
"""

import json
from pathlib import Path

import unreal

ROOT = Path("D:/UnrealProjects/P_RD_develop_20260803")
MOCKUPS = ROOT / "Tools/UI/mockups"
RESULT = ROOT / "Saved/LegacyAudit/apply_tryon.txt"

NONE = "__none__"          # "그림 없앰"
WHITE = unreal.LinearColor(1.0, 1.0, 1.0, 1.0)

# 글꼴 이름 -> 에셋. export_text_style.py 와 같은 목록이어야 한다.
FONTS = {
    "F_HUD_Oswald": "/Game/SVN/OutSideAsset/Fonts/F_HUD_Oswald.F_HUD_Oswald",
    "F_HUD_LINESeedKR":
        "/Game/SVN/OutSideAsset/Fonts/F_HUD_LINESeedKR.F_HUD_LINESeedKR",
    "F_HUD_NotoSansKR":
        "/Game/SVN/OutSideAsset/Fonts/F_HUD_NotoSansKR.F_HUD_NotoSansKR",
    "Roboto": "/Engine/EngineFonts/Roboto.Roboto",
}

JUSTIFY = {"LEFT": unreal.TextJustify.LEFT, "CENTER": unreal.TextJustify.CENTER,
           "RIGHT": unreal.TextJustify.RIGHT}
VERTICAL = {"TOP": 0.0, "CENTER": 0.5, "BOTTOM": 1.0}
HORIZONTAL = {"LEFT": 0.0, "CENTER": 0.5, "RIGHT": 1.0}

LINES = []


def say(text):
    LINES.append(text)
    unreal.log(text)


def catalogue():
    """그림 이름 -> (에셋 경로, 9-slice 여백 비율(x, y))."""
    data = json.loads((MOCKUPS / "assets.json").read_text(encoding="utf-8"))
    book = {}
    for item in data:
        book[item["name"]] = (item["asset"], item.get("slice"))
    return book


def widget_of(blueprint, name):
    tree = unreal.find_object(None, blueprint.get_path_name() + ":WidgetTree")
    if tree is None:
        return None
    return unreal.find_object(None, f"{tree.get_path_name()}.{name}")


def frame_of(blueprint, name):
    """Button 이름에 맞는 FrameImage 위젯. 없으면 None.

    ``ContinueButton__base_16_9`` -> ``ContinueButtonFrameImage__base_16_9``
    """
    head, sep, tail = name.partition("__")
    candidate = f"{head}FrameImage{sep}{tail}"
    found = widget_of(blueprint, candidate)
    return (candidate, found) if isinstance(found, unreal.Image) else (None, None)


def button_of(blueprint, name):
    """FrameImage 이름에 맞는 Button. 없으면 None.

    ``frame_of`` 의 반대다. 사람이 그림칸을 바로 골랐을 때도 그 위의 Button 이
    덮고 있으면 소용이 없으므로, 양쪽에서 다 찾아야 한다.
    """
    head, sep, tail = name.partition("__")
    if not head.endswith("FrameImage"):
        return None, None
    candidate = f"{head[:-len('FrameImage')]}{sep}{tail}"
    found = widget_of(blueprint, candidate)
    return (candidate, found) if isinstance(found, unreal.Button) else (None, None)


def covers(button):
    """이 버튼이 밑을 덮고 있나.

    그림 없는 브러시라도 ``draw_as`` 가 NoDrawType 이 아니면 단색으로 칠한다.
    타이틀 단추가 ROUNDED_BOX 알파 1.0 이었다.
    """
    normal = button.get_editor_property("widget_style").get_editor_property("normal")
    if normal.get_editor_property("draw_as") == unreal.SlateBrushDrawType.NO_DRAW_TYPE:
        return False
    tint = normal.get_editor_property("tint_color").get_editor_property(
        "specified_color")
    return tint.a > 0.02


def slot_size(widget):
    """이 위젯이 그려질 크기. 못 알아내면 None.

    CanvasPanelSlot 의 offsets 는 앵커가 한 점일 때만 크기다. 늘려 놓은
    것이면 화면에서 뺀 값이라 여기서는 모른다고 답한다.
    """
    slot = widget.get_editor_property("slot")
    if not isinstance(slot, unreal.CanvasPanelSlot):
        return None
    anchors = slot.get_anchors()
    if (anchors.minimum.x != anchors.maximum.x
            or anchors.minimum.y != anchors.maximum.y):
        return None
    offsets = slot.get_offsets()
    if offsets.right <= 0.0 or offsets.bottom <= 0.0:
        return None
    return (float(offsets.right), float(offsets.bottom))


# 테두리가 자리의 이만큼을 넘으면 9-slice 를 포기한다.
BORDER_ROOM = 0.8


def fill(brush, texture, slice_ratio, target=None):
    """브러시에 그림과 9-slice 여백을 넣는다.

    여백이 있으면 Box 로 그린다 -- 모서리 장식은 원본 픽셀 그대로 두고
    가운데만 늘어난다. 통짜로 늘리면 장식까지 같이 늘어나 뭉개진다.

    **다만 테두리가 자리보다 크면 Box 를 못 쓴다.** 9-slice 테두리는 원본
    픽셀로 그려지므로(ElementBatcher.cpp:857) 자리를 줄여도 안 얇아진다.
    1861x342 짜리 판의 90px 테두리는 64px 짜리 자리에 위아래 180px 을
    넣으라는 말이 된다. 그럴 때는 통짜로 늘린다 -- 브라우저에서 보고 고른
    모습도 통짜로 늘린 것이었으니 오히려 그쪽이 고른 대로다.

    @return 실제로 쓴 방식과 까닭.
    """
    brush.set_editor_property("resource_object", texture)
    why = ""
    use_box = bool(slice_ratio)
    if use_box and target is not None:
        width = float(texture.blueprint_get_size_x())
        height = float(texture.blueprint_get_size_y())
        wide = width * float(slice_ratio[0]) * 2.0
        tall = height * float(slice_ratio[1]) * 2.0
        if wide > target[0] * BORDER_ROOM or tall > target[1] * BORDER_ROOM:
            use_box = False
            why = (f"테두리 {wide:.0f}x{tall:.0f}px 가 자리 "
                   f"{target[0]:.0f}x{target[1]:.0f} 에 안 들어가 통짜로 늘림")
    if use_box:
        x, y = float(slice_ratio[0]), float(slice_ratio[1])
        brush.set_editor_property("draw_as", unreal.SlateBrushDrawType.BOX)
        brush.set_editor_property("margin", unreal.Margin(x, y, x, y))
    else:
        brush.set_editor_property("draw_as", unreal.SlateBrushDrawType.IMAGE)
        brush.set_editor_property("margin", unreal.Margin(0.0, 0.0, 0.0, 0.0))
    brush.set_editor_property("tint_color", unreal.SlateColor(WHITE))
    return ("9-slice" if use_box else "통짜"), why


def clear(brush):
    brush.set_editor_property("resource_object", None)
    brush.set_editor_property("draw_as", unreal.SlateBrushDrawType.NO_DRAW_TYPE)
    brush.set_editor_property("margin", unreal.Margin(0.0, 0.0, 0.0, 0.0))
    return brush


def put_image(widget, texture, slice_ratio):
    brush = widget.get_editor_property("brush")
    if texture is None:
        clear(brush)
        how, why = "안 그림", ""
    else:
        how, why = fill(brush, texture, slice_ratio, slot_size(widget))
    widget.set_editor_property("brush", brush)
    widget.set_color_and_opacity(WHITE)
    return how, why


def put_button(button, texture, slice_ratio):
    """FrameImage 가 없는 Button. 스타일 네 칸을 다 채운다.

    한 칸이라도 비우면 그 상태에서 슬레이트 기본 회색 사각이 그려진다.
    """
    target = slot_size(button)
    style = button.get_editor_property("widget_style")
    how, why = "안 그림", ""
    for slot in ("normal", "hovered", "pressed", "disabled"):
        brush = unreal.SlateBrush()
        if texture is None:
            clear(brush)
        else:
            how, why = fill(brush, texture, slice_ratio, target)
            if slot == "pressed":
                # 눌림 그림이 따로 없다. 같은 그림을 조금 어둡게 해 티만 낸다.
                brush.set_editor_property("tint_color", unreal.SlateColor(
                    unreal.LinearColor(0.80, 0.80, 0.78, 1.0)))
            elif slot == "disabled":
                brush.set_editor_property("tint_color", unreal.SlateColor(
                    unreal.LinearColor(0.45, 0.45, 0.45, 1.0)))
        style.set_editor_property(slot, brush)
    button.set_editor_property("widget_style", style)
    return how, why


def uncover(button):
    """Button 이 밑의 FrameImage 를 덮지 않게 비운다.

    타이틀 단추는 그림 없이 ``ROUNDED_BOX`` 를 알파 1.0 으로 칠하고 있었다.
    Button 이 z22, FrameImage 가 z20 이라 고른 그림이 통째로 가려진다 --
    브라우저에서는 Button 을 테두리로만 그려서 안 보이던 문제다.

    누르는 자리는 그대로 두고 **칠만 없앤다.** 누른 티는 옅은 흰 판으로 낸다.
    """
    style = button.get_editor_property("widget_style")
    for slot in ("normal", "disabled"):
        style.set_editor_property(slot, clear(unreal.SlateBrush()))
    for slot, alpha in (("hovered", 0.12), ("pressed", 0.22)):
        brush = unreal.SlateBrush()
        brush.set_editor_property("resource_object", None)
        brush.set_editor_property("draw_as", unreal.SlateBrushDrawType.BOX)
        brush.set_editor_property("tint_color", unreal.SlateColor(
            unreal.LinearColor(1.0, 0.94, 0.80, alpha)))
        style.set_editor_property(slot, brush)
    button.set_editor_property("widget_style", style)


def texture_of(path):
    leaf = path.rsplit("/", 1)[-1]
    return unreal.load_object(None, f"{path}.{leaf}")


def rgb(text):
    text = str(text).lstrip("#")
    return unreal.LinearColor(*[int(text[i:i + 2], 16) / 255.0
                                for i in (0, 2, 4)], 1.0)


def put_text(widget, look):
    """글자칸에 글꼴·크기·색·정렬을 넣는다.

    **세로 가운데는 TextBlock 혼자 못 한다.** TextBlock 에는 세로 정렬이
    없어서, 칸을 정해진 높이로 깔면 글자가 늘 칸 위쪽에 붙는다. 그래서
    타이틀은 C++ 이 런타임에 Overlay 로 감싸고 있었다(TitleMenuWidget.cpp:293).

    여기서는 판에서 끝낸다: 칸을 **auto size** 로 바꾸고 alignment 를 원하는
    쪽으로 준 뒤, 자리를 원래 칸의 그 지점으로 옮긴다. 칸이 글자를 딱 감싸므로
    alignment 가 곧 정렬이 된다 -- 감싸는 위젯을 새로 만들 필요가 없다.

    @return 무엇을 했는지.
    """
    font = widget.get_editor_property("font")
    told = []

    face_name = look.get("font")
    if face_name in FONTS:
        face = unreal.load_object(None, FONTS[face_name])
        if face is not None:
            font.set_editor_property("font_object", face)
            typeface = look.get("typeface")
            if typeface:
                font.set_editor_property("typeface_font_name", str(typeface))
            told.append(face_name)
    if look.get("size"):
        font.set_editor_property("size", float(look["size"]))
        told.append(f"{float(look['size']):.0f}pt")
    widget.set_editor_property("font", font)

    # 테두리는 **글꼴 안**에 들어 있다. 굵기가 0이면 없는 것이다.
    if "outSize" in look:
        outline = font.get_editor_property("outline_settings")
        outline.set_editor_property("outline_size", int(look["outSize"]))
        if look.get("outC"):
            colour = rgb(look["outC"])
            colour.a = float(look.get("outA", 1.0))
            outline.set_editor_property("outline_color", colour)
            # 옅은 테두리를 쓰려면 이게 켜져 있어야 한다. 안 그러면 글자
            # 알파와 섞여 색이 탁해진다.
            outline.set_editor_property("separate_fill_alpha",
                                        float(look.get("outA", 1.0)) < 0.999)
        if "outDrop" in look:
            # 테두리를 그림자에도 입히면 그림자가 테두리만큼 두꺼워진다 --
            # 슬레이트에는 번짐이 없으니 은은하게 보이려면 이 방법뿐이다.
            outline.set_editor_property("apply_outline_to_drop_shadows",
                                        bool(look["outDrop"]))
        font.set_editor_property("outline_settings", outline)
        widget.set_editor_property("font", font)
        if look["outSize"]:
            told.append(f"테두리 {int(look['outSize'])}px "
                        f"{float(look.get('outA', 1.0)):.0%}"
                        + ("·그림자에도" if look.get("outDrop") else ""))
        else:
            told.append("테두리 없음")

    if look.get("color"):
        widget.set_color_and_opacity(unreal.SlateColor(rgb(look["color"])))
        told.append(look["color"])

    # 그림자는 글자칸 쪽이다. 진하기가 0이면 밀어 놔도 안 보인다.
    if "shX" in look or "shY" in look or "shA" in look:
        widget.set_editor_property("shadow_offset", unreal.Vector2D(
            float(look.get("shX", 0.0)), float(look.get("shY", 0.0))))
        colour = rgb(look.get("shC", "#000000"))
        colour.a = float(look.get("shA", 0.0))
        widget.set_editor_property("shadow_color_and_opacity", colour)
        told.append(f"그림자 ({look.get('shX', 0)},{look.get('shY', 0)}) "
                    f"{look.get('shA', 0):.0%}" if look.get("shA")
                    else "그림자 없음")

    just = str(look.get("just", "")).upper()
    if just in JUSTIFY:
        widget.set_editor_property("justification", JUSTIFY[just])
        told.append(f"좌우 {just}")

    vert = str(look.get("vert", "")).upper()
    slot = widget.get_editor_property("slot")
    if isinstance(slot, unreal.CanvasPanelSlot) and (
            vert in VERTICAL or just in HORIZONTAL):
        offsets = slot.get_offsets()
        align = slot.get_alignment()
        if slot.get_auto_size():
            # 이미 글자를 감싸고 있다. 정렬점만 옮기면 글자가 딴 데로 가므로
            # 자리는 그대로 두고 값만 바꾼다.
            want_x = HORIZONTAL.get(just, align.x)
            want_y = VERTICAL.get(vert, align.y)
            slot.set_alignment(unreal.Vector2D(want_x, want_y))
            told.append(f"세로 {vert}" if vert else "")
        else:
            # 정해진 칸이다. 칸의 왼쪽위가 어디였는지 구해서, 고른 지점으로
            # 자리를 옮기고 칸을 글자에 맡긴다.
            left = offsets.left - align.x * offsets.right
            top = offsets.top - align.y * offsets.bottom
            want_x = HORIZONTAL.get(just, 0.5)
            want_y = VERTICAL.get(vert, 0.5)
            slot.set_auto_size(True)
            slot.set_alignment(unreal.Vector2D(want_x, want_y))
            slot.set_offsets(unreal.Margin(
                left + offsets.right * want_x, top + offsets.bottom * want_y,
                0.0, 0.0))
            told.append(f"세로 {vert or 'CENTER'} (칸 "
                        f"{offsets.right:.0f}x{offsets.bottom:.0f} -> auto)")
    return " · ".join(t for t in told if t)


def main():
    picks = json.loads((MOCKUPS / "tryon.json").read_text(encoding="utf-8"))
    book = catalogue()
    touched = {}

    # 같은 판끼리 모아 둔다. 판마다 한 번만 컴파일·저장하려는 것이다.
    by_asset = {}
    for key, choice in picks.items():
        asset, _sep, widget_name = key.rpartition("/")
        by_asset.setdefault(asset, []).append((widget_name, choice))

    for asset in sorted(by_asset):
        blueprint = unreal.EditorAssetLibrary.load_asset(asset)
        say(f"{asset}")
        if blueprint is None:
            say("    판 없음")
            continue

        for widget_name, choice in sorted(by_asset[asset]):
            widget = widget_of(blueprint, widget_name)
            if widget is None:
                say(f"    {widget_name}: 위젯 없음")
                continue

            # 값이 덩어리면 글자 설정이다. 그림은 이름 하나로 온다.
            if isinstance(choice, dict):
                if not isinstance(widget, unreal.TextBlock):
                    say(f"    {widget_name}: 글자칸이 아닌데 글자 설정이 왔음")
                    continue
                blueprint.modify()
                widget.modify()
                told = put_text(widget, choice)
                touched[asset] = blueprint
                say(f"    {widget_name} <- {told}  [TextBlock]")
                continue

            texture, slice_ratio, label = None, None, "그림 없앰"
            if choice != NONE:
                entry = book.get(choice)
                if entry is None:
                    say(f"    {widget_name}: 그림 {choice} 가 목록에 없음")
                    continue
                path, slice_ratio = entry
                texture = texture_of(path)
                if texture is None:
                    say(f"    {widget_name}: 그림 {path} 를 못 읽음")
                    continue
                label = choice

            blueprint.modify()

            # Button 을 골랐어도 밑에 FrameImage 가 있으면 그쪽에 넣고,
            # Button 은 안 덮게 비운다.
            where, note = widget_name, ""
            if isinstance(widget, unreal.Button):
                frame_name, frame = frame_of(blueprint, widget_name)
                if frame is not None:
                    widget, where = frame, frame_name

            # 그림칸 위에 덮는 Button 이 있으면 칠을 지운다. 사람이 Button 을
            # 골랐든 그림칸을 골랐든 같은 일이 필요하다.
            if isinstance(widget, unreal.Image) and texture is not None:
                above_name, above = button_of(blueprint, where)
                if above is not None and covers(above):
                    above.modify()
                    uncover(above)
                    note = f"  ({above_name} 의 칠은 지움 -- 안 그러면 덮는다)"

            widget.modify()
            if isinstance(widget, unreal.Image):
                how, why = put_image(widget, texture, slice_ratio)
                kind_name = "Image"
            elif isinstance(widget, unreal.Button):
                how, why = put_button(widget, texture, slice_ratio)
                kind_name = "Button 스타일"
            elif isinstance(widget, unreal.Border):
                brush = widget.get_editor_property("background")
                if texture is None:
                    clear(brush)
                    how, why = "안 그림", ""
                else:
                    how, why = fill(brush, texture, slice_ratio, slot_size(widget))
                widget.set_editor_property("background", brush)
                kind_name = "Border"
            else:
                say(f"    {widget_name}: {type(widget).__name__} 은 그림을 못 받음")
                continue

            room = slot_size(widget)
            edge = f"  자리 {room[0]:.0f}x{room[1]:.0f}" if room else ""
            touched[asset] = blueprint
            say(f"    {where} <- {label}  [{kind_name} · {how}]{edge}{note}")
            if why:
                say(f"        {why}")

    for asset, blueprint in touched.items():
        unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
        saved = unreal.EditorAssetLibrary.save_loaded_asset(blueprint, False)
        say(f"{asset.rsplit('/', 1)[-1]} 저장={saved}")

    RESULT.parent.mkdir(parents=True, exist_ok=True)
    RESULT.write_text("\n".join(LINES) + "\n", encoding="utf-8")


main()
