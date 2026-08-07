"""Add the six widget families the combat HUD looks up but nobody ever authored.

무엇을 고치나
-------------
``UCombatLayoutHUDWidget`` 이 이름으로 찾는데 ``WBP_CombatHUD04`` 에도, 그것을
굽는 ``CombatHUDWidgetBuilder`` 에도 없던 위젯들이다. Find/GetWidgetFromName 이
못 찾으면 조용히 nullptr 을 주고 SetTextIfPresent 가 그냥 넘어가므로, **에러 한 줄
없이 기능만 빠져 있었다.**

    ObjectiveText           "모든 적 처치 — 남은 적 N"
    PartyAPPip_%d_%d        파티 카드의 남은 AP 칸
    PartyAPPipUsed_%d_%d    파티 카드의 쓴 AP 칸
    ArtifactButton_%d       아티팩트 칸을 꾹 눌러 상세를 열 자리

카드의 AP·쿨타임과 턴 토큰 이름은 **일부러 안 만든다.** 카드 오른쪽 배지에 이미
적혀 있고 턴바에는 이름을 안 쓰기로 했다(0804 검수). 코드의 Find 는 그대로 둬도
못 찾으면 조용히 건너뛴다.

배치 원칙
---------
**기존 위젯은 하나도 건드리지 않는다.** 형제 옆 빈자리에만 새로 놓는다. 파티 AP
칸은 이미 잘 도는 ``TurnAPPip_%d`` 를 그대로 본떠 같은 텍스처·같은 겹침 방식을 쓴다
(남은 칸과 쓴 칸을 같은 자리에 겹쳐 두고 하나만 켠다).

Run headless:
    UnrealEditor-Cmd.exe <project> -run=pythonscript
        -script="Tools/UI/add_missing_hud_widgets.py"
"""

from pathlib import Path

import unreal

RESULT = Path("D:/UnrealProjects/P_RD_develop_20260803/Saved/LegacyAudit/hud_widgets_add.txt")
ASSET = "/Game/UI/CombatLayouts/WBP_CombatHUD04"
LINES = []

PARTY_SLOTS = 3
PARTY_AP_PIPS = 10          # 턴 AP 칸과 같은 수

PIP_TEX = "/Game/SVN/OutSideAsset/AICreation/UI/HUD04/KK_HUD04_ap_pip"
PIP_SPENT_TEX = "/Game/SVN/OutSideAsset/AICreation/UI/HUD04/KK_HUD04_ap_pip_spent"

CREAM = unreal.LinearColor(1.0, 0.94, 0.82, 1.0)
GOLD = unreal.LinearColor(0.95, 0.78, 0.42, 1.0)
OUTLINE = unreal.LinearColor(0.02, 0.01, 0.0, 0.95)


def find(tree, name):
    return unreal.find_object(None, tree.get_path_name() + "." + name)


def rect_of(widget):
    slot = widget.get_editor_property("slot")
    if not isinstance(slot, unreal.CanvasPanelSlot):
        return None
    offsets = slot.get_editor_property("layout_data").get_editor_property("offsets")
    return (offsets.left, offsets.top, offsets.right, offsets.bottom)


def place(canvas, child, pos, box, z):
    slot = canvas.add_child_to_canvas(child)
    slot.set_anchors(unreal.Anchors(unreal.Vector2D(0.0, 0.0), unreal.Vector2D(0.0, 0.0)))
    slot.set_alignment(unreal.Vector2D(0.0, 0.0))
    slot.set_editor_property("auto_size", False)
    slot.set_position(unreal.Vector2D(pos[0], pos[1]))
    slot.set_size(unreal.Vector2D(box[0], box[1]))
    slot.set_z_order(z)


def add_text(tree, canvas, name, size, pos, box, z, color, align="center"):
    if find(tree, name) is not None:
        return False
    text = unreal.new_object(unreal.TextBlock, outer=tree, name=name)
    text.set_text(unreal.Text(""))
    font = text.get_editor_property("font")
    font.set_editor_property("size", size)
    outline = font.get_editor_property("outline_settings")
    outline.set_editor_property("outline_size", 1)
    outline.set_editor_property("outline_color", OUTLINE)
    font.set_editor_property("outline_settings", outline)
    text.set_editor_property("font", font)
    text.set_color_and_opacity(unreal.SlateColor(color))
    text.set_editor_property("justification", {
        "left": unreal.TextJustify.LEFT}.get(align, unreal.TextJustify.CENTER))
    text.set_visibility(unreal.SlateVisibility.SELF_HIT_TEST_INVISIBLE)
    place(canvas, text, pos, box, z)
    return True


