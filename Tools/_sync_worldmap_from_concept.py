# WBP_FrontendMap/Node/Line 월드맵 시안 싱크: concept_worldmap_claude02.json(216)을 정본으로.
# hybridSlotSync — 프레임(스크림/양피지/두루마리/패널/버튼/범례/마커)은 WBP 슬롯+브러시로 소유 이전,
# 노드/선 스타일(링/경로 텍스처, 두께)은 WBP 클래스 디폴트(CDO)로 이관한다.
# v2(20260703): concept 갱신 반영 —
#   1) stage_info_panel 철회: Map_StageInfoPanel/Map_StageNameText/Map_StageProgressText 생성 중단 + 1차 실행 산출물 제거
#   2) 범례 = 완성형 이미지 1장(Map_LegendImage, T_wm_legend_full) — 구 행 조립 위젯(Map_LegendPanel/Title/Icon_*/Label_*) 제거
#   3) CloseButton = 클래스 선택과 동일한 기존 UE 에셋(wbp.ueAsset) 직접 로드(임포트 금지), 전 상태 동일 브러시 + disabled 틴트 0.44
#   4) wbpNativeSpec.topUIInset -> 메인 WBP CDO mTopUIInset 주입
# v3(20260704): 범례 PNG(legend_full)가 내용 없는 빈 프레임으로 판명 —
#   Map_LegendImage는 9-slice(BOX, wbp.nineSlice=0.28)로 스트레치, 프레임 리전 기준 6행(아이콘 44px + 라벨) 재조립.
#   제거 목록은 Map_LegendPanel(옛 프레임)만 유지. MapStatusText는 ensure-생성(제거 금지).
# v4(20260704): 사용자 에디터 직접 수정 반영 —
#   1) "JSON에 요소가 없으면 대응 위젯도 제거" 일반 규칙(ELEMENT_WIDGET 매핑). 이번 삭제분:
#      decor_candle/decor_spellbook/selected_room_strip -> Map_DecorCandle/Map_DecorSpellbook/Map_SelectedRoomStrip 제거.
#      생성 테이블은 없는 요소를 조용히 스킵(에러 금지).
#   2) MapStatusText: strip 요소가 있으면 리전 중앙, 없으면 center/bottom 핀 design [960,1024] 중심 폴백(폰트 18). 위젯 자체는 항상 유지.
#   3) 범례: '범례제목' 리전 삭제 -> 위젯 제거, 리전 존재 시에만 생성(데이터 주도).
# v5(20260704): 해석 정정 + HUD 확장 —
#   1) 범례 = innerDesignSize 비례 확대가 정본(v4 raw 해석 철회): 사용자 에디터의 "내부 스케일" 모드 —
#      리전 좌표는 innerDesignSize(300x460) 기준이고 요소 박스(429x599)로 비례 확대된다(region_abs).
#      프레임 = 요소 박스 전체, 행 = region_abs 확대 rect, 아이콘 = 행높이x0.85, 라벨 x = 아이콘+행높이x0.27, 폰트 = 행높이x0.38.
#   2) 탑바 배경판 신설(sync_hud_backdrop): topbar_backdrop 요소 -> 다른 WBP(/Game/UI/Concept02/WBP_Concept02_HUD)의
#      TopBar_Backdrop Image 하나만 소유. 좌우 스트레치 앵커(0,0)-(1,0), 높이 112, 단색(wbp.color), z-50,
#      Collapsed(C++이 켬). 요소 삭제 시 위젯 제거(삭제 동기 규칙 동일). HUD의 다른 위젯 절대 불가침.
# v6(20260704): 양피지 한 장 + 로드 철회 —
#   1) parchment_body = 두루마리 상단+몸통+하단 합성 한 장(worldmap_scroll_full.png, 1797x4197) —
#      브러시 BOX + Margin(0, nineSliceV.top, 0, nineSliceV.bottom): 세로만 9-slice(두루마리 보존, 몸통만 늘어남).
#      기존 세로 타일링(VERTICAL) 철회 — NO_TILE로 리셋(9-slice와 충돌, 재실행 수렴).
#   2) scroll_rod_top/bottom 요소 삭제 -> ELEMENT_WIDGET 매핑 추가로 Map_ScrollRodTop/Bottom 위젯 제거.
#   3) topbar_backdrop 알파 1.0 — 빌더는 wbp.color 배열 알파를 그대로 사용(폴백 기본값도 1.0으로 정렬).
# v7(20260704): 데이터 주도 확대 + 틴트 CDO(새 C++ UPROPERTY — 컴파일 후 실행 전제) —
#   1) status_text 요소 신설 -> MapStatusText 배치를 요소 rect 기준(박스 중심 autosize+중앙, 폰트=높이x0.4)으로.
#      요소 없으면 기존 strip/하드코딩 폴백 체인 유지. ensure-생성 로직 불변.
#   2) LINE CDO에 mOpenTint/mLockedTint(FLinearColor) 주입 — path_layer wbp.classDefaults에서 읽음.
#   3) NODE CDO에 mLockedIconTint(FLinearColor) 주입 — wbpNativeSpec.nodeStyle.lockedIconTint.
#   4) 범례 비율(iconRatio/gapRatio/fontRatio)을 legend_panel wbp.rowStyle에서 읽음(폴백 동일값).
# v8(20260704): 범례 그룹화 — PIE 실측(폭 1110에서 범례가 화면 39% 점유) 대응.
#   1) Map_LegendGroup(CanvasPanel) 신설: 기존 프레임과 동일한 right/center 핀 + 요소 rect(429x599) 슬롯(autosize 아님).
#      C++이 wbpNativeSpec.legendScale(refWidth/minScale)로 그룹 렌더 스케일을 통째 축소(우중앙 피벗).
#   2) 프레임(Map_LegendImage, 그룹 fill)/제목/행 12종을 그룹 안으로 이주 — 그룹-상대 좌표(절대 rect - 요소 원점).
#      루트 낱개 잔재(pre-v8)는 발견 시 제거 후 그룹 안에 재생성(idempotent).
#   3) 메인 WBP CDO에 mLegendRefWidth/mLegendMinScale(float) 주입(C++ UPROPERTY — 컴파일 후 실행 전제).
#   4) 삭제 동기: legend_panel -> Map_LegendGroup(그룹째) + 낱개 잔재 전체.
# v9(20260704): 브러시 image_size 조용한 실패 봉쇄 — 쓰기-되돌리기 + 재읽기 검증 + brushSizes 리포트.
# v10(20260704): 프로브 진상 확정 반영 —
#   사실1) 슬레이트 Box 테두리 픽셀 크기 = "마진 x 텍스처 실제 크기"(ImageSize 무관). 기존 ImageSize가 전부
#     32x32(UMG 기본값)로 남아 있었는데도 테두리가 뻥튀기된 게 그 증거. 범례 버그 전체 메커니즘 =
#     균일 마진 0.28 x (1024,1636) = 코너 287x458 > 위젯 429x599 -> 슬레이트 축소-맞춤 -> 중앙 띠 붕괴.
#   사실2) image_size는 UE5.7 파이썬에서 오직 텍스트 직렬화(export_text/import_text + ImageSize 정규식)로만
#     설정 가능(프로브: DeprecateSlateVector2D 생성자 인자 불가, set_editor_property x/y 무효, to_tuple 항상 빈값).
#     렌더에는 영향 없지만 메타 정확성+관측 목적 — brushSizes의 got이 실값으로 채워진다.
#   변경3) legend_panel.wbp.nineSlice = dict {left,top,right,bottom}(테두리 아트 실측) — margin 인자에 dict 지원.
#     이 마진이면 코너 (153,229)/(161,128)px로 위젯(429x599) 안에 들어가 크레스트가 온전히 나온다.
# v11(20260704): 사용자 UMG 수동 확정 레이아웃 역반영 — 목표: 재실행해도 확정 레이아웃과 1px 내 동일.
#   1) frameOutset 지원: Map_LegendImage 슬롯 = 그룹 fill 대신 stretch 앵커(0,0)-(1,1) +
#      offsets(-L,-T,-R,-B) — 프레임이 그룹 경계 밖으로 outset만큼 확장. frameOutset 없으면 기존 fill 유지.
#   2) legend innerDesignSize=[429,599](요소 크기와 동일=스케일 1) + 행 리전 요소 px 그대로 — region_abs가 항등 처리(확인).
#   3) nineSlice 숫자 0.01(균일) 복귀 — 기존 숫자 마진 경로 그대로. rowStyle fontRatio 0.369 — 데이터로 흐름(확인).
#   4) set_font_size 절사(int) -> 반올림(round): 행 67.7 x 0.369 = 24.98은 사용자 확정 25가 되어야 한다.
# 규칙: 슬롯/브러시/클래스 디폴트만 쓴다. 이벤트/바인딩/그래프 불가침. 재실행 안전(idempotent, v11 상태로 수렴).
# 실행: UnrealEditor-Cmd <uproject> -ExecutePythonScript=<이 파일> (headless)
# 최종 출력: "WORLDMAPSYNC|" + json 리포트 한 줄 (saved 플래그/생성/갱신/제거/임포트 수)
import unreal, json, os, re, shutil

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
TOP_UI_INSET = float(doc.get("wbpNativeSpec", {}).get("topUIInset", 112))
# v8: 범례 그룹 렌더 스케일 메타 — C++이 (디자인 폭 / refWidth)로 축소(상한 1.0, 하한 minScale)
_LEGEND_SCALE = doc.get("wbpNativeSpec", {}).get("legendScale", {})
LEGEND_REF_WIDTH = float(_LEGEND_SCALE.get("refWidth", 1920))
LEGEND_MIN_SCALE = float(_LEGEND_SCALE.get("minScale", 0.5))

