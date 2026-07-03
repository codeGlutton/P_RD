# WBP_SettingsPanel 슬롯 싱크: 복구된 concept_settings_claude02.json(216)을 정본으로
# Set_* 아트/컨트롤 위젯의 슬롯만 재배치한다(바인딩/배선/브러시 불가침).
#  - 우측 열 720px 복원, 손상 좌표 교정, btn_close→우상단 핀, footnote 텍스트 신설
#  - 폴드 마커(Set_grid_fold, 무렌더 Spacer): 그리드 폴드 스케일의 디자이너 정의(C++ 컷<1.68이 소비)
import unreal, json, os, shutil

CONCEPT = "D:/UnrealProjects/P_RD_develop_20260701_216/Start_CombatUIRectEditor/concept_settings_claude02.json"
WPATH = "/Game/BP/UI/WBP_SettingsPanel"
UASSET = "D:/UnrealProjects/P_RD_develop_20260702/Content/BP/UI/WBP_SettingsPanel.uasset"
DW, DH, CX, CY = 1920.0, 1080.0, 960.0, 540.0

doc = json.load(open(CONCEPT, encoding="utf-8"))
els = {e["name"]: e for e in doc["screens"][0]["elements"]}


def region_abs(el_name, rg_name):
    e = els[el_name]
    ex, ey, ew, eh = e["screenRect"]
    ids = e.get("innerDesignSize") or [ew, eh]
    sx, sy = ew / ids[0], eh / ids[1]
    for r in e.get("regions", []):
        if r.get("name") == rg_name:
            rx, ry, rw, rh = r["rect"]
            return [ex + rx * sx, ey + ry * sy, rw * sx, rh * sy]
    return None


def elem_abs(el_name):
    return list(map(float, els[el_name]["screenRect"]))


# ---- 목표 슬롯 테이블: name -> (anchor(ax,ay), 절대rect) ----
# anchor: C=(0.5,0.5) 중앙블록, RT=(1,0), LB=(0,1), RB=(1,1), CB=(0.5,1), CT=(0.5,0), FILL=None
T = {}
def C(name, rect):  T[name] = ((0.5, 0.5), rect)
def pin(name, ax, ay, rect): T[name] = ((ax, ay), rect)

T["Set_scrim"] = (None, None)          # fill 유지
T["Set_panel_frame"] = (None, None)    # concept: stretch 풀블리드 → fill로 전환
pin("SettingsTitleText", 0.5, 0.0, elem_abs("header"))
pin("Set_btn_close", 1.0, 0.0, elem_abs("btn_close"))

C("Set_sec_graphics_banner", elem_abs("sec_graphics_header"))
C("Set_sec_graphics_icon", region_abs("sec_graphics_header", "아이콘_그래픽"))
C("Set_row_fps_plate", elem_abs("row_fps"))
C("Set_row_fps_label", region_abs("row_fps", "라벨_FPS"))
# 세그 통일: 옵션별 단일 프레임 리전 — 버튼 rect = 프레임 rect (히트영역 = 필 전체)
C("FpsThirtyButton", region_abs("row_fps", "세그틀_30"))
C("FpsSixtyButton", region_abs("row_fps", "세그틀_60"))
C("Set_row_quality_plate", elem_abs("row_quality"))
C("QualityRow_Label", region_abs("row_quality", "라벨_그래픽품질"))
C("QualityLowButton", region_abs("row_quality", "세그틀_하"))
C("QualityMidButton", region_abs("row_quality", "세그틀_중"))
C("QualityHighButton", region_abs("row_quality", "세그틀_상"))
C("Set_row_screenshake_plate", elem_abs("row_screenshake"))
C("ScreenShakeRow_Label", region_abs("row_screenshake", "라벨_화면흔들림"))
C("ScreenShakeCheckBox", region_abs("row_screenshake", "체크박스_화면흔들림_켬"))
C("Set_row_effects_plate", elem_abs("row_effects"))
C("Set_row_effects_label", region_abs("row_effects", "라벨_이펙트"))
C("EffectsCheckBox", region_abs("row_effects", "체크박스_이펙트_끔"))

C("Set_sec_volume_banner", elem_abs("sec_volume_header"))
C("Set_sec_volume_icon", region_abs("sec_volume_header", "아이콘_음량"))
def slider_rect(row, track, knob):
    t = region_abs(row, track); k = region_abs(row, knob)
    return [t[0], t[1] + t[3] / 2 - k[3] / 2, t[2], k[3]]
