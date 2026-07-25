"""Report what every Image brush in the layout actually points at.

The capture showed frames but no surface, rings, portraits, gems or icons.
Either the brushes lost their resource on save, or they have one and something
else is hiding them -- and those two need completely different fixes.
"""
import unreal

ASSET = "/Game/UI/CombatLayouts/WBP_CombatLayout_01_ClassicCRPG"

blueprint = unreal.EditorAssetLibrary.load_asset(ASSET)

names = []
info = unreal.MCPythonHelper.umg_get_widget_info(blueprint)
import json
for entry in json.loads(info).get("widgets", []):
    names.append((entry.get("name"), entry.get("type")))

unreal.log("[Inspect] {} widgets".format(len(names)))

no_resource, with_resource = [], []
for name, kind in names:
    if kind not in ("Image", "Border"):
        continue
    widget = unreal.MCPythonHelper.umg_find_widget(blueprint, name)
    if widget is None:
        continue
    field = "brush" if kind == "Image" else "background"
    brush = widget.get_editor_property(field)
    resource = brush.get_editor_property("resource_object")
    tint = brush.get_editor_property("tint_color").get_editor_property(
        "specified_color")
    row = "{:34s} {:7s} res={:38s} a={:.2f} draw={} tile={}".format(
        name, kind,
        resource.get_name() if resource else "None",
        tint.a,
        str(brush.get_editor_property("draw_as")).split(".")[-1],
        str(brush.get_editor_property("tiling")).split(".")[-1])
    (with_resource if resource else no_resource).append(row)

unreal.log("[Inspect] ---- select frames ----")
for name in ("PartySelected_0", "CommandSelected_1", "TurnCurrent_0"):
    widget = unreal.MCPythonHelper.umg_find_widget(blueprint, name)
    if widget is None:
        continue
    brush = widget.get_editor_property("brush")
    extent = brush.get_editor_property("image_size")
    margin = brush.get_editor_property("margin")
    unreal.log("[Inspect] {:20s} size=({:.2f},{:.2f}) margin=({:.4f},{:.4f}) draw={}".format(
        name,
        extent.get_editor_property("x"), extent.get_editor_property("y"),
        margin.get_editor_property("left"), margin.get_editor_property("top"),
        str(brush.get_editor_property("draw_as")).split(".")[-1]))
unreal.log("[Inspect] ---- no resource ({}) ----".format(len(no_resource)))
for row in no_resource[:40]:
    unreal.log("[Inspect] " + row)