res = {"project": "", "saved": {"WBP_FrontendMap": None, "WBP_FrontendMapNode": None, "WBP_FrontendMapLine": None,
                                "WBP_Concept02_HUD": None},
       "created": [], "synced": [], "removed": [], "imported": [], "collapsed": [], "reparented": [],
       "missing": [], "errors": [], "notes": [], "trees": {}, "counts": {},
       "brushSizes": []}   # v9: box(9-slice) 브러시 전수 검증 — {widget, got, want, ok}. 조용한 실패 감시.


def err(scope, msg):
    res["errors"].append(scope + ": " + str(msg)[:120])


# ---------------- concept 좌표 유틸 (레퍼런스 빌더와 동일 계열) ----------------
def elem_abs(el_name):
    return list(map(float, els[el_name]["screenRect"]))


def region_abs(el_name, rg_name_prefix=None, rg_type=None, index=0):
    """요소 내부 리전의 화면 절대 rect(innerDesignSize 스케일 적용). prefix/type로 매칭, index로 n번째 선택.
    요소가 JSON에 없으면 None(에러 금지 — v4 삭제 규칙)."""
    if el_name not in els:
        return None
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
    if el_name not in els:
        return None
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


def load_ue_tex(obj_path):
    """기존 UE 텍스처 에셋 직접 로드(임포트 금지) — '/Game/.../T_x' 패키지 경로."""
    if not obj_path:
        return None
    full = obj_path if "." in obj_path else obj_path + "." + obj_path.rsplit("/", 1)[-1]
    if unreal.EditorAssetLibrary.does_asset_exist(full):
        return unreal.load_asset(full)
    res["missing"].append("ueAsset:" + obj_path)
    return None