def add_image(tree, canvas, name, texture_path, pos, box, z):
    if find(tree, name) is not None:
        return False
    image = unreal.new_object(unreal.Image, outer=tree, name=name)
    if texture_path:
        leaf = texture_path.rsplit("/", 1)[-1]
        source = unreal.load_object(None, f"{texture_path}.{leaf}")
        if source is not None:
            image.set_brush_from_texture(source, False)
    image.set_visibility(unreal.SlateVisibility.SELF_HIT_TEST_INVISIBLE)
    place(canvas, image, pos, box, z)
    return True


def build():
    blueprint = unreal.EditorAssetLibrary.load_asset(ASSET)
    if blueprint is None:
        raise RuntimeError(f"missing {ASSET}")
    blueprint.modify()
    tree = unreal.find_object(None, blueprint.get_path_name() + ":WidgetTree")
    tree.modify()
    added = 0

    # 3) 목표 문구 -- 라운드 글자 아래.
    round_text = find(tree, "RoundText")
    if round_text is not None:
        panel = round_text.get_parent()
        panel_rect = rect_of(round_text)
        if isinstance(panel, unreal.CanvasPanel) and panel_rect is not None:
            panel.modify()
            added += add_text(tree, panel, "ObjectiveText", 20,
                              (panel_rect[0], panel_rect[1] + panel_rect[3] - 6.0),
                              (panel_rect[2], 34.0), 12, GOLD, "left")

    # 4) 파티 카드의 AP 칸 -- 이미 도는 TurnAPPip 을 그대로 본뜬다.
    #    남은 칸과 쓴 칸을 같은 자리에 겹쳐 두고 코드가 하나만 켠다.
    for index in range(PARTY_SLOTS):
        content = find(tree, f"PartyContent_{index}")
        plate = find(tree, f"PartyAPPlate_{index}")
        if not isinstance(content, unreal.CanvasPanel) or plate is None:
            continue
        plate_rect = rect_of(plate)
        if plate_rect is None:
            continue
        content.modify()
        inset = 4.0
        step = (plate_rect[2] - inset * 2) / PARTY_AP_PIPS
        pip_width = max(6.0, step - 2.0)
        pip_height = max(10.0, plate_rect[3] - 12.0)
        top = plate_rect[1] + (plate_rect[3] - pip_height) / 2.0
        for pip in range(PARTY_AP_PIPS):
            left = plate_rect[0] + inset + step * pip
            added += add_image(tree, content, f"PartyAPPip_{index}_{pip}", PIP_TEX,
                               (left, top), (pip_width, pip_height), 16)
            added += add_image(tree, content, f"PartyAPPipUsed_{index}_{pip}", PIP_SPENT_TEX,
                               (left, top), (pip_width, pip_height), 15)

    # 5) 아티팩트 칸을 누를 수 있게 한다. 지금까지 Image 뿐이라 눌러도 아무 일이 없었다.
    for index in range(6):
        frame = find(tree, f"ArtifactFrame_{index}")
        if frame is None:
            continue
        parent = frame.get_parent()
        rect = rect_of(frame)
        if not isinstance(parent, unreal.CanvasPanel) or rect is None:
            continue
        name = f"ArtifactButton_{index}"
        if find(tree, name) is not None:
            continue
        parent.modify()
        button = unreal.new_object(unreal.Button, outer=tree, name=name)
        style = button.get_editor_property("widget_style")
        empty = unreal.SlateBrush()
        empty.set_editor_property("draw_as", unreal.SlateBrushDrawType.NO_DRAW_TYPE)
        for state in ("normal", "hovered", "pressed", "disabled"):
            style.set_editor_property(state, empty)
        button.set_editor_property("widget_style", style)
        place(parent, button, (rect[0], rect[1]), (rect[2], rect[3]), 30)
        added += 1

    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    saved = unreal.EditorAssetLibrary.save_loaded_asset(blueprint, False)
    LINES.append(f"added={added} saved={saved}")
    if not saved:
        LINES.append("SAVE BLOCKED -- 에디터/게임에서 이 WBP 를 닫고 다시 실행")


try:
    build()
except Exception as error:  # noqa: BLE001
    import traceback
    LINES.append("FAILED: %s" % error)
    LINES.append(traceback.format_exc())
finally:
    RESULT.write_text("\n".join(LINES), encoding="utf-8")
