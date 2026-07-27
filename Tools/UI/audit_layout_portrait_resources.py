"""Report the baked portrait brush resources in combat layout 01."""

import json
import os

import unreal


ASSET = "/Game/UI/CombatLayouts/WBP_CombatLayout_01_ClassicCRPG"
OUTPUT = os.path.join(
    unreal.Paths.project_saved_dir(),
    "UI",
    "layout_portrait_resources.json",
)
NAMES = (
    "EnemyPortrait",
    "PartyPortrait_0",
    "PartyPortrait_1",
    "PartyPortrait_2",
    "TurnPortrait_0",
    "TurnPortrait_1",
    "TurnPortrait_2",
    "TurnPortrait_3",
    "TurnPortrait_4",
)

blueprint = unreal.EditorAssetLibrary.load_asset(ASSET)
if blueprint is None:
    raise RuntimeError("layout missing: " + ASSET)

rows = []
for name in NAMES:
    widget = unreal.MCPythonHelper.umg_find_widget(blueprint, name)
    resource = None
    if widget is not None:
        brush = widget.get_editor_property("brush")
        value = brush.get_editor_property("resource_object")
        resource = value.get_path_name() if value else None
    rows.append({"widget": name, "resource": resource})

os.makedirs(os.path.dirname(OUTPUT), exist_ok=True)
with open(OUTPUT, "w", encoding="utf-8") as handle:
    json.dump(rows, handle, ensure_ascii=False, indent=2)
unreal.log("[LayoutPortraitAudit] wrote {}".format(OUTPUT))
