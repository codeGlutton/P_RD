# WBP_FrontendMap/Node/Line 월드맵 시안 싱크: concept_worldmap_claude02.json(216)을 정본으로.
# hybridSlotSync — 프레임(스크림/양피지/두루마리/패널/버튼/범례/마커)은 WBP 슬롯+브러시로 소유 이전,
# 노드/선 스타일(링/경로 텍스처, 두께)은 WBP 클래스 디폴트(CDO)로 이관한다.
# 규칙: 슬롯/브러시/클래스 디폴트만 쓴다. 이벤트/바인딩/그래프 불가침. 재실행 안전(idempotent).
# 실행: UnrealEditor-Cmd <uproject> -ExecutePythonScript=<이 파일> (headless)
# 최종 출력: "WORLDMAPSYNC|" + json 리포트 한 줄 (saved 플래그/생성/갱신/임포트 수)
import unreal, json, os, shutil

CONCEPT = "D:/UnrealProjects/P_RD_develop_20260701_216/Start_CombatUIRectEditor/concept_worldmap_claude02.json"
TARGET_PROJECT = "D:/UnrealProjects/P_RD_develop_20260702"
MAP_PATH = "/Game/BP/UI/WBP_FrontendMap"
NODE_PATH = "/Game/BP/UI/WBP_FrontendMapNode"
LINE_PATH = "/Game/BP/UI/WBP_FrontendMapLine"
CONTENT_UI = TARGET_PROJECT + "/Content/BP/UI/"
ASSETS = "D:/UnrealProjects/P_RD_CombatUI_Assets_nobg/"
TEX_DEST = "/Game/SVN/OutSideAsset/AICreation/UI/WorldMap"
MAPNODE_DIR = "/Game/SVN/OutSideAsset/AICreation/UI/MapNode/"

doc = json.load(open(CONCEPT, encoding="utf-8"))
DW, DH = float(doc["designSize"][0]), float(doc["designSize"][1])
els = {e["name"]: e for e in doc["screens"][0]["elements"]}
NM = doc.get("wbpNativeSpec", {}).get("nodeMetrics", {})
NODE_SIZE = float(NM.get("nodeSize", 112))
BOSS_NODE_SIZE = float(NM.get("bossNodeSize", 128))
ROW_PITCH = float(NM.get("rowPitch", 176))
COL_PITCH_MAX = float(NM.get("colPitchMax", 240))

res = {"project": "", "saved": {"WBP_FrontendMap": None, "WBP_FrontendMapNode": None, "WBP_FrontendMapLine": None},
       "created": [], "synced": [], "imported": [], "collapsed": [], "reparented": [],
       "missing": [], "errors": [], "notes": [], "trees": {}, "counts": {}}


def err(scope, msg):
    res["errors"].append(scope + ": " + str(msg)[:120])


# ---------------- concept 좌표 유틸 (레퍼런스 빌더와 동일 계열) ----------------
def elem_abs(el_name):
    return list(map(float, els[el_name]["screenRect"]))


def region_abs(el_name, rg_name_prefix=None, rg_type=None, index=0):
    """요소 내부 리전의 화면 절대 rect. prefix/type로 매칭, index로 n번째 선택."""
    e = els[el_name]
    ex, ey, ew, eh = map(float, e["screenRect"])
    ids = e.get("innerDesignSize") or [ew, eh]
    sx, sy = ew / float(ids[0]), eh / float(ids[1])
    hit = 0
    for r in e.get("regions", []):
        if rg_name_prefix is not None and not r.get("name", "").startswith(rg_name_prefix):
            continue
        if rg_type is not None and r.get("type") != rg_type:
            continue
        if hit == index:
            rx, ry, rw, rh = map(float, r["rect"])
            return [ex + rx * sx, ey + ry * sy, rw * sx, rh * sy]
        hit += 1
    return None


def region_image(el_name, index=0, rg_type=None):
    for r in els[el_name].get("regions", []):
        if rg_type is not None and r.get("type") != rg_type:
            continue
        if index == 0:
            return r.get("imageAsset") or None
        index -= 1
    return None


PIN = {"left": 0.0, "center": 0.5, "right": 1.0, "top": 0.0, "bottom": 1.0}


def pin_of(el_name):
    """wbp.pin("left/center" 등) 우선, 없으면 anchor h/v — 포인트 앵커 분수 (ax, ay)."""
    e = els[el_name]
    p = e.get("wbp", {}).get("pin")
    if p:
        h, v = p.split("/")
        return (PIN[h], PIN[v])
    a = e.get("anchor", {})
    return (PIN.get(a.get("h"), 0.5), PIN.get(a.get("v"), 0.5))


# ---------------- 텍스처 임포트 (T_wm_<원본이름>, idempotent: 있으면 재사용) ----------------
def tex_asset_name(src):
    return "T_wm_" + os.path.splitext(os.path.basename(src))[0]


def ensure_import(src):
    if not src:
        return None
    name = tex_asset_name(src)
    obj = TEX_DEST + "/" + name + "." + name
    if unreal.EditorAssetLibrary.does_asset_exist(obj):
        return unreal.load_asset(obj)
    if not os.path.exists(src):
        res["missing"].append("src-png:" + src)
        return None
    try:
        task = unreal.AssetImportTask()
        task.filename = src
        task.destination_path = TEX_DEST
        task.destination_name = name
        task.replace_existing = True
        task.automated = True
        task.save = True
        unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    except Exception as ex:
        err("import", name + " " + str(ex))
        return None
    if unreal.EditorAssetLibrary.does_asset_exist(obj):
        res["imported"].append(name)
        return unreal.load_asset(obj)
    err("import", name + " not created")
    return None


