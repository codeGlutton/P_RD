"""WBP_C03_Experience 위젯 트리 덤프: 각 UImage의 텍스처와 캔버스 위치 확인."""
import unreal

bp = unreal.EditorAssetLibrary.load_asset(
    "/Game/UI/RewardSettlement/Concepts/WBP_C03_Experience")
tree = bp.get_editor_property("widget_tree")
widgets = []

def walk(w):
    if w is None:
        return
    widgets.append(w)
    if isinstance(w, unreal.PanelWidget):
        for i in range(w.get_children_count()):
            walk(w.get_child_at(i))
    elif isinstance(w, unreal.ContentWidget):
        walk(w.get_content())

walk(tree.get_editor_property("root_widget"))
unreal.log(f"C03DUMP total={len(widgets)}")
for w in widgets:
    cls = w.get_class().get_name()
    line = f"C03DUMP {w.get_name():34s} {cls}"
    if isinstance(w, unreal.Image):
        brush = w.get_editor_property("brush")
        res = brush.get_editor_property("resource_object")
        line += f" tex={res.get_name() if res else 'NONE'}"
    slot = w.get_editor_property("slot")
    if isinstance(slot, unreal.CanvasPanelSlot):
        d = slot.get_editor_property("layout_data")
        off = d.get_editor_property("offsets")
        z = slot.get_editor_property("z_order")
        line += (f" pos=({off.get_editor_property('left'):.0f},"
                 f"{off.get_editor_property('top'):.0f},"
                 f"{off.get_editor_property('right'):.0f},"
                 f"{off.get_editor_property('bottom'):.0f}) z={z}")
    unreal.log(line)
unreal.SystemLibrary.execute_console_command(None, "QUIT_EDITOR")