# ---------------- 브러시/슬롯 유틸 ----------------
def apply_brush(img, tex, w_px, h_px, draw="image", margin=None):
    """Image 위젯 브러시 설정 — v9 쓰기-되돌리기 + v10 텍스트 직렬화 image_size.
    margin: 숫자(4변 균일) | (L,T,R,B) 튜플 | dict {left,top,right,bottom} (v10 사변 실측).

    범례 버그의 진짜 메커니즘(v10 프로브 확정): 슬레이트 Box의 테두리 픽셀 크기는 ImageSize가 아니라
    "마진 x 텍스처 실제 크기"다 — 기존 ImageSize가 전부 32x32(UMG 기본값)로 남아 있었는데도 테두리가
    뻥튀기된 게 그 증거. 균일 마진 0.28 x (1024,1636) = 코너 287x458 > 위젯 429x599
    -> 슬레이트 축소-맞춤 -> 중앙 띠 붕괴. 해법은 마진 자체(사변 실측 dict)이고,
    image_size는 렌더와 무관하지만 메타 정확성+관측(brushSizes) 목적으로 정확히 기록한다.

    image_size는 UE5.7 파이썬에서 오직 텍스트 직렬화로만 설정 가능(프로브 검증:
    DeprecateSlateVector2D 생성자 인자 불가, set_editor_property x/y 무효, to_tuple 항상 빈값).
    get_editor_property가 주는 것은 구조체 복사본 — 마지막에 반드시 brush를 되쓴다."""
    want_w, want_h = float(w_px), float(h_px)
    b = img.get_editor_property("brush")
    b.set_editor_property("resource_object", tex)
    b.set_editor_property("draw_as",
                          unreal.SlateBrushDrawType.BOX if draw == "box" else unreal.SlateBrushDrawType.IMAGE)
    if margin is not None:
        if isinstance(margin, dict):
            b.set_editor_property("margin", unreal.Margin(float(margin.get("left", 0.0)),
                                                          float(margin.get("top", 0.0)),
                                                          float(margin.get("right", 0.0)),
                                                          float(margin.get("bottom", 0.0))))
        elif isinstance(margin, (tuple, list)):
            b.set_editor_property("margin", unreal.Margin(float(margin[0]), float(margin[1]),
                                                          float(margin[2]), float(margin[3])))
        else:
            b.set_editor_property("margin", unreal.Margin(margin, margin, margin, margin))
    # image_size: 텍스트 직렬화 경로(검증된 유일 레시피)
    try:
        txt = b.export_text()
        size_token = "ImageSize=(X=%.6f,Y=%.6f)" % (want_w, want_h)
        if re.search(r"ImageSize=\([^)]*\)", txt):
            new_txt = re.sub(r"ImageSize=\([^)]*\)", size_token, txt)
        elif txt.startswith("("):
            new_txt = "(" + size_token + "," + txt[1:]   # 기본값 생략 직렬화 대비: 토큰 삽입
        else:
            new_txt = txt
        if new_txt != txt:
            b.import_text(new_txt)
    except Exception:
        pass   # 실패해도 아래 재읽기 검증이 잡아 리포트한다
    img.set_editor_property("brush", b)   # 복사본 수정 후 반드시 되쓴다

    # ---- 되쓴 뒤 재읽기 검증: export_text에서 ImageSize 정규식 추출 — 다시는 조용히 못 실패하게 ----
    got_w, got_h = None, None
    try:
        txt2 = img.get_editor_property("brush").export_text()
        m = re.search(r"ImageSize=\(X=([-+0-9.eE]+),Y=([-+0-9.eE]+)\)", txt2)
        if m:
            got_w, got_h = float(m.group(1)), float(m.group(2))
    except Exception:
        pass
    name = img.get_name()
    ok = (got_w is not None and got_h is not None
          and abs(got_w - want_w) <= 1.0 and abs(got_h - want_h) <= 1.0)
    if draw == "box":
        res["brushSizes"].append({"widget": name, "got": [got_w, got_h],
                                  "want": [want_w, want_h], "ok": ok})
    if not ok:
        res["notes"].append("brush image_size verify failed: %s got=(%s,%s) want=(%.0f,%.0f)"
                            % (name, got_w, got_h, want_w, want_h))


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
    # v11: 절사(int) -> 반올림 — 사용자 확정 레이아웃과 1px 내 동일 목표(예: 67.7행 x 0.369 = 24.98 -> 25)
    f = t.get_editor_property("font")
    f.set_editor_property("size", int(round(max(12.0, min(30.0, float(size))))))
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
            w.set_editor_property("visibility", unreal.SlateVisibility.COLLAPSED)
            res["collapsed"].append(n)
        except Exception as ex:
            err("map", n + " collapse " + str(ex)[:50])


def remove_widgets(bp, names, scope="map"):
    """소스 트리에서 제거. WidgetTree.RemoveWidget은 스크립트 미노출 — 부모 remove_child로 분리하면
    루트 탐색 기반인 find_source_widget_by_name에서 사라져 재실행 시 조용히 스킵된다(idempotent)."""
    for n in names:
        w = find_w(bp, n)
        if w is None:
            continue  # 없으면 조용히 스킵
        try:
            p = w.get_parent()
            if p is not None:
                p.remove_child(w)
                res["removed"].append(n)
            else:
                w.set_visibility(unreal.SlateVisibility.COLLAPSED)
                res["removed"].append(n + "(collapsed:no-parent)")
        except Exception as ex:
            err(scope, n + " remove " + str(ex)[:50])


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


def compile_and_save(bp, asset_path, scope, key, cdo_pairs=None):
    """위젯 편집 -> 컴파일(generated_class 최신화) -> CDO 클래스 디폴트 주입 -> 저장.
    save_asset은 에디터가 열려 있으면 False(조용한 실패) — 반환값을 saved 플래그로 그대로 노출."""
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
    if cdo_pairs:
        cdo = get_cdo(asset_path)
        if cdo is None:
            err(scope, "CDO load failed")
        else:
            set_cdo_props(cdo, scope, cdo_pairs)
        if scope_errors(scope):
            res["saved"][key] = False
            res["notes"].append(key + ": errors present, save skipped")
            return
    ok = bool(unreal.EditorAssetLibrary.save_asset(asset_path, only_if_is_dirty=False))
    res["saved"][key] = ok
    if not ok:
        res["notes"].append(key + ": save_asset returned False (editor open? asset locked?)")