C("Set_row_master_plate", elem_abs("row_master"))
C("MasterVolumeRow_Label", region_abs("row_master", "라벨_전체음량"))
C("MasterVolumeSlider", slider_rect("row_master", "슬라이더트랙_전체", "슬라이더노브_전체"))
C("Set_row_bgm_plate", elem_abs("row_bgm"))
C("BGMVolumeRow_Label", region_abs("row_bgm", "라벨_배경음"))
C("BGMVolumeSlider", slider_rect("row_bgm", "슬라이더트랙_배경음", "슬라이더노브_배경음"))
C("Set_row_sfx_plate", elem_abs("row_sfx"))
C("SFXVolumeRow_Label", region_abs("row_sfx", "라벨_효과음"))
C("SFXVolumeSlider", slider_rect("row_sfx", "슬라이더트랙_효과음", "슬라이더노브_효과음"))
C("Set_sec_gameplay_banner", elem_abs("sec_gameplay_header"))
C("Set_sec_gameplay_icon", region_abs("sec_gameplay_header", "아이콘_게임플레이"))
C("Set_row_language_plate", elem_abs("row_language"))
C("LanguageRow_Label", region_abs("row_language", "라벨_언어"))
C("LanguageKoreanButton", region_abs("row_language", "세그틀_한국어"))
C("LanguageEnglishButton", region_abs("row_language", "세그틀_English"))

pin("Set_btn_back_frame", 0.0, 1.0, elem_abs("btn_back"))
pin("BackButton", 0.0, 1.0, elem_abs("btn_back"))
pin("Set_btn_save_frame", 1.0, 1.0, elem_abs("btn_save"))
pin("SaveAndExitButton", 1.0, 1.0, elem_abs("btn_save"))
pin("Set_btn_abandon_frame", 1.0, 1.0, elem_abs("btn_abandon_run_small"))
pin("AbandonRunButton", 1.0, 1.0, elem_abs("btn_abandon_run_small"))
pin("Set_btn_reset_frame", 1.0, 1.0, elem_abs("btn_reset_small"))
pin("ResetButton", 1.0, 1.0, elem_abs("btn_reset_small"))

# 신설: footnote 텍스트(center/bottom, concept에 있을 때만), 폴드 그리드 마커(무렌더 Spacer, C++ 컷 소비)
NEW_FOOTNOTE = ("SettingsFootnoteText", (0.5, 1.0), elem_abs("footnote")) if "footnote" in els else None

# 그리드 경계는 concept에서 동적으로 계산(사용자가 base를 재조정해도 폴드 스케일이 따라간다).
GRID_NAMES = ["sec_graphics_header", "row_fps", "row_quality", "row_screenshake", "row_effects",
              "sec_volume_header", "row_master", "row_bgm", "row_sfx", "sec_gameplay_header", "row_language"]
gx0 = min(elem_abs(n)[0] for n in GRID_NAMES if n in els)
gy0 = min(elem_abs(n)[1] for n in GRID_NAMES if n in els)
gx1 = max(elem_abs(n)[0] + elem_abs(n)[2] for n in GRID_NAMES if n in els)
gy1 = max(elem_abs(n)[1] + elem_abs(n)[3] for n in GRID_NAMES if n in els)
GRID_BASE = [gx0, gy0, gx1 - gx0, gy1 - gy0]
FOLD_USABLE_W = 1199.0 - 40.0                     # 폴드(1.11:1) 디자인 폭 - 여백
FOLD_S = min(1.0, FOLD_USABLE_W / GRID_BASE[2])
GRID_FOLD = [CX - (CX - GRID_BASE[0]) * FOLD_S, CY - (CY - GRID_BASE[1]) * FOLD_S,
             GRID_BASE[2] * FOLD_S, GRID_BASE[3] * FOLD_S]

res = {"synced": [], "missing": [], "created": [], "errors": [], "saved": None}
shutil.copy2(UASSET, UASSET + ".bak_pre_sync")
bp = unreal.EditorAssetLibrary.load_asset(WPATH)


def find(n):
    try:
        return unreal.EditorUtilityLibrary.find_source_widget_by_name(bp, n)
    except Exception:
        return None