def load_mapnode(base):
    """기존 프로젝트 노드 아이콘(T_MapNode_*) 재사용 — 재임포트 금지."""
    p = MAPNODE_DIR + base + "." + base
    if unreal.EditorAssetLibrary.does_asset_exist(p):
        return unreal.load_asset(p)
    res["missing"].append("mapnode:" + base)
    return None


def icon_tex_for(image_asset):
    """리전 imageAsset 해석: T_MapNode_*는 기존 에셋 로드, 그 외(커스텀 휴식 등)는 임포트."""
    if not image_asset:
        return None
    base = os.path.splitext(os.path.basename(image_asset))[0]
    if base.startswith("T_MapNode_"):
        return load_mapnode(base)
    return ensure_import(image_asset)


# ---------------- 브러시/슬롯 유틸 ----------------
def apply_brush(img, tex, w_px, h_px, draw="image", margin=None):
    """Image 위젯 기존 브러시에 텍스처만 얹는다. image_size는 DeprecateSlateVector2D 폴백 체인."""
    b = img.get_editor_property("brush")
    b.set_editor_property("resource_object", tex)
    b.set_editor_property("draw_as",
                          unreal.SlateBrushDrawType.BOX if draw == "box" else unreal.SlateBrushDrawType.IMAGE)
    if margin is not None:
        b.set_editor_property("margin", unreal.Margin(margin, margin, margin, margin))
    try:
        b.set_editor_property("image_size", unreal.DeprecateSlateVector2D(float(w_px), float(h_px)))
    except Exception:
        try:
            b.set_editor_property("image_size", [float(w_px), float(h_px)])
        except Exception:
            pass  # 크기 미설정 시 브러시 기본값 — 슬롯 크기가 렌더 박스를 지배한다.
    img.set_editor_property("brush", b)


def make_brush(tex, w_px, h_px, draw="image", margin=None):
    b = unreal.SlateBrush()
    b.set_editor_property("resource_object", tex)
    b.set_editor_property("draw_as",
                          unreal.SlateBrushDrawType.BOX if draw == "box" else unreal.SlateBrushDrawType.IMAGE)
    if margin is not None:
        b.set_editor_property("margin", unreal.Margin(margin, margin, margin, margin))
    try:
        b.set_editor_property("image_size", unreal.DeprecateSlateVector2D(float(w_px), float(h_px)))
    except Exception:
        try:
            b.set_editor_property("image_size", [float(w_px), float(h_px)])
        except Exception:
            pass
    return b


def pin_slot(w, ax, ay, rect, z=None):
    """포인트 앵커(ax,ay) + design px 오프셋(DPI 1080 기준) — 레퍼런스 빌더와 동일 수식."""
    ld = unreal.AnchorData(
        offsets=unreal.Margin(rect[0] - ax * DW, rect[1] - ay * DH, rect[2], rect[3]),
        anchors=unreal.Anchors(minimum=unreal.Vector2D(ax, ay), maximum=unreal.Vector2D(ax, ay)),
        alignment=unreal.Vector2D(0.0, 0.0))
    w.slot.set_editor_property("layout_data", ld)
    if z is not None:
        w.slot.set_editor_property("z_order", int(z))


def fill_slot(w, z=None):
    ld = unreal.AnchorData(
        offsets=unreal.Margin(0, 0, 0, 0),
        anchors=unreal.Anchors(minimum=unreal.Vector2D(0, 0), maximum=unreal.Vector2D(1, 1)),
        alignment=unreal.Vector2D(0.0, 0.0))
    w.slot.set_editor_property("layout_data", ld)
    if z is not None:
        w.slot.set_editor_property("z_order", int(z))


def text_center_slot(w, ax, ay, box_rect, z=None):
    """텍스트: 박스 중심점 + autosize + 정렬(0.5,0.5) — 길이 무관, 벗어남 방지(레퍼런스 패턴)."""
    cx, cy = box_rect[0] + box_rect[2] / 2.0, box_rect[1] + box_rect[3] / 2.0
    ld = unreal.AnchorData(
        offsets=unreal.Margin(cx - ax * DW, cy - ay * DH, 0.0, 0.0),
        anchors=unreal.Anchors(minimum=unreal.Vector2D(ax, ay), maximum=unreal.Vector2D(ax, ay)),
        alignment=unreal.Vector2D(0.5, 0.5))
    w.slot.set_editor_property("layout_data", ld)
    w.slot.set_editor_property("auto_size", True)
    if z is not None:
        w.slot.set_editor_property("z_order", int(z))


def set_font_size(t, size):
    f = t.get_editor_property("font")
    f.set_editor_property("size", int(max(12, min(30, size))))
    t.set_editor_property("font", f)


def style_text(t, box_h, color=None, center=True):
    if center:
        t.set_editor_property("justification", unreal.TextJustify.CENTER)   # set_justification 없음
    if color is not None:
        t.set_color_and_opacity(unreal.SlateColor(unreal.LinearColor(*color)))
    set_font_size(t, box_h * 0.55)


# ---------------- WBP 트리 유틸 ----------------
def find_w(bp, name):
    try:
        return unreal.EditorUtilityLibrary.find_source_widget_by_name(bp, name)
    except Exception:
        return None