# 텍스트 색: 양피지 위 잉크 톤, 타이틀 골드 톤 (시안 무드 기준의 재량값)
COL_GOLD = (0.95, 0.87, 0.60, 1.0)
COL_INK = (0.30, 0.22, 0.12, 0.85)

# 철회된 v1 산출물 — 찾아서 소스 트리에서 제거(없으면 스킵).
# v3: 범례 행(Map_LegendTitleText/Icon_*/Label_*)은 프레임 리전 기준으로 다시 만들므로 제거 금지 —
#     옛 프레임 Map_LegendPanel만 남긴다. MapStatusText 절대 포함 금지(C++ 필수 바인딩).
V2_REMOVED = ["Map_StageInfoPanel", "Map_StageNameText", "Map_StageProgressText", "Map_LegendPanel"]

# v4 일반 규칙: "JSON에 요소가 없으면 대응 위젯도 제거" — 요소명 -> 빌더 소유 위젯명 매핑.
# CloseButton/EnterRoomButton/MapStatusText는 C++ 필수 바인딩이라 이 규칙에 넣지 않는다(요소가 사라져도 위젯 유지).
ELEMENT_WIDGET = {
    "decor_candle": "Map_DecorCandle",
    "decor_spellbook": "Map_DecorSpellbook",
    "selected_room_strip": "Map_SelectedRoomStrip",
    "hint_scroll": "Map_ScrollHint",
    "map_title": "MapTitleText",
    "legend_panel": "Map_LegendGroup",          # v8: 그룹째 제거(낱개 잔재는 삭제 분기에서 추가 정리)
    "scroll_rod_top": "Map_ScrollRodTop",       # v6: 합성 양피지 한 장으로 통합 — 로드 요소 철회
    "scroll_rod_bottom": "Map_ScrollRodBottom",
}

# v3 범례 행: 리전 '행_<키>' -> (위젯 접미사, 기존 노드 아이콘 에셋 | None=커스텀 휴식 아이콘 임포트)
LEGEND_ROWS = {"전투": ("Battle", "T_MapNode_Monster"), "엘리트": ("Elite", "T_MapNode_Elite"),
               "보스": ("Boss", "T_MapNode_Boss"), "상점": ("Shop", "T_MapNode_Shop"),
               "보물": ("Treasure", "T_MapNode_Treasure"), "휴식": ("Rest", None)}
