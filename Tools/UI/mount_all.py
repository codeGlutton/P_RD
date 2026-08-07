"""화면 전체에서 **틀 안에 놓인 글자·버튼을 그 틀에 묶는다.**

왜
--
지금은 틀과 글자가 캔버스에 따로 절대 좌표로 놓여 있어 서로 아무 끈이 없다.
틀을 옮기거나 그림을 갈아 끼우면 글자는 제자리에 남는다. 타이틀에서 그것
때문에 매번 도구를 다시 돌려야 했다.

짝은 이름이 아니라 **자리**로 찾는다
------------------------------------
`XxxText` <-> `XxxFrameImage` 같은 이름 규칙은 화면마다 다르다. 대신 글자가
어느 그림 안에 들어 있는지를 본다. 후보가 여럿이면 **가장 작은 것**을 고른다 --
화면을 덮는 배경이 아니라 바로 뒤의 판이 그 글자의 틀이다.

지금 모습은 안 바꾼다
---------------------
묶으면서 글자를 틀 한가운데로 옮기면 화면이 우르르 달라진다. 그래서 여백을
**지금 자리 그대로** 계산해 넣는다. 구조만 바뀌고 보이는 것은 같다.

Run headless:
    UnrealEditor-Cmd.exe <project> -run=pythonscript
        -script="Tools/UI/mount_all.py"
"""

import json
from pathlib import Path

import unreal

ROOT = Path("D:/UnrealProjects/P_RD_develop_20260803")
WORKSPACE = Path("D:/UnrealProjects_WBP_Editor/data/workspace.json")
RESULT = ROOT / "Saved/LegacyAudit/mount_all.txt"

# 틀이 이보다 크면 배경으로 본다. 배경에 글자를 묶으면 화면 전체가 한 덩어리가 된다.
BIGGEST_FRAME = 0.55          # 캔버스 넓이 대비
SMALLEST_FRAME = 24.0         # 이보다 작은 그림은 아이콘이지 틀이 아니다
FRAME_CLASSES = ("Image", "Border")
RIDER_CLASSES = ("TextBlock", "RichTextBlock", "Button")

LINES = []


def say(text):
    LINES.append(text)
    unreal.log(text)


def rect_of(row):
    r = row.get("rect") or {}
    return (float(r.get("x", 0)), float(r.get("y", 0)),
            float(r.get("w", 0)), float(r.get("h", 0)))


def inside(small, big, slack=2.0):
    sx, sy, sw, sh = small
    bx, by, bw, bh = big
    return (sx >= bx - slack and sy >= by - slack
            and sx + sw <= bx + bw + slack and sy + sh <= by + bh + slack)


def plan(document):
    """이 캔버스에서 (틀, 그 안에 놓인 것들) 짝을 뽑는다."""
    size = document.get("designSize") or [1920, 1080]
    canvas_area = float(size[0]) * float(size[1])
    rows = [w for w in document.get("widgets", []) if not w.get("nested")]

    frames = []
    for row in rows:
        if row.get("className") not in FRAME_CLASSES:
            continue
        box = rect_of(row)
        if box[2] < SMALLEST_FRAME or box[3] < SMALLEST_FRAME:
            continue
        if box[2] * box[3] > canvas_area * BIGGEST_FRAME:
            continue
        frames.append((row, box))

    pairs = {}
    for row in rows:
        name = row.get("name", "")
        if row.get("className") not in RIDER_CLASSES and not name.endswith("_Center"):
            continue
        if name.endswith("Mount") or "Mount__" in name:
            continue
        box = rect_of(row)
        if box[2] <= 0 or box[3] <= 0:
            continue
        # 가장 작은 -- 즉 가장 가까운 -- 틀을 고른다.
        best = None
        for frame_row, frame_box in frames:
            if frame_row is row or not inside(box, frame_box):
                continue
            if best is None or frame_box[2] * frame_box[3] < best[1][2] * best[1][3]:
                best = (frame_row, frame_box)
        if best is not None:
            pairs.setdefault(best[0]["name"], (best[1], []))[1].append((name, box))
    return pairs


def slot_box(widget):
    slot = widget.get_editor_property("slot") if widget else None
    if not isinstance(slot, unreal.CanvasPanelSlot):
        return None
    return (slot.get_anchors(), slot.get_offsets(), slot.get_alignment(),
            slot.get_z_order())