def find_root(bp, probe_names):
    """widget_tree.root_widget 우선, 실패 시 알려진 위젯에서 get_parent()로 최상단까지 등반."""
    try:
        tree = bp.get_editor_property("widget_tree")
        if tree is not None:
            r = tree.get_editor_property("root_widget")
            if r is not None:
                return r
    except Exception:
        pass
    for n in probe_names:
        w = find_w(bp, n)
        if w is not None:
            top = w
            while True:
                p = top.get_parent()
                if p is None:
                    return top
                top = p
    return None


def first_canvas(root):
    if root is None:
        return None
    queue = [root]
    while queue:
        w = queue.pop(0)
        if isinstance(w, unreal.CanvasPanel):
            return w
        try:
            for i in range(w.get_children_count()):
                queue.append(w.get_child_at(i))
        except Exception:
            pass
    return None


def dump_tree(root):
    lines = []

    def rec(w, depth):
        if w is None:
            return
        lines.append("  " * depth + w.get_name() + " (" + type(w).__name__ + ")")
        try:
            for i in range(w.get_children_count()):
                rec(w.get_child_at(i), depth + 1)
        except Exception:
            pass

    rec(root, 0)
    return lines


def ensure_widget(bp, cls, name, parent_name):
    w = find_w(bp, name)
    if w is not None:
        return w
    w = unreal.EditorUtilityLibrary.add_source_widget(bp, cls, name, parent_name)
    if w is not None:
        res["created"].append(name)
    return w


def ensure_on_canvas(w, canvas):
    """기존 위젯이 캔버스 밖(하단 HBox 등)에 있으면 루트 캔버스로 재부모화(add_child_to_canvas/remove_child)."""
    slot = w.slot
    if slot is not None and type(slot).__name__ == "CanvasPanelSlot" and w.get_parent() == canvas:
        return
    if slot is not None and type(slot).__name__ == "CanvasPanelSlot":
        return  # 이미 다른 캔버스(그래프 캔버스 등) 소속이면 건드리지 않는다
    parent = w.get_parent()
    if parent is not None:
        parent.remove_child(w)
    canvas.add_child_to_canvas(w)
    res["reparented"].append(w.get_name())


def collapse_if_found(bp, names):
    for n in names:
        w = find_w(bp, n)
        if w is None:
            continue
        try:
            w.set_visibility(unreal.SlateVisibility.COLLAPSED)
            res["collapsed"].append(n)
        except Exception as ex:
            err("map", n + " collapse " + str(ex)[:50])


def get_cdo(asset_path):
    gen_path = asset_path + "." + asset_path.rsplit("/", 1)[-1] + "_C"
    cls = None
    try:
        cls = unreal.load_object(None, gen_path)
    except Exception:
        cls = None
    if cls is None:
        try:
            cls = unreal.load_class(None, gen_path)
        except Exception:
            cls = None
    if cls is None:
        try:
            cls = unreal.EditorAssetLibrary.load_blueprint_class(asset_path)
        except Exception:
            cls = None
    if cls is None:
        return None
    try:
        return unreal.get_default_object(cls)
    except Exception:
        return None


def set_cdo_props(cdo, scope, pairs):
    """컴파일된 새 UPROPERTY에 클래스 디폴트 주입. 프로퍼티 부재(=C++ 미컴파일)는 에러로 기록."""
    for prop, value in pairs:
        if value is None:
            res["missing"].append(scope + "-cdo-value:" + prop)
            continue
        try:
            cdo.set_editor_property(prop, value)
            res["synced"].append(scope + ":cdo:" + prop)
        except Exception as ex:
            err(scope, "cdo " + prop + " " + str(ex)[:70])


def scope_errors(scope):
    return [e for e in res["errors"] if e.startswith(scope + ":")]


def compile_and_save(bp, asset_path, scope, key):
    """save_asset은 에디터가 열려 있으면 False(조용한 실패) — 반환값을 saved 플래그로 그대로 노출."""
    if scope_errors(scope):
        res["saved"][key] = False
        res["notes"].append(key + ": errors present, save skipped")
        return
    try:
        unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    except Exception as ex:
        err(scope, "compile " + str(ex)[:80])
        res["saved"][key] = False
        return
    ok = bool(unreal.EditorAssetLibrary.save_asset(asset_path))
    res["saved"][key] = ok
    if not ok:
        res["notes"].append(key + ": save_asset returned False (editor open? asset locked?)")


# 텍스트 색: 네이비/골드 패널 위 밝은 톤, 양피지 위 잉크 톤 (시안 무드 기준의 재량값)
COL_LIGHT = (0.88, 0.91, 0.96, 1.0)
COL_GOLD = (0.95, 0.87, 0.60, 1.0)
COL_INK = (0.30, 0.22, 0.12, 0.85)

# 범례 행 한글 키 -> 위젯 접미사(ASCII, 결정적 이름 = idempotent 재실행 안전)
LEGEND_KEY = {"전투": "Battle", "엘리트": "Elite", "보스": "Boss", "상점": "Shop", "보물": "Treasure", "휴식": "Rest"}