LEGEND_REST_SRC = ASSETS + "12_WorldMap/05_CustomNodeIcons/node_rest_nobg.png"   # 기존 임포트분(T_wm_node_rest_nobg) 재사용
# v5: 아이콘/간격/폰트는 확대된 행 높이에 비례(고정 px 철회) — 내부 스케일 모드에서 요소 리사이즈를 따라간다.
# v7: 비율은 legend_panel wbp.rowStyle 메타가 정본(데이터 주도) — 폴백은 기존 재량값과 동일.
_ROW_STYLE = (els.get("legend_panel") or {}).get("wbp", {}).get("rowStyle", {})
LEGEND_ICON_RATIO = float(_ROW_STYLE.get("iconRatio", 0.85))   # 아이콘 한 변 = 행 높이 x 비율
LEGEND_GAP_RATIO = float(_ROW_STYLE.get("gapRatio", 0.27))     # 아이콘-라벨 간격 = 행 높이 x 비율
LEGEND_FONT_RATIO = float(_ROW_STYLE.get("fontRatio", 0.38))   # 라벨 폰트 = 행 높이 x 비율
COL_LEGEND_GOLD = (0.96, 0.93, 0.80, 1.0)
COL_WHITE = (1.0, 1.0, 1.0, 1.0)
# 범례 부속 위젯 전체(legend_panel 요소 삭제 시 프레임과 함께 제거 — 고아 방지)
LEGEND_CHILD_WIDGETS = (["Map_LegendTitleText"]
                        + ["Map_LegendIcon_" + s for s, _ in LEGEND_ROWS.values()]
                        + ["Map_LegendLabel_" + s for s, _ in LEGEND_ROWS.values()])


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

    # ---- 철회 위젯 제거 (스테이지 패널 계열 + 옛 범례 프레임 — 없으면 조용히 스킵) ----
    remove_widgets(bp, V2_REMOVED)

    # ---- v4 일반 규칙: JSON에서 삭제된 요소의 대응 위젯 제거 ----
    for el_name, widget_name in ELEMENT_WIDGET.items():
        if el_name not in els:
            remove_widgets(bp, [widget_name])
            if el_name == "legend_panel":
                # v8: 그룹 제거가 내부 부속을 함께 지운다 — 아래는 pre-v8 루트 낱개 잔재 정리용
                remove_widgets(bp, ["Map_LegendImage"] + LEGEND_CHILD_WIDGETS)

    # ---- 텍스처 준비 (JSON imageAsset 정본, T_wm_* 로 임포트) ----
    t_scrim = ensure_import(region_image("bg_scrim"))
    t_parch = ensure_import(region_image("parchment_body"))
    t_rod_top = ensure_import(region_image("scroll_rod_top"))
    t_rod_bot = ensure_import(region_image("scroll_rod_bottom"))
    t_marker = ensure_import(region_image("marker_current"))
    t_glow = ensure_import(region_image("next_select_glow"))
    t_legend = ensure_import(region_image("legend_panel"))     # '범례프레임' 리전 imageAsset(사용자 업로드 PNG)
    t_candle = ensure_import(region_image("decor_candle"))     # 요소 삭제 시 None — 조용히 스킵
    t_book = ensure_import(region_image("decor_spellbook"))
    t_strip = ensure_import(region_image("selected_room_strip"))
    # CloseButton: 기존 UE 에셋 직접 참조(wbp.ueAsset, 임포트 금지) — 클래스 선택 BACK 프레임과 동일
    t_back = load_ue_tex(els.get("btn_close", {}).get("wbp", {}).get("ueAsset"))
    enter_src = region_image("btn_enter")
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

    # ---- Map_ScrollHint: 텍스트 힌트(center/top 핀, 박스중심+autosize) — 요소 없으면 조용히 스킵 ----
    if "hint_scroll" in els:
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

    # ---- 프레임/장식 이미지들: edge-pin + 브러시 — JSON에 없는 요소는 조용히 스킵(위젯 제거는 v4 규칙이 담당) ----
    ART = [
        ("Map_DecorCandle", "decor_candle", t_candle, "image", None),
        ("Map_DecorSpellbook", "decor_spellbook", t_book, "image", None),
        ("Map_SelectedRoomStrip", "selected_room_strip", t_strip, "box", 0.30),
    ]
    for name, el, tex, draw, margin in ART:
        if el not in els:
            continue   # v4: 삭제된 요소 — 에러 금지, 조용히 스킵
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

    # ---- 범례(v8): Map_LegendGroup(CanvasPanel)으로 묶음 — C++이 legendScale 메타로 통째 렌더 스케일 축소 ----
    # 그룹 슬롯 = 기존 프레임과 동일한 right/center 핀 + 요소 rect(429x599, autosize 아님).
    # 내부는 그룹-상대 좌표(절대 rect - 요소 원점): 프레임 fill, 제목/행은 기존 region_abs 산식 그대로.
    # idempotent: 그룹 있으면 내부 갱신, 루트 낱개 잔재(pre-v8)는 in_group()이 제거 후 그룹 안에 재생성.
    if "legend_panel" in els:
        try:
            lg_ax, lg_ay = pin_of("legend_panel")
            frame_rect = elem_abs("legend_panel")            # 그룹 = 요소 박스 전체(429x599)
            gx, gy = frame_rect[0], frame_rect[1]            # 그룹-상대 변환 원점
            group = ensure_widget(bp, unreal.CanvasPanel, "Map_LegendGroup", root_name)
            if group is None:
                raise RuntimeError("Map_LegendGroup create failed")
            pin_slot(group, lg_ax, lg_ay, frame_rect, z=10)
            group.set_visibility(unreal.SlateVisibility.SELF_HIT_TEST_INVISIBLE)
            res["synced"].append("Map_LegendGroup")

            def in_group(cls, name):
                """그룹 소속 보장: 루트 낱개 잔재는 제거 후 그룹 안에 새로 만든다. (widget, 신규여부) 반환."""
                gw = find_w(bp, name)
                if gw is not None and gw.get_parent() != group:
                    remove_widgets(bp, [name])
                    gw = None
                was_new = False
                if gw is None:
                    gw = unreal.EditorUtilityLibrary.add_source_widget(bp, cls, name, "Map_LegendGroup")
                    if gw is not None:
                        res["created"].append(name)
                        was_new = True
                return gw, was_new

            # 프레임: 9-slice(BOX, wbp.nineSlice — 숫자=균일, dict=사변)
            # v11 frameOutset: 사용자가 프레임을 그룹 밖으로 확장 확정 — stretch 앵커(0,0)-(1,1) +
            # offsets(-L,-T,-R,-B)로 그룹 경계 밖 outset만큼 확장. frameOutset 없으면 기존 그룹 fill.
            legend_slice = els["legend_panel"].get("wbp", {}).get("nineSlice", 0.28)
            frame_outset = els["legend_panel"].get("wbp", {}).get("frameOutset")
            w, _ = in_group(unreal.Image, "Map_LegendImage")
            if w is not None:
                if isinstance(frame_outset, dict):
                    o_l = float(frame_outset.get("left", 0.0))
                    o_t = float(frame_outset.get("top", 0.0))
                    o_r = float(frame_outset.get("right", 0.0))
                    o_b = float(frame_outset.get("bottom", 0.0))
                    ld = unreal.AnchorData(
                        offsets=unreal.Margin(-o_l, -o_t, -o_r, -o_b),
                        anchors=unreal.Anchors(minimum=unreal.Vector2D(0.0, 0.0),
                                               maximum=unreal.Vector2D(1.0, 1.0)),
                        alignment=unreal.Vector2D(0.0, 0.0))
                    w.slot.set_editor_property("layout_data", ld)
                    w.slot.set_editor_property("z_order", 0)
                    fw, fh = frame_rect[2] + o_l + o_r, frame_rect[3] + o_t + o_b   # 실제 렌더 크기(메타용)
                else:
                    fill_slot(w, z=0)
                    fw, fh = frame_rect[2], frame_rect[3]
                w.set_visibility(unreal.SlateVisibility.HIT_TEST_INVISIBLE)
                if t_legend is not None:
                    apply_brush(w, t_legend, fw, fh, draw="box", margin=legend_slice)
                res["synced"].append("Map_LegendImage(group%s)" % ("+outset" if isinstance(frame_outset, dict) else ""))

            # 제목: 리전 존재 시에만(데이터 주도) — 그룹-상대 중심
            title_rect = region_abs("legend_panel", rg_name_prefix="범례제목")
            if title_rect is not None:
                tw, tw_new = in_group(unreal.TextBlock, "Map_LegendTitleText")
                if tw is not None:
                    if tw_new:
                        tw.set_text(unreal.Text("범례"))
                    rel = [title_rect[0] - gx, title_rect[1] - gy, title_rect[2], title_rect[3]]
                    text_center_slot(tw, 0.0, 0.0, rel, z=1)
                    style_text(tw, title_rect[3], COL_LEGEND_GOLD)   # 중앙정렬 + 골드
                    tw.set_visibility(unreal.SlateVisibility.HIT_TEST_INVISIBLE)
                    res["synced"].append("Map_LegendTitleText")
            else:
                remove_widgets(bp, ["Map_LegendTitleText"])   # 리전 삭제 -> 위젯 제거

            for r in els["legend_panel"].get("regions", []):
                rname = r.get("name", "")
                if not rname.startswith("행_"):
                    continue
                key_kr = rname.split("_", 1)[1]
                if key_kr not in LEGEND_ROWS:
                    res["notes"].append("legend row unmapped: " + key_kr)
                    continue
                suffix, mapnode = LEGEND_ROWS[key_kr]
                row = region_abs("legend_panel", rg_name_prefix=rname)
                if row is None:
                    res["missing"].append("legend:" + rname)
                    continue
                rel_x, rel_y = row[0] - gx, row[1] - gy      # 그룹-상대(핀 수식은 ax=ay=0으로 항등)
                row_cy = rel_y + row[3] / 2.0
                icon_px = row[3] * LEGEND_ICON_RATIO
                # 아이콘: 행 왼쪽 끝, 정방형, 수직중앙
                t_icon = load_mapnode(mapnode) if mapnode else ensure_import(LEGEND_REST_SRC)
                iw, _ = in_group(unreal.Image, "Map_LegendIcon_" + suffix)
                if iw is not None:
                    pin_slot(iw, 0.0, 0.0, [rel_x, row_cy - icon_px / 2.0, icon_px, icon_px], z=1)
                    iw.set_visibility(unreal.SlateVisibility.HIT_TEST_INVISIBLE)
                    if t_icon is not None:
                        apply_brush(iw, t_icon, icon_px, icon_px)
                    res["synced"].append("Map_LegendIcon_" + suffix)
                # 라벨: 아이콘 오른쪽(간격=행높이x비율), 좌정렬 수직중앙(alignment(0,0.5)+autosize), 흰색
                lw, lw_new = in_group(unreal.TextBlock, "Map_LegendLabel_" + suffix)
                if lw is not None:
                    if lw_new:
                        lw.set_text(unreal.Text(key_kr))
                    label_x = rel_x + icon_px + row[3] * LEGEND_GAP_RATIO
                    ld = unreal.AnchorData(
                        offsets=unreal.Margin(label_x, row_cy, 0.0, 0.0),
                        anchors=unreal.Anchors(minimum=unreal.Vector2D(0.0, 0.0),
                                               maximum=unreal.Vector2D(0.0, 0.0)),
                        alignment=unreal.Vector2D(0.0, 0.5))
                    lw.slot.set_editor_property("layout_data", ld)
                    lw.slot.set_editor_property("auto_size", True)
                    lw.slot.set_editor_property("z_order", 1)
                    lw.set_editor_property("justification", unreal.TextJustify.LEFT)
                    lw.set_color_and_opacity(unreal.SlateColor(unreal.LinearColor(*COL_WHITE)))
                    set_font_size(lw, row[3] * LEGEND_FONT_RATIO)
                    lw.set_visibility(unreal.SlateVisibility.HIT_TEST_INVISIBLE)
                    res["synced"].append("Map_LegendLabel_" + suffix)
        except Exception as ex:
            err("map", "legend group " + str(ex)[:70])

    # ---- MapTitleText: map_title rect 중심, autosize+중앙정렬, 폰트=rect높이*0.55 — 요소 없으면 조용히 스킵 ----
    if "map_title" in els:
        try:
            w = find_w(bp, "MapTitleText")
            if w is None:
                # 기존 WBP에는 MapTitleText가 없을 수 있다(BindWidgetOptional) — 생성해 바인딩을 살린다
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
    # CloseButton: 기존 UE 에셋(t_back) 전 상태 동일, disabled만 틴트 0.44 감쇠. EnterRoomButton: v1 유지.
    BTNS = [("CloseButton", "btn_close", t_back, t_back, t_back, True),
            ("EnterRoomButton", "btn_enter", t_enter_n, t_enter_n, t_enter_p or t_enter_n, False)]
    for bname, el, t_n, t_h, t_p, dim_disabled in BTNS:
        if el not in els:
            continue   # 요소 삭제 시 슬롯/스타일 싱크만 스킵 — 버튼 위젯은 C++ 필수라 제거하지 않는다
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
                st.set_editor_property("hovered", make_brush(t_h, rect[2], rect[3]))
                st.set_editor_property("pressed", make_brush(t_p, rect[2], rect[3]))
                if dim_disabled:
                    db = make_brush(t_n, rect[2], rect[3])
                    db.set_editor_property("tint_color",
                                           unreal.SlateColor(unreal.LinearColor(0.44, 0.44, 0.44, 1.0)))
                    st.set_editor_property("disabled", db)
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

    # ---- MapStatusText: C++ BindWidgetOptional 필수 위젯(승리/로딩/지도 준비 안 됨 상태 문구) ----
    # ensure(없으면 루트 캔버스 직속 TextBlock 생성) — 재부모화 없음, 제거 규칙에 절대 포함 금지.
    # v7: status_text 요소가 정본(박스 중심 autosize+중앙정렬, 폰트=박스 높이x0.4).
    #     요소 없으면 v4 폴백 체인 유지: strip 리전 중앙 -> 하드코딩 [960,1024](폰트 18).
    try:
        w = find_w(bp, "MapStatusText")
        if w is None:
            w = ensure_widget(bp, unreal.TextBlock, "MapStatusText", root_name)
            if w is not None:
                w.set_text(unreal.Text(""))   # 문구는 C++이 채운다 — 비워둔다
        if w is None:
            err("map", "MapStatusText create failed")
        else:
            if "status_text" in els:
                ax, ay = pin_of("status_text")           # center/bottom edge-pin
                rect = elem_abs("status_text")
                text_center_slot(w, ax, ay, rect, z=12)  # 박스 중심 + autosize + alignment(0.5,0.5)
                set_font_size(w, rect[3] * 0.4)          # 폰트 = 박스 높이 x 0.4
                res["synced"].append("MapStatusText(status_text)")
            elif "selected_room_strip" in els:
                ax, ay = pin_of("selected_room_strip")   # center/bottom edge-pin
                rect = region_abs("selected_room_strip", rg_type="text", index=0) or elem_abs("selected_room_strip")
                text_center_slot(w, ax, ay, rect, z=12)  # 리전 중앙 + autosize + alignment(0.5,0.5)
                set_font_size(w, rect[3] * 0.5)          # 폰트 = 리전 높이*0.5
                res["synced"].append("MapStatusText(strip)")
            else:
                text_center_slot(w, 0.5, 1.0, [960.0, 1024.0, 0.0, 0.0], z=12)   # 폴백: 하단 중앙 [960,1024]
                set_font_size(w, 18)
                res["synced"].append("MapStatusText(fallback-bottom)")
            w.set_editor_property("justification", unreal.TextJustify.CENTER)
            w.set_visibility(unreal.SlateVisibility.HIT_TEST_INVISIBLE)
    except Exception as ex:
        err("map", "MapStatusText " + str(ex)[:70])

    # ---- MapGraphCanvas(스크롤 콘텐츠) 안 ----
    # Map_ParchmentBody(v6): 두루마리 상단+몸통+하단 합성 한 장(1797x4197) — 세로만 9-slice.
    # 브러시 BOX + Margin(0, top, 0, bottom): 좌우 마진 0, 상/하 = 두루마리 로드 보존, 몸통만 늘어남.
    # 슬롯은 임의(C++이 매 리프레시 크기 동기), zorder -200.
    try:
        w = ensure_widget(bp, unreal.Image, "Map_ParchmentBody", "MapGraphCanvas")
        if w is not None:
            prect = None
            if "parchment_body" in els:
                prect = region_abs("parchment_body") or elem_abs("parchment_body")
            if prect is None:
                prect = [0.0, 0.0, 1920.0, 812.0]
            nsv = els.get("parchment_body", {}).get("wbp", {}).get("nineSliceV", {})
            m_top = float(nsv.get("top", 0.1227))
            m_bot = float(nsv.get("bottom", 0.1165))
            pin_slot(w, 0.0, 0.0, [0.0, 0.0, prect[2], prect[3]], z=-200)
            w.set_visibility(unreal.SlateVisibility.HIT_TEST_INVISIBLE)
            if t_parch is not None:
                apply_brush(w, t_parch, prect[2], prect[3], draw="box", margin=(0.0, m_top, 0.0, m_bot))
                try:
                    b = w.get_editor_property("brush")
                    b.set_editor_property("tiling", unreal.SlateBrushTileType.NO_TILE)  # v6: 타일링 철회(9-slice와 충돌)
                    w.set_editor_property("brush", b)
                except Exception:
                    pass
            res["synced"].append("Map_ParchmentBody")
    except Exception as ex:
        err("map", "Map_ParchmentBody " + str(ex)[:70])

    # Map_ScrollRodTop/Bottom: 높이=concept rect 높이(84), 폭/위치는 C++ 동기, zorder -190
    for name, el, tex, y0 in [("Map_ScrollRodTop", "scroll_rod_top", t_rod_top, 0.0),
                              ("Map_ScrollRodBottom", "scroll_rod_bottom", t_rod_bot, 728.0)]:
        if el not in els:
            continue   # v4: 요소 삭제 시 조용히 스킵
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
        if "node_area" not in els:
            raise KeyError("node_area element missing in concept")   # 배치 경계 마커는 필수 — 에러로 노출
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
        if el not in els:
            continue   # v4: 요소 삭제 시 조용히 스킵 (Collapsed 마커라 위젯 잔존 무해)
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
    # 컴파일 후 메인 WBP generated_class CDO 주입 (C++ UPROPERTY — 컴파일 후 실행 전제):
    #   mTopUIInset(v2) + mLegendRefWidth/mLegendMinScale(v8, 범례 그룹 렌더 스케일)
    compile_and_save(bp, MAP_PATH, "map", "WBP_FrontendMap",
                     cdo_pairs=[("mTopUIInset", TOP_UI_INSET),
                                ("mLegendRefWidth", LEGEND_REF_WIDTH),
                                ("mLegendMinScale", LEGEND_MIN_SCALE)])


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

    # 위젯 편집 -> 컴파일 -> CDO 클래스 디폴트(링 4종 + 아이콘 5종 + 잠김 틴트) -> 저장
    # v7: 잠긴 방 아이콘 곱색 — wbpNativeSpec.nodeStyle.lockedIconTint (C++ UPROPERTY FLinearColor mLockedIconTint)
    locked_icon_tint = doc.get("wbpNativeSpec", {}).get("nodeStyle", {}).get("lockedIconTint", [0.8, 0.8, 0.8, 1.0])
    compile_and_save(bp, NODE_PATH, "node", "WBP_FrontendMapNode", cdo_pairs=[
        ("mRingNormalTexture", rings.get("normal")),
        ("mRingCurrentTexture", rings.get("current")),
        ("mRingLockedTexture", rings.get("locked")),
        ("mRingClearedTexture", rings.get("cleared")),
        ("mLockedIconTint", unreal.LinearColor(*[float(c) for c in locked_icon_tint])),
    ] + list(icons.items()))


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

    # 위젯 편집 -> 컴파일 -> CDO 클래스 디폴트(경로 텍스처/두께 + v7 열림/잠김 틴트) -> 저장
    # (C++ UPROPERTY FLinearColor mOpenTint/mLockedTint — 컴파일 후 실행 전제)
    open_tint = cd.get("mOpenTint", [1.0, 1.0, 1.0, 0.95])
    locked_tint = cd.get("mLockedTint", [1.0, 1.0, 1.0, 0.55])
    compile_and_save(bp, LINE_PATH, "line", "WBP_FrontendMapLine", cdo_pairs=[
        ("mSolidTexture", t_solid),
        ("mDashedTexture", t_dash),
        ("mLineThickness", thickness),
        ("mOpenTint", unreal.LinearColor(*[float(c) for c in open_tint])),
        ("mLockedTint", unreal.LinearColor(*[float(c) for c in locked_tint])),
    ])