def set_slot(w, anchor, rect, keep_fill=False):
    slot = w.slot
    if keep_fill or anchor is None:
        ld = unreal.AnchorData(offsets=unreal.Margin(0, 0, 0, 0),
                               anchors=unreal.Anchors(minimum=unreal.Vector2D(0, 0), maximum=unreal.Vector2D(1, 1)),
                               alignment=unreal.Vector2D(0, 0))
    else:
        ax, ay = anchor
        ld = unreal.AnchorData(offsets=unreal.Margin(rect[0] - ax * DW, rect[1] - ay * DH, rect[2], rect[3]),
                               anchors=unreal.Anchors(minimum=unreal.Vector2D(ax, ay), maximum=unreal.Vector2D(ax, ay)),
                               alignment=unreal.Vector2D(0, 0))
    slot.set_editor_property("layout_data", ld)


if bp is None:
    res["errors"].append("load failed")
else:
    # ---- STEP 0: 모달 레터박스 재구조화 (다이스 보드 패턴, 멱등) ----
    # SettingsPanelRoot > [Set_scrim(fill, 잔류)] + [SettingsScaleBox(fill, ScaleToFit)
    #   > SettingsSizeBox(1920x1080, ScaleBoxSlot CENTER — FILL이면 tiny+corner 버그!)
    #     > SettingsModalCanvas(1920x1080 시트)] 로 바꾸고, 모달 위젯 전부를 시트 안으로 옮긴다.
    # 시트 안에서는 기존 슬롯 데이터(핀/오프셋)가 그대로 유효하다(시트=디자인 공간 1920x1080).
    KEEP_AT_ROOT = {"Set_scrim", "AbandonConfirmPanel", "SettingsDimBackground", "SettingsPanelFrame",
                    "SettingsScaleBox", "Set_grid_base", "Set_grid_fold"}
    root = find("SettingsPanelRoot")
    modal_canvas = find("SettingsModalCanvas")
    if root is None:
        res["errors"].append("SettingsPanelRoot missing")
    elif modal_canvas is None:
        try:
            scalebox = unreal.EditorUtilityLibrary.add_source_widget(bp, unreal.ScaleBox, "SettingsScaleBox", "SettingsPanelRoot")
            scalebox.set_stretch(unreal.Stretch.SCALE_TO_FIT)
            sb_slot = scalebox.slot
            sb_slot.set_editor_property("layout_data", unreal.AnchorData(
                offsets=unreal.Margin(0, 0, 0, 0),
                anchors=unreal.Anchors(minimum=unreal.Vector2D(0, 0), maximum=unreal.Vector2D(1, 1)),
                alignment=unreal.Vector2D(0, 0)))
            sb_slot.set_editor_property("z_order", 5)
            sizebox = unreal.EditorUtilityLibrary.add_source_widget(bp, unreal.SizeBox, "SettingsSizeBox", "SettingsScaleBox")
            sizebox.set_width_override(1920.0)
            sizebox.set_height_override(1080.0)
            sizebox.slot.set_editor_property("horizontal_alignment", unreal.HorizontalAlignment.H_ALIGN_CENTER)
            sizebox.slot.set_editor_property("vertical_alignment", unreal.VerticalAlignment.V_ALIGN_CENTER)
            modal_canvas = unreal.EditorUtilityLibrary.add_source_widget(bp, unreal.CanvasPanel, "SettingsModalCanvas", "SettingsSizeBox")
            modal_canvas.slot.set_editor_property("horizontal_alignment", unreal.HorizontalAlignment.H_ALIGN_FILL)
            modal_canvas.slot.set_editor_property("vertical_alignment", unreal.VerticalAlignment.V_ALIGN_FILL)
            res["created"].append("SettingsScaleBox/SizeBox/ModalCanvas")
        except Exception as ex:
            res["errors"].append("letterbox: " + str(ex)[:80])
            modal_canvas = None
    if root is not None and modal_canvas is not None and not res["errors"]:
        try:
            children = [root.get_child_at(i) for i in range(root.get_children_count())]
            moved = 0
            for w in children:
                if w is None or w.get_name() in KEEP_AT_ROOT:
                    continue
                slot = w.slot
                if type(slot).__name__ != "CanvasPanelSlot":
                    continue
                ld = slot.get_editor_property("layout_data")
                z = slot.get_editor_property("z_order")
                root.remove_child(w)
                new_slot = modal_canvas.add_child_to_canvas(w)
                new_slot.set_editor_property("layout_data", ld)
                new_slot.set_editor_property("z_order", z)
                moved += 1
            if moved:
                res["synced"].append("letterbox-moved:%d" % moved)
        except Exception as ex:
            res["errors"].append("letterbox-move: " + str(ex)[:80])

    for name, (anchor, rect) in T.items():
        w = find(name)
        if w is None:
            res["missing"].append(name); continue
        try:
            set_slot(w, anchor, rect, keep_fill=(rect is None))
            res["synced"].append(name)
        except Exception as ex:
            res["errors"].append(name + ": " + str(ex)[:60])

    # footnote 텍스트: concept에 있으면 생성/배치, 없으면(사용자 삭제) 기존 위젯을 Collapsed로 무해화.
    if NEW_FOOTNOTE is not None:
        fn_name, fn_anchor, fn_rect = NEW_FOOTNOTE
        w = find(fn_name)
        if w is None:
            try:
                w = unreal.EditorUtilityLibrary.add_source_widget(bp, unreal.TextBlock, fn_name, "SettingsPanelRoot")
                if w is not None:
                    w.set_text(unreal.Text("변경 사항은 저장 시 적용됩니다."))
                    w.set_editor_property("justification", unreal.TextJustify.CENTER)
                    w.set_color_and_opacity(unreal.SlateColor(unreal.LinearColor(0.82, 0.88, 0.95, 0.9)))
                    font = w.get_editor_property("font"); font.set_editor_property("size", 20)
                    w.set_editor_property("font", font)
                    res["created"].append(fn_name)
            except Exception as ex:
                res["errors"].append(fn_name + ": " + str(ex)[:60])
        if w is not None:
            try:
                set_slot(w, fn_anchor, fn_rect)
            except Exception as ex:
                res["errors"].append(fn_name + ": slot " + str(ex)[:50])
    else:
        w = find("SettingsFootnoteText")
        if w is not None:
            try:
                w.set_visibility(unreal.SlateVisibility.COLLAPSED)
                res["synced"].append("SettingsFootnoteText(collapsed)")
            except Exception as ex:
                res["errors"].append("SettingsFootnoteText: " + str(ex)[:50])

    # 폴드 그리드 마커(무렌더 Spacer, 중앙 점앵커) — C++이 폭 비율로 스케일 s를 읽는다.
    for mk, rect in [("Set_grid_base", GRID_BASE), ("Set_grid_fold", GRID_FOLD)]:
        w = find(mk)
        if w is None:
            try:
                w = unreal.EditorUtilityLibrary.add_source_widget(bp, unreal.Spacer, mk, "SettingsPanelRoot")
                if w is not None:
                    w.set_visibility(unreal.SlateVisibility.SELF_HIT_TEST_INVISIBLE)
                    res["created"].append(mk)
            except Exception as ex:
                res["errors"].append(mk + ": " + str(ex)[:60]); continue
        if w is not None:
            try:
                set_slot(w, (0.5, 0.5), rect)
            except Exception as ex:
                res["errors"].append(mk + ": slot " + str(ex)[:50])

    # ---- 텍스트 패스: 상하좌우 중앙 (박스 중심점 + autosize + 중앙정렬 → 길이 무관, 벗어남 방지) ----
    STANDALONE_TEXTS = {
        # name: (elemName, regionName|None)  — 박스 = 리전(있으면) 또는 요소 rect
        "SettingsTitleText": ("header", None),
        "Set_row_fps_label": ("row_fps", "라벨_FPS"),
        "QualityRow_Label": ("row_quality", "라벨_그래픽품질"),
        "ScreenShakeRow_Label": ("row_screenshake", "라벨_화면흔들림"),
        "Set_row_effects_label": ("row_effects", "라벨_이펙트"),
        "MasterVolumeRow_Label": ("row_master", "라벨_전체음량"),
        "BGMVolumeRow_Label": ("row_bgm", "라벨_배경음"),
        "SFXVolumeRow_Label": ("row_sfx", "라벨_효과음"),
        "LanguageRow_Label": ("row_language", "라벨_언어"),
    }
    for name, (el, rg) in STANDALONE_TEXTS.items():
        w = find(name)
        if w is None:
            res["missing"].append(name); continue
        try:
            rect = region_abs(el, rg) if rg else elem_abs(el)
            cx_box, cy_box = rect[0] + rect[2] / 2, rect[1] + rect[3] / 2
            # 기존 핀 앵커 유지(타이틀=상단핀, 라벨=중앙핀), 위치=박스 중심, 정렬(0.5,0.5)+autosize
            an = w.slot.get_editor_property("layout_data").get_editor_property("anchors")
            ax, ay = an.get_editor_property("minimum").x, an.get_editor_property("minimum").y
            ld = unreal.AnchorData(
                offsets=unreal.Margin(cx_box - ax * DW, cy_box - ay * DH, 0.0, 0.0),
                anchors=unreal.Anchors(minimum=unreal.Vector2D(ax, ay), maximum=unreal.Vector2D(ax, ay)),
                alignment=unreal.Vector2D(0.5, 0.5))
            w.slot.set_editor_property("layout_data", ld)
            w.slot.set_editor_property("auto_size", True)
            w.set_editor_property("justification", unreal.TextJustify.CENTER)
            res["synced"].append(name + "(text-center)")
        except Exception as ex:
            res["errors"].append(name + ": txt " + str(ex)[:50])

    # 버튼 안 텍스트: ButtonSlot 중앙 정렬 + 패딩 제거 + 중앙 저스티피케이션
    BUTTONS_WITH_TEXT = ["FpsThirtyButton", "FpsSixtyButton", "QualityLowButton", "QualityMidButton",
                         "QualityHighButton", "LanguageKoreanButton", "LanguageEnglishButton",
                         "BackButton", "SaveAndExitButton", "AbandonRunButton", "ResetButton"]
    for bn in BUTTONS_WITH_TEXT:
        b = find(bn)
        if b is None:
            res["missing"].append(bn); continue
        try:
            for ci in range(b.get_children_count()):
                child = b.get_child_at(ci)
                cs = child.slot
                cs.set_editor_property("horizontal_alignment", unreal.HorizontalAlignment.H_ALIGN_CENTER)
                cs.set_editor_property("vertical_alignment", unreal.VerticalAlignment.V_ALIGN_CENTER)
                cs.set_editor_property("padding", unreal.Margin(0, 0, 0, 0))
                if isinstance(child, unreal.TextBlock):
                    child.set_editor_property("justification", unreal.TextJustify.CENTER)
            res["synced"].append(bn + "(btn-text-center)")
        except Exception as ex:
            res["errors"].append(bn + ": btntxt " + str(ex)[:50])

    # ---- 섹션 제목 텍스트 신설(그래픽/음량/게임플레이) — concept 섹션 텍스트 리전 좌표 ----
    SECTION_TITLES = [
        ("Set_sec_graphics_text", "sec_graphics_header", "섹션_그래픽", "그래픽"),
        ("Set_sec_volume_text", "sec_volume_header", "섹션_음량", "음량"),
        ("Set_sec_gameplay_text", "sec_gameplay_header", "섹션_게임플레이", "게임플레이"),
    ]
    for name, el, rg, label in SECTION_TITLES:
        try:
            rect = region_abs(el, rg) or elem_abs(el)
        except Exception:
            rect = elem_abs(el)
        w = find(name)
        if w is None:
            try:
                w = unreal.EditorUtilityLibrary.add_source_widget(bp, unreal.TextBlock, name, "SettingsPanelRoot")
                if w is not None:
                    w.set_text(unreal.Text(label))
                    w.set_color_and_opacity(unreal.SlateColor(unreal.LinearColor(0.92, 0.96, 1.0, 1.0)))
                    font = w.get_editor_property("font"); font.set_editor_property("size", 26)
                    w.set_editor_property("font", font)
                    res["created"].append(name)
            except Exception as ex:
                res["errors"].append(name + ": add " + str(ex)[:50]); continue
        if w is not None:
            try:
                cx_box, cy_box = rect[0] + rect[2] / 2, rect[1] + rect[3] / 2
                ld = unreal.AnchorData(
                    offsets=unreal.Margin(cx_box - CX, cy_box - CY, 0.0, 0.0),
                    anchors=unreal.Anchors(minimum=unreal.Vector2D(0.5, 0.5), maximum=unreal.Vector2D(0.5, 0.5)),
                    alignment=unreal.Vector2D(0.5, 0.5))
                w.slot.set_editor_property("layout_data", ld)
                w.slot.set_editor_property("auto_size", True)
                w.slot.set_editor_property("z_order", 13)   # 배너 아트(z10) 위로 — z0이면 아트에 가려 안 보인다.
                w.set_editor_property("justification", unreal.TextJustify.CENTER)
                w.set_visibility(unreal.SlateVisibility.SELF_HIT_TEST_INVISIBLE)
            except Exception as ex:
                res["errors"].append(name + ": slot " + str(ex)[:50])

    # ---- 패널 프레임 원비율 고정: 텍스처 실측 → 시트 안 contain 배치 (9-slice 철회, 무왜곡) ----
    pf = find("Set_panel_frame")
    if pf is not None:
        try:
            b = pf.get_editor_property("brush")
            b.set_editor_property("draw_as", unreal.SlateBrushDrawType.IMAGE)
            pf.set_editor_property("brush", b)
            t = b.get_editor_property("resource_object")
            tw = float(t.blueprint_get_size_x())
            th = float(t.blueprint_get_size_y())
            s = min(DW / tw, DH / th)
            cw, chh = tw * s, th * s
            set_slot(pf, (0.5, 0.5), [(DW - cw) / 2, (DH - chh) / 2, cw, chh])
            pf.slot.set_editor_property("z_order", 5)
            res["synced"].append("Set_panel_frame(contain %.0fx%.0f)" % (cw, chh))
        except Exception as ex:
            res["errors"].append("panel contain: " + str(ex)[:60])

    # ---- FPS/언어 세그 텍스트를 concept의 텍스트 리전 위치에 맞춤(ButtonSlot 패딩으로 중심 이동) ----
    # (버튼명, 요소, 프레임리전(=칸), 칸idx, 칸수, 텍스트리전, 라벨문자수, 문자폭계수 — 폰트 파생용)
    # 세그 통일: 각 버튼이 자기 단일 프레임을 칸으로 가진다(cnt=1).
    SEG_TEXT_ALIGN = [
        ("FpsThirtyButton", "row_fps", "세그틀_30", 0, 1, "세그텍스트_30", 2, 0.55),
        ("FpsSixtyButton", "row_fps", "세그틀_60", 0, 1, "세그텍스트_60", 2, 0.55),
        ("QualityLowButton", "row_quality", "세그틀_하", 0, 1, "세그텍스트_하", 4, 0.55),
        ("QualityMidButton", "row_quality", "세그틀_중", 0, 1, "세그텍스트_중", 4, 0.55),
        ("QualityHighButton", "row_quality", "세그틀_상", 0, 1, "세그텍스트_상", 4, 0.55),
        ("LanguageKoreanButton", "row_language", "세그틀_한국어", 0, 1, "세그텍스트_한국어", 3, 1.0),
        ("LanguageEnglishButton", "row_language", "세그틀_English", 0, 1, "세그텍스트_English", 7, 0.55),
    ]
    for bn, el, seg_rg, idx, cnt, txt_rg, chars, charw in SEG_TEXT_ALIGN:
        b = find(bn)
        tr = region_abs(el, txt_rg)
        sg = region_abs(el, seg_rg)
        if b is None or tr is None or sg is None:
            res["missing"].append(bn + ":segalign"); continue
        try:
            cell_w = sg[2] / cnt
            cell_x0 = sg[0] + cell_w * idx
            btn_cx = cell_x0 + cell_w / 2
            btn_cy = sg[1] + sg[3] / 2
            tr_cx = tr[0] + tr[2] / 2
            tr_cy = tr[1] + tr[3] / 2
            # 가드: 텍스트 박스 중심이 자기 칸 안에 있을 때만 사용. 벗어나면(세그먼트틀 리전과
            # 텍스트 박스의 기준 불일치) 칸 중앙 폴백 — 아트(필 칸)와 항상 일치하게 유지.
            if cell_x0 <= tr_cx <= cell_x0 + cell_w:
                dx, dy = tr_cx - btn_cx, tr_cy - btn_cy
                res["synced"].append(bn + "(seg-align)")
            else:
                dx, dy = 0.0, 0.0
                res["synced"].append(bn + "(seg-align:칸이탈→중앙폴백)")
            # 폰트 파생: 박스 높이(×0.55)와 폭(문자수×계수) 중 작은 쪽. 에디터 박스가 곧 텍스트 크기의 정본.
            font_h = tr[3] * 0.55
            font_w = tr[2] / (chars * charw) if chars > 0 else font_h
            font_size = int(max(11.0, min(24.0, min(font_h, font_w))))
            for ci in range(b.get_children_count()):
                child = b.get_child_at(ci)
                cs = child.slot
                # 패딩 시프트는 콘텐츠 영역을 줄여 텍스트를 잘라낸다(Low/High 클리핑) —
                # 레이아웃 불변인 렌더 트랜스폼 이동으로 대체.
                cs.set_editor_property("padding", unreal.Margin(0, 0, 0, 0))
                child.set_editor_property("render_transform", unreal.WidgetTransform(
                    translation=unreal.Vector2D(dx, dy), scale=unreal.Vector2D(1, 1),
                    shear=unreal.Vector2D(0, 0), angle=0.0))
                if isinstance(child, unreal.TextBlock):
                    fnt = child.get_editor_property("font")
                    fnt.set_editor_property("size", font_size)
                    child.set_editor_property("font", fnt)
        except Exception as ex:
            res["errors"].append(bn + ": segalign " + str(ex)[:50])

    # ---- 체크박스/슬라이더 시안 텍스처 스타일 ----
    TEXDIR = "/Game/SVN/OutSideAsset/AICreation/UI/Settings/"
    def tex(n):
        p = TEXDIR + n + "." + n
        return unreal.load_asset(p) if unreal.EditorAssetLibrary.does_asset_exist(p) else None

    def brush(t, w_px, h_px):
        b = unreal.SlateBrush()
        b.set_editor_property("resource_object", t)
        try:
            b.set_editor_property("image_size", unreal.DeprecateSlateVector2D(float(w_px), float(h_px)))
        except Exception:
            try:
                b.set_editor_property("image_size", [float(w_px), float(h_px)])
            except Exception:
                pass   # 크기 미설정 시 브러시 기본값 — 위젯 슬롯 크기가 렌더 박스를 지배한다.
        return b

    t_on, t_off = tex("T_set_checkbox_on"), tex("T_set_checkbox_off")
    if t_on and t_off:
        for cbn in ("ScreenShakeCheckBox", "EffectsCheckBox"):
            cb = find(cbn)
            if cb is None:
                continue
            try:
                st = cb.get_editor_property("widget_style")
                for prop, t in [("unchecked_image", t_off), ("unchecked_hovered_image", t_off), ("unchecked_pressed_image", t_off),
                                ("checked_image", t_on), ("checked_hovered_image", t_on), ("checked_pressed_image", t_on)]:
                    st.set_editor_property(prop, brush(t, 52, 52))
                cb.set_editor_property("widget_style", st)
                res["synced"].append(cbn + "(style)")
            except Exception as ex:
                res["errors"].append(cbn + ": style " + str(ex)[:50])
    else:
        res["errors"].append("checkbox tex missing: " + TEXDIR)

    t_track, t_knob = tex("T_set_slider_track"), tex("T_set_slider_knob")
    if t_track and t_knob:
        for sln in ("MasterVolumeSlider", "BGMVolumeSlider", "SFXVolumeSlider"):
            sl = find(sln)
            if sl is None:
                continue
            try:
                st = sl.get_editor_property("widget_style")
                for prop in ("normal_bar_image", "hovered_bar_image"):
                    st.set_editor_property(prop, brush(t_track, 424, 14))
                for prop in ("normal_thumb_image", "hovered_thumb_image"):
                    st.set_editor_property(prop, brush(t_knob, 40, 40))
                st.set_editor_property("bar_thickness", 14.0)
                sl.set_editor_property("widget_style", st)
                res["synced"].append(sln + "(style)")
            except Exception as ex:
                res["errors"].append(sln + ": style " + str(ex)[:50])
    else:
        res["errors"].append("slider tex missing: " + TEXDIR)

    # ---- 슬라이더 값 채움 오버레이: ProgressBar(fill=T_set_slider_fill), C++이 매 틱 슬라이더 값과 동기 ----
    t_fill = tex("T_set_slider_fill")
    if t_fill is not None:
        FILL_BARS = [("Set_slider_fill_master", "row_master", "슬라이더트랙_전체"),
                     ("Set_slider_fill_bgm", "row_bgm", "슬라이더트랙_배경음"),
                     ("Set_slider_fill_sfx", "row_sfx", "슬라이더트랙_효과음")]
        for name, el, rg in FILL_BARS:
            rect = region_abs(el, rg)
            if rect is None:
                res["missing"].append(name); continue
            w = find(name)
            if w is None:
                try:
                    w = unreal.EditorUtilityLibrary.add_source_widget(bp, unreal.ProgressBar, name, "SettingsPanelRoot")
                    if w is not None:
                        w.set_visibility(unreal.SlateVisibility.SELF_HIT_TEST_INVISIBLE)
                        res["created"].append(name)
                except Exception as ex:
                    res["errors"].append(name + ": add " + str(ex)[:50]); continue
            if w is None:
                continue
            try:
                set_slot(w, (0.5, 0.5), rect)
                w.slot.set_editor_property("z_order", 11)   # 플레이트(10) 위, 슬라이더(12) 아래
                st = w.get_editor_property("widget_style")
                st.set_editor_property("fill_image", brush(t_fill, rect[2], rect[3]))
                bg = unreal.SlateBrush()
                try:
                    bg.set_editor_property("draw_as", unreal.SlateBrushDrawType.NO_DRAW_TYPE)
                except Exception:
                    bg.set_editor_property("tint_color", unreal.SlateColor(unreal.LinearColor(0, 0, 0, 0)))
                st.set_editor_property("background_image", bg)
                w.set_editor_property("widget_style", st)
                w.set_editor_property("percent", 0.8)
                res["synced"].append(name + "(fill)")
            except Exception as ex:
                res["errors"].append(name + ": fill " + str(ex)[:50])
    else:
        res["errors"].append("fill tex missing: " + TEXDIR)

    # ---- 세그 통일: 옛 통짜 세그 이미지 Collapsed + 옵션별 단일 프레임(Set_seg1_*) 신설 ----
    for old in ("Set_seg_fps", "Set_seg_quality", "Set_seg_language"):
        w = find(old)
        if w is not None:
            try:
                w.set_visibility(unreal.SlateVisibility.COLLAPSED)
                res["synced"].append(old + "(collapsed)")
            except Exception as ex:
                res["errors"].append(old + ": col " + str(ex)[:40])

    seg1_asset = TEXDIR + "T_set_segment1.T_set_segment1"
    if not unreal.EditorAssetLibrary.does_asset_exist(seg1_asset):
        try:
            task = unreal.AssetImportTask()
            task.filename = "D:/UnrealProjects/P_RD_CombatUI_Assets_nobg/13_Settings/04_SegmentedSelector/segment_1_frame.png"
            task.destination_path = "/Game/SVN/OutSideAsset/AICreation/UI/Settings"
            task.destination_name = "T_set_segment1"
            task.replace_existing = True
            task.automated = True
            task.save = True
            unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
            res["created"].append("T_set_segment1(import)")
        except Exception as ex:
            res["errors"].append("seg1 import: " + str(ex)[:60])

    t_seg1 = tex("T_set_segment1")
    SEG1_FRAMES = [
        ("Set_seg1_fps30", "row_fps", "세그틀_30"), ("Set_seg1_fps60", "row_fps", "세그틀_60"),
        ("Set_seg1_q_low", "row_quality", "세그틀_하"), ("Set_seg1_q_mid", "row_quality", "세그틀_중"),
        ("Set_seg1_q_high", "row_quality", "세그틀_상"),
        ("Set_seg1_lang_ko", "row_language", "세그틀_한국어"), ("Set_seg1_lang_en", "row_language", "세그틀_English"),
    ]
    if t_seg1 is not None:
        for name, el, rg in SEG1_FRAMES:
            rect = region_abs(el, rg)
            if rect is None:
                res["missing"].append(name); continue
            w = find(name)
            if w is None:
                try:
                    w = unreal.EditorUtilityLibrary.add_source_widget(bp, unreal.Image, name, "SettingsModalCanvas")
                    if w is not None:
                        w.set_visibility(unreal.SlateVisibility.SELF_HIT_TEST_INVISIBLE)
                        res["created"].append(name)
                except Exception as ex:
                    res["errors"].append(name + ": add " + str(ex)[:50]); continue
            if w is None:
                continue
            try:
                set_slot(w, (0.5, 0.5), rect)
                w.slot.set_editor_property("z_order", 11)   # 플레이트(10) 위, 버튼/텍스트(12) 아래
                fb = w.get_editor_property("brush")
                fb.set_editor_property("resource_object", t_seg1)
                fb.set_editor_property("draw_as", unreal.SlateBrushDrawType.BOX)
                fb.set_editor_property("margin", unreal.Margin(0.30, 0.30, 0.30, 0.30))   # 라운드 끝 보존(9-slice)
                w.set_editor_property("brush", fb)
                res["synced"].append(name + "(frame)")
            except Exception as ex:
                res["errors"].append(name + ": frame " + str(ex)[:50])
    else:
        res["errors"].append("seg1 tex missing after import")

    if not res["errors"]:
        unreal.BlueprintEditorLibrary.compile_blueprint(bp)
        res["saved"] = unreal.EditorAssetLibrary.save_asset(WPATH)

print("SETTINGS_SYNC|" + json.dumps(res, ensure_ascii=False))