# ==================================================================================
# 1) WBP_FrontendMap
# ==================================================================================
def sync_main_map():
    bp = unreal.EditorAssetLibrary.load_asset(MAP_PATH)
    if bp is None:
        err("map", "load failed " + MAP_PATH)
        return
    root = find_root(bp, ["MapScrollBox", "MapGraphCanvas", "CloseButton", "MapStatusText"])
    res["trees"]["map_before"] = dump_tree(root)
    canvas = first_canvas(root)
    if canvas is None:
        err("map", "root canvas not found")
        return
    root_name = canvas.get_name()
    graph = find_w(bp, "MapGraphCanvas")
    if graph is None:
        err("map", "MapGraphCanvas missing")
        return

    # ---- 텍스처 준비 (JSON imageAsset 정본, T_wm_* 로 임포트) ----
    t_scrim = ensure_import(region_image("bg_scrim"))
    t_parch = ensure_import(region_image("parchment_body"))
    t_rod_top = ensure_import(region_image("scroll_rod_top"))
    t_rod_bot = ensure_import(region_image("scroll_rod_bottom"))
    t_marker = ensure_import(region_image("marker_current"))
    t_glow = ensure_import(region_image("next_select_glow"))
    t_panel = ensure_import(region_image("stage_info_panel"))
    t_candle = ensure_import(region_image("decor_candle"))
    t_book = ensure_import(region_image("decor_spellbook"))
    t_strip = ensure_import(region_image("selected_room_strip"))
    close_src = region_image("btn_close")
    enter_src = region_image("btn_enter")
    t_close_n = ensure_import(close_src)
    t_close_p = ensure_import(close_src.replace("_normal", "_pressed")) if close_src else None
    t_enter_n = ensure_import(enter_src)
    t_enter_p = ensure_import(enter_src.replace("_normal", "_pressed")) if enter_src else None

    # ---- Map_Scrim: 풀앵커, zorder 맨 뒤(-300), HitTestInvisible ----
    try:
        w = ensure_widget(bp, unreal.Image, "Map_Scrim", root_name)
        if w is not None:
            fill_slot(w, z=-300)
            w.set_visibility(unreal.SlateVisibility.HIT_TEST_INVISIBLE)
            if t_scrim is not None:
                apply_brush(w, t_scrim, DW, DH)
            res["synced"].append("Map_Scrim")
    except Exception as ex:
        err("map", "Map_Scrim " + str(ex)[:70])

    # ---- MapScrollBox: 풀블리드 앵커 ----
    try:
        w = find_w(bp, "MapScrollBox")
        if w is None:
            res["missing"].append("MapScrollBox")
        else:
            ensure_on_canvas(w, canvas)
            fill_slot(w)
            res["synced"].append("MapScrollBox(fullbleed)")
    except Exception as ex:
        err("map", "MapScrollBox " + str(ex)[:70])

    # ---- Map_ScrollHint: 텍스트 힌트(center/top 핀, 박스중심+autosize) ----
    try:
        w = find_w(bp, "Map_ScrollHint")
        if w is None:
            w = ensure_widget(bp, unreal.TextBlock, "Map_ScrollHint", root_name)
            if w is not None:
                w.set_text(unreal.Text("드래그하여 지도 탐색"))
        if w is not None:
            ax, ay = pin_of("hint_scroll")
            rect = elem_abs("hint_scroll")
            text_center_slot(w, ax, ay, rect, z=12)
            style_text(w, rect[3], COL_INK)
            w.set_visibility(unreal.SlateVisibility.HIT_TEST_INVISIBLE)
            res["synced"].append("Map_ScrollHint")
    except Exception as ex:
        err("map", "Map_ScrollHint " + str(ex)[:70])

    # ---- 프레임/장식 이미지들: edge-pin + 브러시 (프레임은 BOX 9-slice로 모서리 보존) ----
    ART = [
        ("Map_StageInfoPanel", "stage_info_panel", t_panel, "box", 0.25),
        ("Map_LegendPanel", "legend_panel", t_panel, "box", 0.25),
        ("Map_DecorCandle", "decor_candle", t_candle, "image", None),
        ("Map_DecorSpellbook", "decor_spellbook", t_book, "image", None),
        ("Map_SelectedRoomStrip", "selected_room_strip", t_strip, "box", 0.30),
    ]
    for name, el, tex, draw, margin in ART:
        try:
            w = ensure_widget(bp, unreal.Image, name, root_name)
            if w is None:
                res["missing"].append(name)
                continue
            ax, ay = pin_of(el)
            rect = elem_abs(el)
            pin_slot(w, ax, ay, rect, z=10)
            w.set_visibility(unreal.SlateVisibility.HIT_TEST_INVISIBLE)
            if tex is not None:
                apply_brush(w, tex, rect[2], rect[3], draw=draw, margin=margin)
            res["synced"].append(name)
        except Exception as ex:
            err("map", name + " " + str(ex)[:70])

    # ---- stage_info_panel 내부 TextBlock 2개 (텍스트는 비움 — C++이 채움) ----
    try:
        ax, ay = pin_of("stage_info_panel")
        name_rect = region_abs("stage_info_panel", rg_type="text", index=0)
        prog_rect = region_abs("stage_info_panel", rg_type="text", index=1)
        for tname, rect, color in [("Map_StageNameText", name_rect, COL_GOLD),
                                   ("Map_StageProgressText", prog_rect, COL_LIGHT)]:
            if rect is None:
                res["missing"].append(tname + ":region")
                continue
            w = find_w(bp, tname)
            if w is None:
                w = ensure_widget(bp, unreal.TextBlock, tname, root_name)
                if w is not None:
                    w.set_text(unreal.Text(""))
            if w is not None:
                text_center_slot(w, ax, ay, rect, z=12)
                style_text(w, rect[3], color)
                w.set_visibility(unreal.SlateVisibility.HIT_TEST_INVISIBLE)
                res["synced"].append(tname)
    except Exception as ex:
        err("map", "stage texts " + str(ex)[:70])

    # ---- legend_panel: 아이콘(기존 T_MapNode_* 재사용) + 라벨 TextBlock 행들 + 제목 ----
    try:
        ax, ay = pin_of("legend_panel")
        title_rect = region_abs("legend_panel", rg_name_prefix="범례제목")
        if title_rect is not None:
            w = find_w(bp, "Map_LegendTitleText")
            if w is None:
                w = ensure_widget(bp, unreal.TextBlock, "Map_LegendTitleText", root_name)
                if w is not None:
                    w.set_text(unreal.Text("범례"))
            if w is not None:
                text_center_slot(w, ax, ay, title_rect, z=12)
                style_text(w, title_rect[3], COL_GOLD)
                w.set_visibility(unreal.SlateVisibility.HIT_TEST_INVISIBLE)
                res["synced"].append("Map_LegendTitleText")
        for r in els["legend_panel"].get("regions", []):
            rname = r.get("name", "")
            if not rname.startswith("범례아이콘_"):
                continue
            key_kr = rname.split("_", 1)[1]
            suffix = LEGEND_KEY.get(key_kr)
            if suffix is None:
                res["notes"].append("legend key unmapped: " + key_kr)
                continue
            icon_rect = region_abs("legend_panel", rg_name_prefix=rname)
            label_rect = region_abs("legend_panel", rg_name_prefix="범례라벨_" + key_kr)
            t_icon = icon_tex_for(r.get("imageAsset"))
            iw = ensure_widget(bp, unreal.Image, "Map_LegendIcon_" + suffix, root_name)
            if iw is not None and icon_rect is not None:
                pin_slot(iw, ax, ay, icon_rect, z=12)
                iw.set_visibility(unreal.SlateVisibility.HIT_TEST_INVISIBLE)
                if t_icon is not None:
                    apply_brush(iw, t_icon, icon_rect[2], icon_rect[3])
                res["synced"].append("Map_LegendIcon_" + suffix)
            lw = find_w(bp, "Map_LegendLabel_" + suffix)
            if lw is None:
                lw = ensure_widget(bp, unreal.TextBlock, "Map_LegendLabel_" + suffix, root_name)
                if lw is not None:
                    lw.set_text(unreal.Text(key_kr))
            if lw is not None and label_rect is not None:
                text_center_slot(lw, ax, ay, label_rect, z=12)
                style_text(lw, label_rect[3], COL_LIGHT)
                lw.set_visibility(unreal.SlateVisibility.HIT_TEST_INVISIBLE)
                res["synced"].append("Map_LegendLabel_" + suffix)
    except Exception as ex:
        err("map", "legend " + str(ex)[:70])

    # ---- MapTitleText: map_title rect 중심, autosize+중앙정렬, 폰트=rect높이*0.55 ----
    try:
        w = find_w(bp, "MapTitleText")
        if w is None:
            # 현재 WBP에는 MapTitleText가 없다(BindWidgetOptional) — 생성해 바인딩을 살린다
            w = ensure_widget(bp, unreal.TextBlock, "MapTitleText", root_name)
        if w is not None:
            ensure_on_canvas(w, canvas)
            ax, ay = pin_of("map_title")
            rect = elem_abs("map_title")
            text_center_slot(w, ax, ay, rect, z=12)
            style_text(w, rect[3], COL_GOLD)
            w.set_visibility(unreal.SlateVisibility.HIT_TEST_INVISIBLE)
            res["synced"].append("MapTitleText")
    except Exception as ex:
        err("map", "MapTitleText " + str(ex)[:70])

    # ---- CloseButton / EnterRoomButton: 하단 edge-pin + 버튼 스타일 브러시 ----
    for bname, el, t_n, t_p in [("CloseButton", "btn_close", t_close_n, t_close_p),
                                ("EnterRoomButton", "btn_enter", t_enter_n, t_enter_p)]:
        try:
            b = find_w(bp, bname)
            if b is None:
                res["missing"].append(bname)
                continue
            ensure_on_canvas(b, canvas)
            ax, ay = pin_of(el)
            rect = elem_abs(el)
            pin_slot(b, ax, ay, rect, z=11)
            if t_n is not None:
                st = b.get_editor_property("widget_style")
                st.set_editor_property("normal", make_brush(t_n, rect[2], rect[3]))
                st.set_editor_property("hovered", make_brush(t_n, rect[2], rect[3]))   # hovered 원본 없음 → normal
                st.set_editor_property("pressed", make_brush(t_p or t_n, rect[2], rect[3]))
                b.set_editor_property("widget_style", st)
            # 버튼 안 텍스트 중앙 정렬(레퍼런스 패턴)
            for ci in range(b.get_children_count()):
                child = b.get_child_at(ci)
                cs = child.slot
                cs.set_editor_property("horizontal_alignment", unreal.HorizontalAlignment.H_ALIGN_CENTER)
                cs.set_editor_property("vertical_alignment", unreal.VerticalAlignment.V_ALIGN_CENTER)
                cs.set_editor_property("padding", unreal.Margin(0, 0, 0, 0))
                if isinstance(child, unreal.TextBlock):
                    child.set_editor_property("justification", unreal.TextJustify.CENTER)
            res["synced"].append(bname)
        except Exception as ex:
            err("map", bname + " " + str(ex)[:70])

    # ---- MapStatusText: Map_SelectedRoomStrip 안 텍스트 리전 중앙으로 ----
    try:
        w = find_w(bp, "MapStatusText")
        if w is None:
            res["missing"].append("MapStatusText")
        else:
            ensure_on_canvas(w, canvas)
            ax, ay = pin_of("selected_room_strip")
            rect = region_abs("selected_room_strip", rg_type="text", index=0) or elem_abs("selected_room_strip")
            text_center_slot(w, ax, ay, rect, z=12)
            w.set_editor_property("justification", unreal.TextJustify.CENTER)
            set_font_size(w, rect[3] * 0.55)   # 색은 기존 디자인 유지
            res["synced"].append("MapStatusText(strip)")
    except Exception as ex:
        err("map", "MapStatusText " + str(ex)[:70])

    # ---- MapGraphCanvas(스크롤 콘텐츠) 안 ----
    # Map_ParchmentBody: 슬롯은 임의(C++이 매 리프레시 크기 동기), zorder -200
    try:
        w = ensure_widget(bp, unreal.Image, "Map_ParchmentBody", "MapGraphCanvas")
        if w is not None:
            prect = region_abs("parchment_body", rg_type="content") or [0, 0, 1240, 812]
            pin_slot(w, 0.0, 0.0, [0.0, 0.0, prect[2], prect[3]], z=-200)
            w.set_visibility(unreal.SlateVisibility.HIT_TEST_INVISIBLE)
            if t_parch is not None:
                apply_brush(w, t_parch, prect[2], prect[3])
                try:
                    b = w.get_editor_property("brush")
                    b.set_editor_property("tiling", unreal.SlateBrushTileType.VERTICAL)  # 세로 타일링(시안 노트)
                    w.set_editor_property("brush", b)
                except Exception:
                    pass
            res["synced"].append("Map_ParchmentBody")
    except Exception as ex:
        err("map", "Map_ParchmentBody " + str(ex)[:70])

    # Map_ScrollRodTop/Bottom: 높이=concept rect 높이(84), 폭/위치는 C++ 동기, zorder -190
    for name, el, tex, y0 in [("Map_ScrollRodTop", "scroll_rod_top", t_rod_top, 0.0),
                              ("Map_ScrollRodBottom", "scroll_rod_bottom", t_rod_bot, 728.0)]:
        try:
            w = ensure_widget(bp, unreal.Image, name, "MapGraphCanvas")
            if w is None:
                continue
            rod_h = elem_abs(el)[3]
            rod_w = (region_abs(el, rg_type="content") or [0, 0, 1320, rod_h])[2]
            pin_slot(w, 0.0, 0.0, [0.0, y0, rod_w, rod_h], z=-190)   # 위치/폭은 자리표시 — C++ 동기
            w.set_visibility(unreal.SlateVisibility.HIT_TEST_INVISIBLE)
            if tex is not None:
                apply_brush(w, tex, rod_w, rod_h)
            res["synced"].append(name)
        except Exception as ex:
            err("map", name + " " + str(ex)[:70])

    # Map_NodeArea 마커(Border, 투명, HitTestInvisible):
    # 앵커 X = x/1920 분수(min/max), 앵커 Y = (0,1), 오프셋 Left/Right=0, Top=y, Bottom=1080-y-h
    try:
        w = ensure_widget(bp, unreal.Border, "Map_NodeArea", "MapGraphCanvas")
        if w is not None:
            na = elem_abs("node_area")
            ld = unreal.AnchorData(
                offsets=unreal.Margin(0.0, na[1], 0.0, DH - na[1] - na[3]),
                anchors=unreal.Anchors(minimum=unreal.Vector2D(na[0] / DW, 0.0),
                                       maximum=unreal.Vector2D((na[0] + na[2]) / DW, 1.0)),
                alignment=unreal.Vector2D(0.0, 0.0))
            w.slot.set_editor_property("layout_data", ld)
            w.set_brush_color(unreal.LinearColor(0, 0, 0, 0))
            w.set_editor_property("padding", unreal.Margin(0, 0, 0, 0))
            w.set_visibility(unreal.SlateVisibility.HIT_TEST_INVISIBLE)
            res["synced"].append("Map_NodeArea")
    except Exception as ex:
        err("map", "Map_NodeArea " + str(ex)[:70])

    # Map_NodeMetrics / Map_ColPitch (SizeBox, Collapsed — C++이 값만 읽는 마커)
    for name, wo, ho in [("Map_NodeMetrics", NODE_SIZE, ROW_PITCH),
                         ("Map_ColPitch", COL_PITCH_MAX, BOSS_NODE_SIZE)]:
        try:
            w = ensure_widget(bp, unreal.SizeBox, name, "MapGraphCanvas")
            if w is not None:
                w.set_width_override(float(wo))
                w.set_height_override(float(ho))
                pin_slot(w, 0.0, 0.0, [0.0, 0.0, float(wo), float(ho)])
                w.set_visibility(unreal.SlateVisibility.COLLAPSED)
                res["synced"].append(name)
        except Exception as ex:
            err("map", name + " " + str(ex)[:70])

    # Map_CurrentMarker / Map_SelectGlow (Collapsed — C++이 이동/표시)
    for name, el, tex, z in [("Map_CurrentMarker", "marker_current", t_marker, 20),
                             ("Map_SelectGlow", "next_select_glow", t_glow, -10)]:
        try:
            w = ensure_widget(bp, unreal.Image, name, "MapGraphCanvas")
            if w is not None:
                rect = elem_abs(el)
                pin_slot(w, 0.0, 0.0, rect, z=z)   # 초기 위치는 시안값 — C++이 노드 위치로 이동
                w.set_visibility(unreal.SlateVisibility.COLLAPSED)
                if tex is not None:
                    apply_brush(w, tex, rect[2], rect[3])
                res["synced"].append(name)
        except Exception as ex:
            err("map", name + " " + str(ex)[:70])

    # ---- 레거시 정리: 구형 위젯 Collapsed (C++ HideUnusedMapTextSurfaces 의존 제거) ----
    collapse_if_found(bp, ["MapPaperPanel", "MapPaperShadow", "MapDimBackground",
                           "MapPreviewPanel", "MapPreviewTitleText", "MapPreviewDescriptionText",
                           "MapPreviewStateText", "MapLegendScroll", "MapLegendList", "MapLegendTitle"])

    res["trees"]["map_after"] = dump_tree(find_root(bp, ["MapScrollBox"]))
    compile_and_save(bp, MAP_PATH, "map", "WBP_FrontendMap")


