"""비어 있던 위젯에 **이미 있는 에셋**을 물린다.

왜 새로 안 만드나
-----------------
"빠진 그림" 이라고 세어 둔 것이 열여섯이었는데, 하나씩 따져 보니

    여덟   단색 반투명 판이다. 그림이 아니라 색 하나면 된다(Border).
    여섯   SVN 에 이미 쓸 만한 것이 있다. 351장 중에서 안 골라 썼을 뿐이다.
    둘     조립하면 된다(판 + 글자).

그래서 만들 것이 사실상 없다. AI 로 백 장 넘게 뽑고 여든넷을 버린 뒤에야
세어 본 것이 이 표다 -- 세는 것이 먼저였다.

무엇을 어디에
-------------
9-slice 로 늘려 쓴다. 테두리 두께는 그림이 정하므로(원본 픽셀 그대로) 큰 자리에
놓을수록 얇아 보인다 -- 1024px 짜리 판의 37px 테두리는 1712px 자리에서 2%다.

Run headless:
    UnrealEditor-Cmd.exe <project> -run=pythonscript
        -script="Tools/UI/fill_empty_widgets.py"
"""

import sys
from pathlib import Path

import unreal

sys.path.insert(0, str(Path(__file__).resolve().parent))

HIRE = "/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Hire"

# (에셋, 위젯, 쓸 그림, 9-slice 여백(좌,상,우,하))
#
# 여백은 measure_slice_margins.py 가 잰 값이다. 이 그림들은 원본에서 바로 떠서
# 투명 여백이 없으므로 그대로 쓴다(KitA 부품은 8px 을 더해야 한다).
WIRING = [
    ("/Game/UI/WBP_FrontendMap", "MapPaperPanel",
     f"{HIRE}/T_MB_HireRowNormal", (55, 25, 55, 25)),
    ("/Game/UI/WBP_FrontendMap", "Map_LegendImage",
     f"{HIRE}/T_MB_HireListFrame", (58, 69, 58, 69)),
    ("/Game/UI/CombatResult/WBP_CombatDefeat", "DefeatSummaryPanel",
     f"{HIRE}/T_MB_HireRowNormal", (55, 25, 55, 25)),
    ("/Game/UI/CombatLayouts/WBP_CombatHUD04", "ObjectivePlate",
     f"{HIRE}/T_MB_HireNamePlate", (43, 74, 43, 74)),
    ("/Game/UI/CombatLayouts/WBP_CombatHUD04", "ArtifactStripPlate",
     f"{HIRE}/T_MB_HireStatsStrip", (34, 43, 34, 43)),
    ("/Game/UI/CombatLayouts/WBP_CombatHUD04", "TurnPlate",
     f"{HIRE}/T_MB_HireStatsStrip", (34, 43, 34, 43)),
]

# 색만 있으면 되는 것들. 그림을 찾을 이유가 없다.
#
# 이미 Border 인 것이 대부분이라 색만 확인해 준다. 값이 0 이면 아무것도
# 안 보이므로, 화면을 덮는 것은 확실히 어둡게 둔다.
TINTS = [
    ("/Game/UI/CombatDetail/WBP_CombatDetailOverlay", "DetailScrimBg", (0.015, 0.012, 0.02, 0.62)),
    ("/Game/UI/CombatLayouts/WBP_CombatHUD04", "MercenaryScrim", (0.02, 0.014, 0.01, 0.82)),
    ("/Game/UI/CombatLayouts/WBP_MercenaryPanel", "MercenaryScrim", (0.02, 0.014, 0.01, 0.82)),
    ("/Game/UI/WBP_FrontendMap", "MapDimBackground", (0.02, 0.015, 0.012, 0.78)),
    ("/Game/UI/WBP_FrontendMap", "MapPaperShadow", (0.0, 0.0, 0.0, 0.35)),
    ("/Game/UI/WBP_FrontendMap", "Map_NodeArea", (0.0, 0.0, 0.0, 0.0)),
    ("/Game/UI/WBP_SettingsPanel", "Set_scrimBg", (0.055, 0.038, 0.026, 0.88)),
    ("/Game/UI/WBP_SettingsPanel", "Set_confirm_dim", (0.0, 0.0, 0.0, 0.72)),
]

RESULT = Path("D:/UnrealProjects/P_RD_develop_20260803/Saved/LegacyAudit/fill_empty.txt")
LINES = []


def say(text):
    LINES.append(text)
    unreal.log(text)


def widget_of(blueprint, name):
    tree = unreal.find_object(None, blueprint.get_path_name() + ":WidgetTree")
    return unreal.find_object(None, f"{tree.get_path_name()}.{name}")