# ==================================================================================
# 4) WBP_Concept02_HUD — 탑바 배경판(TopBar_Backdrop) 하나만 소유 (다른 위젯 절대 불가침)
# ==================================================================================
HUD_PATH_FALLBACK = "/Game/UI/Concept02/WBP_Concept02_HUD"


def sync_hud_backdrop():
    """topbar_backdrop 요소 -> HUD WBP의 TopBar_Backdrop Image 생성/갱신.
    지도가 열려 있는 동안만 HUD가 켜는 탑바 배경판(초기 Collapsed — C++ UCombatTileMapHUDWidget이 표시).
    요소가 JSON에서 사라지면 위젯 제거(삭제 동기 규칙 동일)."""
    el = els.get("topbar_backdrop")
    hud_path = (el or {}).get("wbp", {}).get("targetWidget") or HUD_PATH_FALLBACK
    key = "WBP_Concept02_HUD"
    if not unreal.EditorAssetLibrary.does_asset_exist(hud_path):
        err("hud", "asset missing " + hud_path)
        return
    # 최초 1회 백업 (/Game -> Content 경로 변환)
    uasset = TARGET_PROJECT + "/Content" + hud_path[len("/Game"):] + ".uasset"
    try:
        if os.path.exists(uasset) and not os.path.exists(uasset + ".bak_pre_worldmap_sync"):
            shutil.copy2(uasset, uasset + ".bak_pre_worldmap_sync")
    except Exception as ex:
        res["notes"].append("backup " + key + ": " + str(ex)[:60])

    bp = unreal.EditorAssetLibrary.load_asset(hud_path)
    if bp is None:
        err("hud", "load failed " + hud_path)
        return

    # 요소 삭제 동기: topbar_backdrop이 JSON에서 사라지면 위젯 제거 후 저장(위젯도 없으면 no-op)
    if el is None:
        if find_w(bp, "TopBar_Backdrop") is not None:
            remove_widgets(bp, ["TopBar_Backdrop"], scope="hud")
            compile_and_save(bp, hud_path, "hud", key)
        else:
            res["notes"].append(key + ": topbar_backdrop absent, widget absent — no-op")
        return

    root = find_root(bp, ["RootCanvas", "HUD_Map", "HUD_Settings"])
    canvas = first_canvas(root)
    if canvas is None:
        err("hud", "root canvas not found")
        return
    try:
        w = ensure_widget(bp, unreal.Image, "TopBar_Backdrop", canvas.get_name())
        if w is None:
            err("hud", "TopBar_Backdrop create failed")
        else:
            h = float(el["screenRect"][3])                       # 높이 = 요소 rect 높이(=topUIInset 112)
            z = int((el.get("wbp", {}) or {}).get("zOrder", -50))  # 탑바 아트보다 뒤
            ld = unreal.AnchorData(
                offsets=unreal.Margin(0.0, 0.0, 0.0, h),         # L0 / Top0 / R0 / 높이(h)
                anchors=unreal.Anchors(minimum=unreal.Vector2D(0.0, 0.0), maximum=unreal.Vector2D(1.0, 0.0)),
                alignment=unreal.Vector2D(0.0, 0.0))
            w.slot.set_editor_property("layout_data", ld)
            w.slot.set_editor_property("z_order", z)
            tex_src = region_image("topbar_backdrop")
            if tex_src:
                # 아트 확정 시: 리전 imageAsset 텍스처 임포트 + 흰색 틴트
                t = ensure_import(tex_src)
                if t is not None:
                    apply_brush(w, t, DW, h)
                w.set_editor_property("color_and_opacity", unreal.LinearColor(1.0, 1.0, 1.0, 1.0))
            else:
                # 현재: 텍스처 없이 단색 — 기본 흰 브러시에 위젯 틴트로 색을 입힌다.
                # 알파 포함 wbp.color 배열을 그대로 사용(v6: 알파 1.0 — 하드코딩 없음, 폴백도 1.0).
                color = (el.get("wbp", {}) or {}).get("color", [0.02, 0.035, 0.07, 1.0])
                # 런타임 세터(set_color_and_opacity)는 에셋을 더티로 만들지 않아 저장이 조용히 생략된다 — 프로퍼티 경유가 정본.
                w.set_editor_property("color_and_opacity", unreal.LinearColor(*[float(c) for c in color]))
            # Collapsed — C++이 월드맵 열림과 동기로 켠다(켤 때 HitTestInvisible로 표시하는 것은 C++ 책임)
            w.set_visibility(unreal.SlateVisibility.COLLAPSED)
            res["synced"].append("TopBar_Backdrop")
    except Exception as ex:
        err("hud", "TopBar_Backdrop " + str(ex)[:70])

    compile_and_save(bp, hud_path, "hud", key)


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
    sync_hud_backdrop()


try:
    main()
except Exception as ex:
    import traceback
    err("fatal", str(ex)[:150])
    res["notes"].append(traceback.format_exc()[-400:])

res["counts"] = {"created": len(res["created"]), "synced": len(res["synced"]),
                 "removed": len(res["removed"]), "imported": len(res["imported"]),
                 "collapsed": len(res["collapsed"]), "reparented": len(res["reparented"]),
                 "missing": len(res["missing"]), "errors": len(res["errors"])}
print("WORLDMAPSYNC|" + json.dumps(res, ensure_ascii=False))