# ==================================================================================
# 2) WBP_FrontendMapNode — NodeRingImage 신설 + 링/아이콘 클래스 디폴트
# ==================================================================================
def sync_node_wbp():
    bp = unreal.EditorAssetLibrary.load_asset(NODE_PATH)
    if bp is None:
        err("node", "load failed " + NODE_PATH)
        return
    root = find_root(bp, ["NodeOverlay", "NodeButton", "NodePanel", "NodeRootSize"])
    res["trees"]["node"] = dump_tree(root)

    # 텍스처: 상태 링 4종 임포트 + 기존 노드 아이콘 5종 로드(재임포트 금지)
    rings = {}
    for state in ("normal", "current", "locked", "cleared"):
        rings[state] = ensure_import(ASSETS + "12_WorldMap/02_NodeFrame_States/node_ring_%s.png" % state)
    icons = {
        "mIconMonsterTexture": load_mapnode("T_MapNode_Monster"),
        "mIconEliteTexture": load_mapnode("T_MapNode_Elite"),
        "mIconBossTexture": load_mapnode("T_MapNode_Boss"),
        "mIconShopTexture": load_mapnode("T_MapNode_Shop"),
        "mIconTreasureTexture": load_mapnode("T_MapNode_Treasure"),
    }

    # NodeRingImage: 아이콘 위 오버레이 링(뒤에 add → 맨 위), HitTestInvisible
    try:
        ring = find_w(bp, "NodeRingImage")
        if ring is None:
            overlay = find_w(bp, "NodeOverlay")
            parent_name = None
            if overlay is not None:
                parent_name = "NodeOverlay"
            else:
                cv = first_canvas(root)
                if cv is not None:
                    parent_name = cv.get_name()
            if parent_name is None:
                err("node", "no Overlay/Canvas parent for NodeRingImage")
            else:
                ring = unreal.EditorUtilityLibrary.add_source_widget(bp, unreal.Image, "NodeRingImage", parent_name)
                if ring is not None:
                    res["created"].append("NodeRingImage")
        if ring is not None:
            slot = ring.slot
            sname = type(slot).__name__ if slot is not None else ""
            if sname == "CanvasPanelSlot":
                fill_slot(ring)
            else:
                try:
                    slot.set_editor_property("horizontal_alignment", unreal.HorizontalAlignment.H_ALIGN_FILL)
                    slot.set_editor_property("vertical_alignment", unreal.VerticalAlignment.V_ALIGN_FILL)
                except Exception:
                    pass
            ring.set_visibility(unreal.SlateVisibility.HIT_TEST_INVISIBLE)
            if rings.get("normal") is not None:
                apply_brush(ring, rings["normal"], NODE_SIZE, NODE_SIZE)  # 디자이너 프리뷰용 — 런타임은 C++이 교체
            res["synced"].append("NodeRingImage")
    except Exception as ex:
        err("node", "NodeRingImage " + str(ex)[:70])

    # 위젯 편집을 먼저 컴파일해 generated_class를 최신화한 뒤 CDO에 클래스 디폴트를 넣고 저장한다.
    if scope_errors("node"):
        res["saved"]["WBP_FrontendMapNode"] = False
        return
    try:
        unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    except Exception as ex:
        err("node", "compile " + str(ex)[:80])
        res["saved"]["WBP_FrontendMapNode"] = False
        return
    cdo = get_cdo(NODE_PATH)
    if cdo is None:
        err("node", "CDO load failed")
    else:
        set_cdo_props(cdo, "node", [
            ("mRingNormalTexture", rings.get("normal")),
            ("mRingCurrentTexture", rings.get("current")),
            ("mRingLockedTexture", rings.get("locked")),
            ("mRingClearedTexture", rings.get("cleared")),
        ] + list(icons.items()))
    if scope_errors("node"):
        res["saved"]["WBP_FrontendMapNode"] = False
        res["notes"].append("WBP_FrontendMapNode: errors present, save skipped")
        return
    ok = bool(unreal.EditorAssetLibrary.save_asset(NODE_PATH))
    res["saved"]["WBP_FrontendMapNode"] = ok
    if not ok:
        res["notes"].append("WBP_FrontendMapNode: save_asset returned False (editor open?)")