def put(overlay, widget, padding=None, centre=False):
    parent = widget.get_parent()
    if parent is not None and parent != overlay:
        parent.remove_child(widget)
    if widget.get_parent() != overlay:
        overlay.add_child(widget)
    slot = widget.get_editor_property("slot")
    if isinstance(slot, unreal.OverlaySlot):
        slot.set_horizontal_alignment(unreal.HorizontalAlignment.H_ALIGN_FILL)
        slot.set_vertical_alignment(
            unreal.VerticalAlignment.V_ALIGN_CENTER if centre
            else unreal.VerticalAlignment.V_ALIGN_FILL)
        slot.set_padding(padding if padding is not None
                         else unreal.Margin(0.0, 0.0, 0.0, 0.0))


def main():
    data = json.loads(WORKSPACE.read_text(encoding="utf-8"))
    touched, mounted, riders, skipped = {}, 0, 0, 0

    for document in data.get("documents", []):
        if document.get("sourceKind") != "current-develop-wbp":
            continue
        asset = document.get("assetPath", "?")
        pairs = plan(document)
        if not pairs:
            continue
        blueprint = unreal.EditorAssetLibrary.load_asset(asset)
        if blueprint is None:
            continue
        tree = unreal.find_object(None, blueprint.get_path_name() + ":WidgetTree")
        if tree is None:
            continue

        head = f"{asset} / {document.get('canvasName')}"
        rows = []
        for frame_name, (frame_box, kids) in sorted(pairs.items()):
            frame = unreal.find_object(None, f"{tree.get_path_name()}.{frame_name}")
            box = slot_box(frame)
            if frame is None or box is None:
                skipped += 1
                continue
            anchors, offsets, align, z = box
            canvas = frame.get_parent()

            mount_name = f"{frame_name}Mount"
            mount = unreal.find_object(None, f"{tree.get_path_name()}.{mount_name}")
            if mount is None:
                mount = unreal.new_object(unreal.Overlay, outer=tree, name=mount_name)
            blueprint.modify()
            mount.modify()
            if mount.get_parent() is None:
                canvas.add_child(mount)
            mount_slot = mount.get_editor_property("slot")
            if isinstance(mount_slot, unreal.CanvasPanelSlot):
                mount_slot.set_anchors(anchors)
                mount_slot.set_offsets(offsets)
                mount_slot.set_alignment(align)
                mount_slot.set_auto_size(False)
                mount_slot.set_z_order(z)
            mount.set_visibility(unreal.SlateVisibility.SELF_HIT_TEST_INVISIBLE)

            frame.modify()
            put(mount, frame)
            rows.append(f"  {mount_name}")

            for kid_name, kid_box in sorted(kids, key=lambda k: k[1][1]):
                kid = unreal.find_object(None, f"{tree.get_path_name()}.{kid_name}")
                if kid is None:
                    continue
                kid.modify()
                # **지금 자리를 그대로** 재현하는 여백. 구조만 바뀐다.
                pad = unreal.Margin(
                    kid_box[0] - frame_box[0], kid_box[1] - frame_box[1],
                    (frame_box[0] + frame_box[2]) - (kid_box[0] + kid_box[2]),
                    (frame_box[1] + frame_box[3]) - (kid_box[1] + kid_box[3]))
                put(mount, kid, pad)
                riders += 1
                rows.append(f"      {kid_name:44} 여백 "
                            f"{pad.left:.0f}/{pad.top:.0f}/{pad.right:.0f}/{pad.bottom:.0f}")
            mounted += 1
            touched[asset] = blueprint

        if rows:
            say(head)
            for row in rows:
                say(row)

    for asset, blueprint in touched.items():
        unreal.SystemLibrary.execute_console_command(
            None, f"RD.Editor.CleanWidgetVariables {asset}")
        unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
        saved = unreal.EditorAssetLibrary.save_loaded_asset(blueprint, False)
        say(f"{asset.rsplit('/', 1)[-1]} 저장={saved}")

    say(f"\n묶음 {mounted}개 · 안에 넣은 것 {riders}개 · 건너뜀 {skipped}개")
    RESULT.parent.mkdir(parents=True, exist_ok=True)
    RESULT.write_text("\n".join(LINES) + "\n", encoding="utf-8")


main()
