"""Check that KitA parts really landed, and that 9-slice margins are sane.

텍스처만 갈아 끼우고 끝내면 늘어난 자리에서 모서리가 뭉개진다. 그림이 들어갔는지
뿐 아니라 **어떻게 그리는지(Image / Box)와 마진**까지 봐야 한다.

마진이 0 인 Box 는 통짜와 같고, 마진이 0.5 를 넘으면 늘릴 가운데가 없다.
둘 다 잘못이라 따로 표시한다.
"""

from pathlib import Path

import unreal

OUT = Path("D:/UnrealProjects/P_RD_develop_20260803/Saved/LegacyAudit/kit_brushes.txt")
ASSETS = [
    "/Game/UI/WBP_SettingsPanel",
    "/Game/UI/CombatDetail/WBP_CombatDetailOverlay",
    "/Game/UI/MonsterTab/WBP_MonsterTab_Marchbound",
    "/Game/UI/CombatLayouts/WBP_CombatHUD04",
]
LINES = []
TOTAL = {"kit": 0, "bad": 0}


# 트리를 훑는 길(get_children_count)이 파이썬에 안 열려 있다. 실제로 훑어 봤더니
# 자식이 0개로 나왔다. 그래서 넣은 자리의 이름을 직접 찍는다.
PROBES = {
    "/Game/UI/WBP_SettingsPanel": (
        ["Set_panel_body", "BackButtonPlate", "ResetButtonPlate",
         "SaveAndExitButtonPlate", "AbandonRunButtonPlate",
         "LowQualityButtonPlate", "FpsThirtyButtonPlate", "LanguageKoreanButtonPlate",
         "Set_confirm_plate", "CancelAbandonButtonPlate"]
        + [f"Set_row_{key}_plate" for key in
           ("master", "bgm", "sfx", "ui", "shake", "vibration", "effects",
            "quality", "fps", "language")]
        + [f"Set_slider_track_{key}" for key in ("master", "bgm", "sfx", "ui")]),
    "/Game/UI/CombatDetail/WBP_CombatDetailOverlay": (
        ["DetailIconFrame", "DetailAimBlockerPlate", "DetailEffectBlockerPlate"]
        + [f"DetailChip{slot}Frame" for slot in range(5)]),
    "/Game/UI/MonsterTab/WBP_MonsterTab_Marchbound": (
        ["MonsterBackArt"] + [f"MonsterSkillSlot_{index}" for index in range(4)]),
    "/Game/UI/CombatLayouts/WBP_CombatHUD04": (
        ["MercenaryBackArt"] + [f"MercenarySkillFrame_{index}" for index in range(6)]),
}


def check(tree, name):
    widget = unreal.find_object(None, tree.get_path_name() + "." + name)
    if widget is None:
        return f"  {name:34s} 위젯 없음"
    if not isinstance(widget, unreal.Image):
        return f"  {name:34s} Image 가 아님({type(widget).__name__})"
    brush = widget.get_editor_property("brush")
    source = brush.get_editor_property("resource_object")
    texture = source.get_name() if source is not None else "(그림 없음)"
    if not texture.startswith("T_KitA_"):
        return f"  {name:34s} {texture:32s} <-- KitA 아님"
    TOTAL["kit"] += 1
    draw = brush.get_editor_property("draw_as")
    margin = brush.get_editor_property("margin")
    is_box = draw == unreal.SlateBrushDrawType.BOX
    note = ""
    if is_box:
        values = (margin.left, margin.top, margin.right, margin.bottom)
        if min(values) <= 0.0:
            note = "  <-- Box 인데 마진 0. 통짜와 같다"
        elif max(values) >= 0.5:
            note = "  <-- 마진이 커서 늘릴 가운데가 없다"
    if note:
        TOTAL["bad"] += 1
    return (f"  {name:34s} {texture:32s} {'Box' if is_box else 'Image':5s} "
            f"({margin.left:.3f},{margin.top:.3f},{margin.right:.3f},{margin.bottom:.3f}){note}")


for path, names in PROBES.items():
    blueprint = unreal.EditorAssetLibrary.load_asset(path)
    LINES.append(f"=== {path.rsplit('/', 1)[-1]} ===")
    if blueprint is None:
        LINES.append("  자산을 못 읽음")
        continue
    tree = unreal.find_object(None, blueprint.get_path_name() + ":WidgetTree")
    for name in names:
        LINES.append(check(tree, name))
    LINES.append("")

LINES.append(f"KitA 를 쓰는 그림 {TOTAL['kit']}개 / 그리는 방식이 잘못된 것 {TOTAL['bad']}개")
OUT.write_text("\n".join(LINES), encoding="utf-8")