# ==================================================================================
# 3) WBP_FrontendMapLine — LineImage 신설 + 경로 텍스처/두께 클래스 디폴트
# ==================================================================================
def sync_line_wbp():
    bp = unreal.EditorAssetLibrary.load_asset(LINE_PATH)
    if bp is None:
        err("line", "load failed " + LINE_PATH)
        return
    root = find_root(bp, ["LinePanel", "LineRootSize"])
    res["trees"]["line"] = dump_tree(root)

    cd = els.get("path_layer", {}).get("wbp", {}).get("classDefaults", {})
    t_solid = ensure_import(ASSETS + cd.get("mSolidTexture", "12_WorldMap/03_PathConnectors/path_connector.png"))
    t_dash = ensure_import(ASSETS + cd.get("mDashedTexture", "12_WorldMap/03_PathConnectors/path_dot.png"))
    thickness = float(cd.get("mLineThickness", 14.0))

    # LineImage: LinePanel과 나란히가 계약이지만 현 트리 루트가 SizeBox(단일 자식)라
    # 부모가 Overlay/Canvas일 때만 형제로, 아니면 LinePanel(Border) 콘텐츠로 fill 배치한다.
    try:
        img = find_w(bp, "LineImage")
        if img is None:
            lp = find_w(bp, "LinePanel")
            parent_name = None
            placement = ""
            if lp is not None:
                par = lp.get_parent()
                if par is not None and type(par).__name__ in ("Overlay", "CanvasPanel"):
                    parent_name = par.get_name()
                    placement = "sibling"
                else:
                    parent_name = "LinePanel"
                    placement = "inside LinePanel (root SizeBox holds single child)"
            else:
                cv = first_canvas(root)
                if cv is not None:
                    parent_name = cv.get_name()
                    placement = "root canvas fallback"
            if parent_name is None:
                err("line", "no parent for LineImage")
            else:
                img = unreal.EditorUtilityLibrary.add_source_widget(bp, unreal.Image, "LineImage", parent_name)
                if img is not None:
                    res["created"].append("LineImage")
                    res["notes"].append("LineImage placement: " + placement)
        if img is not None:
            slot = img.slot
            sname = type(slot).__name__ if slot is not None else ""
            if sname == "CanvasPanelSlot":
                fill_slot(img)
            elif slot is not None:
                try:
                    slot.set_editor_property("horizontal_alignment", unreal.HorizontalAlignment.H_ALIGN_FILL)
                    slot.set_editor_property("vertical_alignment", unreal.VerticalAlignment.V_ALIGN_FILL)
                except Exception:
                    pass
                try:
                    slot.set_editor_property("padding", unreal.Margin(0, 0, 0, 0))
                except Exception:
                    pass
            lp = find_w(bp, "LinePanel")
            if lp is not None:
                try:
                    lp.set_editor_property("padding", unreal.Margin(0, 0, 0, 0))  # Border 기본 패딩 제거 → 진짜 fill
                except Exception:
                    pass
            img.set_visibility(unreal.SlateVisibility.HIT_TEST_INVISIBLE)
            if t_solid is not None:
                apply_brush(img, t_solid, 220.0, thickness)  # 디자이너 프리뷰용 — 런타임은 C++이 solid/dashed 교체
            res["synced"].append("LineImage")
    except Exception as ex:
        err("line", "LineImage " + str(ex)[:70])

    if scope_errors("line"):
        res["saved"]["WBP_FrontendMapLine"] = False
        return
    try:
        unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    except Exception as ex:
        err("line", "compile " + str(ex)[:80])
        res["saved"]["WBP_FrontendMapLine"] = False
        return
    cdo = get_cdo(LINE_PATH)
    if cdo is None:
        err("line", "CDO load failed")
    else:
        set_cdo_props(cdo, "line", [
            ("mSolidTexture", t_solid),
            ("mDashedTexture", t_dash),
            ("mLineThickness", thickness),
        ])
    if scope_errors("line"):
        res["saved"]["WBP_FrontendMapLine"] = False
        res["notes"].append("WBP_FrontendMapLine: errors present, save skipped")
        return
    ok = bool(unreal.EditorAssetLibrary.save_asset(LINE_PATH))
    res["saved"]["WBP_FrontendMapLine"] = ok
    if not ok:
        res["notes"].append("WBP_FrontendMapLine: save_asset returned False (editor open?)")