def main():
    touched = {}

    for asset, name, texture_path, margin in WIRING:
        blueprint = unreal.EditorAssetLibrary.load_asset(asset)
        if blueprint is None:
            say(f"{asset}: 판 없음")
            continue
        widget = widget_of(blueprint, name)
        leaf = texture_path.rsplit("/", 1)[-1]
        texture = unreal.load_object(None, f"{texture_path}.{leaf}")
        if widget is None or texture is None:
            say(f"    {name}: {'위젯' if widget is None else '그림'} 없음")
            continue
        if not isinstance(widget, unreal.Image):
            # Border 는 색판이라 9-slice 를 못 받는다. 이름을 바꾸면 배선이
            # 끊기므로(C++ 이 이름으로 찾는다) **뒤에 Image 를 깔고** Border 는
            # 투명하게 둔다. 그림은 새 Image 가, 자리는 Border 가 잡는다.
            widget.modify()
            widget.set_brush_color(unreal.LinearColor(0.0, 0.0, 0.0, 0.0))
            tree = unreal.find_object(None, blueprint.get_path_name() + ":WidgetTree")
            backing = unreal.find_object(None, f"{tree.get_path_name()}.{name}Art")
            if backing is None:
                # 변수 등록은 파이썬에 안 열려 있다(on_variable_added 없음).
                # 아래에서 RD.Editor.CleanWidgetVariables 로 한꺼번에 맞춘다.
                backing = unreal.new_object(unreal.Image, outer=tree, name=f"{name}Art")
            parent = widget.get_parent()
            if parent is None:
                say(f"    {name}: 부모가 없음 -- 건너뜀")
                continue
            if backing.get_parent() is None:
                slot = parent.add_child(backing)
            backing.set_visibility(unreal.SlateVisibility.SELF_HIT_TEST_INVISIBLE)
            # Border 와 똑같은 자리에 겹친다.
            source_slot = widget.get_editor_property("slot")
            target_slot = backing.get_editor_property("slot")
            if (isinstance(source_slot, unreal.CanvasPanelSlot)
                    and isinstance(target_slot, unreal.CanvasPanelSlot)):
                target_slot.set_anchors(source_slot.get_anchors())
                target_slot.set_offsets(source_slot.get_offsets())
                target_slot.set_alignment(source_slot.get_alignment())
                target_slot.set_z_order(source_slot.get_z_order() - 1)
            widget = backing

        blueprint.modify()
        widget.modify()
        width = float(texture.blueprint_get_size_x())
        height = float(texture.blueprint_get_size_y())
        brush = widget.get_editor_property("brush")
        brush.set_editor_property("resource_object", texture)
        brush.set_editor_property("draw_as", unreal.SlateBrushDrawType.BOX)
        brush.set_editor_property("margin", unreal.Margin(
            margin[0] / width, margin[1] / height,
            margin[2] / width, margin[3] / height))
        brush.set_editor_property("tint_color", unreal.SlateColor(
            unreal.LinearColor(1.0, 1.0, 1.0, 1.0)))
        widget.set_editor_property("brush", brush)
        widget.set_color_and_opacity(unreal.LinearColor(1.0, 1.0, 1.0, 1.0))
        touched[asset] = blueprint
        say(f"    {name} <- {leaf}  (테두리 {margin[0]}px)")

    for asset, name, colour in TINTS:
        blueprint = unreal.EditorAssetLibrary.load_asset(asset)
        if blueprint is None:
            continue
        widget = widget_of(blueprint, name)
        if widget is None:
            say(f"    {name}: 위젯 없음")
            continue
        blueprint.modify()
        widget.modify()
        value = unreal.LinearColor(*colour)
        if isinstance(widget, unreal.Border):
            widget.set_brush_color(value)
        else:
            widget.set_color_and_opacity(value)
        touched[asset] = blueprint
        say(f"    {name} 색 {colour}")

    world = unreal.EditorLevelLibrary.get_editor_world() if hasattr(
        unreal, "EditorLevelLibrary") else None
    for asset, blueprint in touched.items():
        # 새로 만든 위젯은 변수 GUID 가 없다. 컴파일 전에 맞춰 둬야 ensure 가 안 뜬다.
        unreal.SystemLibrary.execute_console_command(
            world, f"RD.Editor.CleanWidgetVariables {asset}")
        unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
        saved = unreal.EditorAssetLibrary.save_loaded_asset(blueprint, False)
        say(f"{asset.rsplit('/', 1)[-1]} saved={saved}")

    RESULT.parent.mkdir(parents=True, exist_ok=True)
    RESULT.write_text("\n".join(LINES) + "\n", encoding="utf-8")


main()