# ==================================================================================
# main
# ==================================================================================
def main():
    # targetProject 하드체크 — 20260702가 런타임 리포. 다른 리포에서 실행되면 아무것도 만지지 않는다.
    try:
        proj = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
    except Exception:
        proj = ""
    proj_n = proj.replace("\\", "/").rstrip("/").lower()
    expect = str(doc.get("targetProject", TARGET_PROJECT)).replace("\\", "/").rstrip("/").lower()
    res["project"] = proj_n
    if proj_n != expect:
        err("guard", "project mismatch: running=" + proj_n + " expected=" + expect)
        return

    # 원본 백업 (재실행 시 최초 상태 보존을 위해 이미 있으면 덮지 않는다)
    for f in ("WBP_FrontendMap", "WBP_FrontendMapNode", "WBP_FrontendMapLine"):
        src = CONTENT_UI + f + ".uasset"
        bak = src + ".bak_pre_worldmap_sync"
        try:
            if os.path.exists(src) and not os.path.exists(bak):
                shutil.copy2(src, bak)
        except Exception as ex:
            res["notes"].append("backup " + f + ": " + str(ex)[:60])

    sync_main_map()
    sync_node_wbp()
    sync_line_wbp()


try:
    main()
except Exception as ex:
    import traceback
    err("fatal", str(ex)[:150])
    res["notes"].append(traceback.format_exc()[-400:])

res["counts"] = {"created": len(res["created"]), "synced": len(res["synced"]),
                 "imported": len(res["imported"]), "collapsed": len(res["collapsed"]),
                 "reparented": len(res["reparented"]), "missing": len(res["missing"]),
                 "errors": len(res["errors"])}
print("WORLDMAPSYNC|" + json.dumps(res, ensure_ascii=False))
