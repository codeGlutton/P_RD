"""Build 5 layout variants for each of 10 screens (50 WBPs), snapped to frame cells.

바뀐 점 (이전 판의 문제)
------------------------
이전 판은 배경 프레임을 평평한 판으로 보고 절대 좌표로 섹션을 얹었다. 그런데
``T_MT_BaseFrame`` 같은 그림에는 세로 분할선과 제목판이 이미 그려져 있어서,
검사해 보니 섹션 238개 중 236개가 분할선을 밟고 있었다.

여기서는 frame_registry 가 실측한 칸(column/window) 안에서만 배치한다. 화면마다
쓰는 프레임도 FRAME_ASSIGNMENT.md 의 배정을 따른다. 3열 판을 네 화면이 나눠 쓰므로
화면마다 색 틴트를 달리해 어디에 있는지 알 수 있게 했다.

Run headless:
    UnrealEditor-Cmd.exe <project> -run=pythonscript
        -script="Tools/UI/build_screen_variants.py"
"""

import json
import sys
from pathlib import Path

import unreal

sys.path.insert(0, str(Path(__file__).resolve().parent))

from frame_registry import (  # noqa: E402
    FRAMES, SCREEN_FRAME, SCREEN_TINT, SCREEN_TITLES, UIROOT, VARIANTS, window,
)
from variant_lib import MISSING_TEXTURES, THEMES, Painter, cross_pattern, ring_pattern  # noqa: E402
from variant_templates import COLS, WINDOWS  # noqa: E402

AUDIT = Path("D:/UnrealProjects/P_RD_develop_20260803/Saved/LegacyAudit")
RESULT_PATH = AUDIT / "variants_build.txt"
SPEC_PATH = AUDIT / "variants_render_spec.json"
RESULT_LINES, SPEC = [], {}

PACKAGE_ROOT = "/Game/UI/Concepts"
DONOR_ASSET = "/Game/UI/MonsterTab/WBP_MonsterTab_Marchbound"
DONOR_SHELL = (
    ("MonsterTabViewportRoot", "ViewportRoot"), ("MonsterTabWorldDimmer", "WorldDimmer"),
    ("MonsterTabScale", "Scale"), ("MonsterTabDesignSize", "DesignSize"),
    ("MonsterTabCanvas", "Canvas"),
)

P = f"{UIROOT}/Portraits"
SKILL_ICON = f"{UIROOT}/CombatHUD/SkillIcons/T_CombatHUD_SkillIcon_1"

# --- 2026-08-04 에 추가된 28종. 경로는 임포트된 그대로. ---
SKILL_ICONS = f"{UIROOT}/CombatHUD/SkillIcons"
STATUS_ICONS = f"{UIROOT}/CombatHUD/StatusIcons"
ARTIFACTS = f"{UIROOT}/Artifacts"
BADGE = f"{UIROOT}/Common/T_Badge_Grade"
CLASS_SYM = f"{UIROOT}/ClassSelect/T_class_symbol"

ICON = {
    "베기": f"{SKILL_ICONS}/T_SkillIcon_Slash",
    "강타": f"{SKILL_ICONS}/T_SkillIcon_HeavySmash",
    "연속 찌르기": f"{SKILL_ICONS}/T_SkillIcon_MultiThrust",
    "회전베기": f"{SKILL_ICONS}/T_SkillIcon_Whirlwind",
    "도약": f"{SKILL_ICONS}/T_SkillIcon_Leap",
    "포효": f"{SKILL_ICONS}/T_SkillIcon_Roar",
    "야수의 발톱": f"{SKILL_ICONS}/T_SkillIcon_BeastClaw",
    "방벽": f"{SKILL_ICONS}/T_SkillIcon_Barrier",
    "돌진": f"{SKILL_ICONS}/T_SkillIcon_Charge",
    "평타": f"{SKILL_ICONS}/T_SkillIcon_BasicAttack",
}
STATUS = {
    "취약": f"{STATUS_ICONS}/T_Status_Vulnerability",
    "약화": f"{STATUS_ICONS}/T_Status_Weakness",
    "강화": f"{STATUS_ICONS}/T_Status_Fortification",
    "출혈": f"{STATUS_ICONS}/T_Status_Bleed",
    "중독": f"{STATUS_ICONS}/T_Status_Poison",
    "기절": f"{STATUS_ICONS}/T_Status_Stun",
    "은신": f"{STATUS_ICONS}/T_Status_Stealth",
}
GRADE = {"일반": f"{BADGE}_Common", "희귀": f"{BADGE}_Rare", "영웅": f"{BADGE}_Epic"}


SCREENS = {
    "Title": dict(
        name="Rogue the Dice", meta="ver 0.1", art=f"{UIROOT}/Title/T_title_logo", frame=None,
        # 세 번째 자리는 버튼 상태. CONTINUE 는 세이브가 없으면 비활성이라 그걸 보여 준다.
        rows=[("NEW START", "새 런", "normal"), ("CONTINUE", "이어하기", "disabled"),
              ("SETTINGS", "설정", "normal"), ("EXIT", "종료", "pressed")],
        lines=["· 주사위로 굴리는 로그라이크 전술 전투.", "· 용병 셋을 골라 지도를 헤쳐 나간다."],
        rightTitle="시작", bottomTitle="정보", tabs=["시작", "기록", "설정"]),
    "Settings": dict(
        name="설정", meta="그래픽 · 소리 · 게임플레이", art=None, frame=None,
        chips=[("FPS", "60"), ("품질", "높음"), ("흔들림", "켬"), ("효과", "켬")],
        kv=[("언어", "한국어"), ("자동 저장", "켬")],
        rows=[("마스터", "80%"), ("배경음", "70%"), ("효과음", "90%")],
        lines=["· 설정은 즉시 저장된다.", "· 런 포기는 되돌릴 수 없다."],
        leftTitle="게임플레이", rightTitle="소리", bottomTitle="안내", chipTitle="그래픽",
        tabs=["그래픽", "소리", "게임플레이"]),
    "SkillDetail": dict(
        name="베기", meta="액티브 · 근접 · 단일 대상", art=ICON["베기"], frame="skill",
        grade=("희귀 등급", GRADE["희귀"]),
        chips=[("AP", "1"), ("피해", "6~10"), ("쿨타임", "-"), ("타수", "3")],
        grids=[("사거리", cross_pattern(5, 1), 5),
               ("영향 범위", {(2, 2): "caster", (1, 2): "area"}, 5)],
        kv=[("조준 차단", "장애물"), ("영향 차단", "없음")],
        tags=[("취약 2", STATUS["취약"]), ("출혈", STATUS["출혈"]), ("-", None)],
        lines=["· 대상에게 6~10의 피해를 준다.", "· 3타로 나누어 공격한다.",
               "· 마지막 타격에 취약 2를 부여한다."],
        leftTitle="차단 규칙", rightTitle="사거리 · 영향 범위", bottomTitle="효과",
        chipTitle="수치", tabs=["개요", "범위", "효과"]),
    "EnemyDetail": dict(
        name="Mushroom", meta="Lv.1 · 적 · 버섯류",
        art=f"{P}/KK_Face_Enemy_Mushroom_ActionV3", bars=[("50 / 50", 1.0)],
        chips=[("AP", "0/2"), ("속도", "5"), ("방어", "0"), ("예상 피해", "6~10")],
        rows=[("야수의 발톱", "AP 1", "피해 6~10", ICON["야수의 발톱"]),
              ("도약", "AP 2", "이동 3칸", ICON["도약"]),
              ("포효", "AP 1", "약화 2턴", ICON["포효"]),
              ("사냥 본능", "패시브", "체력 50% 이하 +2", None)],
        grids=[("위협 범위", ring_pattern(7, 4, 2), 7)],
        kv=[("이동 범위", "3칸"), ("공격 범위", "2칸")],
        tags=[("취약 2", STATUS["취약"]), ("중독", STATUS["중독"]), ("-", None)],
        lines=["· 패시브 없음.", "· 테두리 = 이동 범위, 채움 = 공격 범위."],
        leftTitle="상태", rightTitle="스킬", bottomTitle="위협", chipTitle="수치",
        tabs=["개요", "스킬", "위협"]),
    "ArtifactDetail": dict(
        name="이빨 부적", meta="전투 시작 시 발동", art=f"{ARTIFACTS}/T_Artifact_FangAmulet",
        grade=("희귀 등급", GRADE["희귀"]),
        frame=f"{UIROOT}/Reward/T_Reward_ArtifactCard_V2",
        chips=[("등급", "희귀"), ("발동", "전투 시작"), ("중첩", "가능"), ("소모", "없음")],
        kv=[("획득처", "보물방"), ("판매가", "45 골드")],
        tags=[("가시 문장", f"{ARTIFACTS}/T_Artifact_ThornCrest"),
              ("여행자의 지도", f"{ARTIFACTS}/T_Artifact_TravelersMap"),
              ("피의 성배", f"{ARTIFACTS}/T_Artifact_BloodChalice"),
              ("낡은 방패", f"{ARTIFACTS}/T_Artifact_WornShieldOrnament")],
        rows=[("행운의 동전", "일반", "골드 +10%", f"{ARTIFACTS}/T_Artifact_LuckyCoin"),
              ("이빨 부적", "희귀", "방어 +3", f"{ARTIFACTS}/T_Artifact_FangAmulet"),
              ("낡은 방패 장식", "일반", "방어 +1", f"{ARTIFACTS}/T_Artifact_WornShieldOrnament")],
        lines=["· 전투 시작 시 방어 3을 얻는다.", "· 피해를 받으면 다음 공격이 +2 강해진다.",
               "· 늑대 이빨을 엮어 만든 부적."],
        leftTitle="획득", rightTitle="보유 아티팩트", bottomTitle="효과", chipTitle="속성",
        tabs=["개요", "효과", "획득"]),
    "EnemySummary": dict(
        name="Mushroom", meta="Lv.1 · 적", art=f"{P}/KK_Face_Enemy_Mushroom_HeadV2",
        bars=[("50 / 50", 1.0)],
        chips=[("AP", "0/2"), ("속도", "5"), ("방어", "0")],
        grids=[("위협 범위", ring_pattern(5, 3, 2), 5)],
        kv=[("예상 피해", "6~10")],
        tags=[("취약 2", STATUS["취약"]), ("기절", STATUS["기절"])],
        lines=["· 테두리 = 이동, 채움 = 공격."],
        leftTitle="상태", rightTitle="위협 범위", bottomTitle="예상 행동", chipTitle="수치",
        tabs=["요약", "스킬", "위협"]),
    "MercenarySummary": dict(
        name="기사", meta="근접 · 방패 특화", art=f"{P}/KK_Face_Knight_HeadV2",
        bars=[("100 / 100", 1.0)],
        chips=[("HP", "100"), ("AP", "10"), ("속도", "5")],
        rows=[("평타", "AP 1", "피해 4~6", ICON["평타"]),
              ("연속 찌르기", "AP 2", "피해 10~18", ICON["연속 찌르기"]),
              ("베기", "AP 1", "피해 6~10", ICON["베기"]),
              ("강타", "AP 2", "피해 6~10", ICON["강타"])],
        kv=[("장비", "낡은 검")],
        tags=[("강화 1턴", STATUS["강화"]), ("은신", STATUS["은신"])],
        lines=["· 방패로 앞줄을 버틴다."],
        leftTitle="상태", rightTitle="스킬", bottomTitle="특성", chipTitle="수치",
        tabs=["요약", "스킬", "장비"]),
    "MercenaryTab": dict(
        name="기사", meta="근접 · 방패 특화", art=f"{P}/KK_Face_Knight_ActionV3",
        grade=("영웅 등급", GRADE["영웅"]),
        bars=[("100 / 100", 1.0)],
        chips=[("HP", "100"), ("AP", "10"), ("속도", "5"), ("방어", "2")],
        # 기존 기사/마법사/도적 심볼은 정사각이 아니라 배너라 규격이 달라 넣지 않는다.
        list=[("기사", "근접", None), ("마법사", "마법", None),
              ("레인저", "원거리", f"{CLASS_SYM}_ranger_v2"),
              ("야만전사", "근접", f"{CLASS_SYM}_barbarian_v2"),
              ("드루이드", "지원", f"{CLASS_SYM}_druid_v2")],
        rows=[("베기", "AP 1", "피해 6~10", ICON["베기"]),
              ("강타", "AP 2", "피해 6~10", ICON["강타"]),
              ("방벽", "AP 2", "방어 +5", ICON["방벽"]),
              ("돌진", "AP 2", "이동 후 타격", ICON["돌진"]),
              ("회전베기", "AP 3", "피해 8~12", ICON["회전베기"])],
        kv=[("골드", "100"), ("파티", "1 / 3")],
        tags=[("강화 1턴", STATUS["강화"]), ("-", None)],
        lines=["· 파티는 최대 셋까지 데려간다."],
        leftTitle="보유 용병", rightTitle="스킬", bottomTitle="파티", chipTitle="수치",
        tabs=["용병", "장비", "유물"]),
    "MonsterTab": dict(
        name="Slime", meta="Lv.1 · 점액류", art=f"{P}/KK_Face_Enemy_Slime_ActionV3",
        grade=("일반 등급", GRADE["일반"]),
        bars=[("50 / 50", 1.0)],
        chips=[("HP", "50"), ("AP", "0"), ("속도", "5"), ("방어", "0")],
        list=[("Slime", "점액류"), ("Mushroom", "버섯류"), ("Spider", "절지류")],
        rows=[("야수의 발톱", "AP 1", "피해 6~10", ICON["야수의 발톱"]),
              ("도약", "AP 2", "이동 3칸", ICON["도약"]),
              ("포효", "AP 1", "약화 2턴", ICON["포효"])],
        grids=[("위협 범위", ring_pattern(5, 3, 2), 5)],
        kv=[("출현", "1층 숲길")],
        tags=[("취약 2", STATUS["취약"]), ("출혈", STATUS["출혈"])],
        lines=["· 이 방에 나오는 적 목록이다."],
        leftTitle="출현 몬스터", rightTitle="스킬", bottomTitle="위협", chipTitle="수치",
        tabs=["몬스터", "위협", "기록"]),
    "Map": dict(
        name="1층 · 숲길", meta="다음 방을 고르세요", art=None, frame=None,
        chips=[("층", "1"), ("방", "3/12"), ("골드", "100"), ("HP", "100")],
        rows=[("전투", "일반", "몬스터 3"), ("보물", "희귀", "아티팩트 1"),
              ("상점", "-", "골드 소모"), ("정예", "위험", "보상 상승")],
        kv=[("현재 위치", "3번 방"), ("남은 방", "9")],
        lines=["· 굵은 선은 갈 수 있는 길.", "· 자물쇠는 아직 못 간다."],
        leftTitle="범례", rightTitle="경로", bottomTitle="현재 런", chipTitle="런 정보",
        tabs=["지도", "범례", "기록"]),
}


SHELL_SUFFIXES = tuple(suffix for _, suffix in DONOR_SHELL)


def trash_leftovers(blueprint, prefix):
    """떼어낸 옛 위젯의 이름을 치운다.

    ``clear_children()`` 은 자식을 슬롯에서 뗄 뿐 객체를 지우지 않는다. 그래서
    다시 빌드할 때 같은 이름으로 '다른 종류'(Border -> Image)를 만들면 엔진이
    이름 충돌로 죽는다. 지우지 못하는 상황을 대비한 안전망이다.
    """
    tree_prefix = blueprint.get_path_name() + ":WidgetTree."
    index = 0
    for obj in unreal.ObjectIterator():
        if not isinstance(obj, unreal.Widget):
            continue
        path = str(obj.get_path_name())
        if not path.startswith(tree_prefix):
            continue
        name = str(obj.get_name())
        if name.startswith(prefix) and name.endswith(SHELL_SUFFIXES):
            continue
        obj.modify()
        obj.rename(f"Retired_{index}")
        index += 1


def prepare(asset_name, folder, prefix):
    package = f"{PACKAGE_ROOT}/{folder}"
    asset_path = f"{package}/{asset_name}"
    if unreal.EditorAssetLibrary.does_asset_exist(asset_path):
        # 옛 위젯이 남은 트리에 다시 그리면 이름 충돌로 엔진이 죽는다. 지우고 새로 뜬다.
        if not unreal.EditorAssetLibrary.delete_asset(asset_path):
            existing = unreal.EditorAssetLibrary.load_asset(asset_path)
            if existing is not None:
                trash_leftovers(existing, prefix)
    if not unreal.EditorAssetLibrary.does_asset_exist(asset_path):
        if not unreal.EditorAssetLibrary.duplicate_asset(DONOR_ASSET, asset_path):
            raise RuntimeError(f"duplicate failed -> {asset_path}")
    blueprint = unreal.EditorAssetLibrary.load_asset(asset_path)
    blueprint.modify()
    tree = unreal.find_object(None, blueprint.get_path_name() + ":WidgetTree")
    tree.modify()
    tree_prefix = tree.get_path_name() + "."
    canvas = None
    for donor_name, suffix in DONOR_SHELL:
        widget = unreal.find_object(None, tree_prefix + prefix + suffix)
        if widget is None:
            widget = unreal.find_object(None, tree_prefix + donor_name)
            if widget is None:
                raise RuntimeError(f"shell widget missing: {prefix + suffix}")
            widget.modify()
            widget.rename(prefix + suffix)
        else:
            widget.modify()
        if isinstance(widget, unreal.CanvasPanel):
            canvas = widget
    canvas.clear_children()
    return blueprint, tree, canvas


def dim(blueprint, prefix, amount):
    widget = unreal.find_object(
        None, blueprint.get_path_name() + ":WidgetTree." + prefix + "WorldDimmer")
    if widget is not None:
        widget.modify()
        widget.set_brush_color(unreal.LinearColor(0.015, 0.012, 0.02, amount))


def draw_title_screen(paint, content, frame):
    """타이틀은 그림의 하단 영역에만 손댄다. 위쪽은 배경 아트라 가리면 안 된다."""
    box = window(frame)
    paint.image("Logo", content.get("art"), (660.0, 120.0), (600.0, 300.0), 12)
    entries = content.get("rows", [])
    height = min(92.0, (box[3] - 16.0 * (len(entries) - 1)) / max(1, len(entries)))
    for index, entry in enumerate(entries):
        label = entry[0]
        state = entry[2] if len(entry) > 2 else "normal"
        y = box[1] + (height + 16.0) * index
        paint.image("MenuPlate", f"{UIROOT}/Title/T_menu_button_frame_{state}",
                    (box[0], y), (box[2] * 0.42, height), 10)
        # 비활성 항목은 글자도 흐리게. 판만 바꾸면 눌리는 줄 안다.
        color = paint.theme["sub"] if state == "disabled" else None
        paint.text("MenuText", label, 34, (box[0], y + height * 0.26),
                   (box[2] * 0.42, height * 0.5), 12, color)
        if state != "disabled":
            paint.button("MenuButton", (box[0], y), (box[2] * 0.42, height), 30)
    paint.image("VersionPlate", f"{UIROOT}/Title/T_version_plate",
                (box[0] + box[2] - 300.0, box[1] + box[3] - 90.0), (300.0, 90.0), 10)
    paint.text("VersionText", content.get("meta", ""), 28,
               (box[0] + box[2] - 300.0, box[1] + box[3] - 68.0), (300.0, 44.0), 12)


def draw_map_screen(paint, content, frame, variant):
    """세로 스크롤 지도. 배경이 0.56 세로형이라 가로로 늘리지 않고 띠로 세운다."""
    box = window(frame)
    strip_height = box[3]
    strip_width = strip_height * frame["stripAspect"]
    strip_x = box[0] + (box[2] - strip_width) / 2.0
    paint.image("MapStrip", frame["strip"], (strip_x, box[1]), (strip_width, strip_height), 6)

    nodes = ["Monster", "Treasure", "Shop", "Elite", "Boss"]
    for index, kind in enumerate(nodes):
        node_y = box[1] + strip_height - 90.0 - (strip_height - 180.0) / (len(nodes) - 1) * index
        offset = (-1 if index % 2 else 1) * strip_width * 0.18
        paint.image("MapNode", f"{UIROOT}/RunFlow/T_MapNode_{kind}_V2",
                    (strip_x + strip_width / 2 - 46.0 + offset, node_y), (92.0, 92.0), 12)
    paint.image("MapMarker", f"{UIROOT}/WorldMap/T_wm_marker_current",
                (strip_x + strip_width / 2 - 34.0, box[1] + strip_height - 150.0),
                (68.0, 68.0), 14)

    left = (box[0], box[1], (strip_x - box[0]) - 24.0, box[3])
    right = (strip_x + strip_width + 24.0, box[1],
             box[0] + box[2] - (strip_x + strip_width) - 24.0, box[3])
    for rect, title, kind in ((left, content["leftTitle"], "rows"),
                              (right, content["bottomTitle"], "chips")):
        if rect[2] < 140.0:
            continue
        paint.section(title, rect)
        inner = (rect[0] + 18.0, rect[1] + 58.0, rect[2] - 36.0, rect[3] - 76.0)
        if kind == "rows":
            items = content["rows"][:4]
            paint.rows(items, (inner[0], inner[1]), inner[2],
                       min(78.0, (inner[3] - 30.0) / len(items)), 10.0)
        else:
            paint.chips(content["chips"][:4], (inner[0], inner[1]),
                        min(110.0, inner[2] / 2 - 10.0), 12.0, columns=2)
            paint.kv(content["kv"], (inner[0], inner[1] + 280.0), inner[2], 46.0, 24)
            paint.lines(content["lines"], (inner[0], inner[1] + 400.0), inner[2], 42.0, 22)


built, blocked = 0, 0
try:
    for screen, content in SCREENS.items():
        frame_key = SCREEN_FRAME[screen]
        frame = FRAMES[frame_key]
        tint = SCREEN_TINT.get(screen)
        for index, (code, label) in enumerate(VARIANTS):
            asset_name = f"WBP_{screen}_{code}"
            prefix = f"{screen}{code}"
            blueprint, tree, canvas = prepare(asset_name, screen, prefix)

            theme_key = ["safe", "shift", "night", "minimal", "lab"][index]
            theme = dict(THEMES[theme_key])
            if tint and theme_key not in ("night",):
                theme["accent"] = unreal.LinearColor(tint[0], tint[1], tint[2], 1.0)
            dim(blueprint, prefix, 0.66 if theme_key == "minimal" else 0.45)

            spec = []
            paint = Painter(tree, canvas, theme, spec)
            if theme_key == "minimal":
                paint.well("BaseFlat", (0.0, 0.0), (1920.0, 1080.0), 0,
                           unreal.LinearColor(0.055, 0.058, 0.07, 0.96))
            else:
                paint.image("BaseFrame", frame["texture"], (0.0, 0.0), (1920.0, 1080.0), 0)
                if tint:
                    # 3열 판을 네 화면이 나눠 쓴다. 옅은 색 물을 들여 어느 화면인지 알린다.
                    paint.well("ScreenTint", (0.0, 0.0), (1920.0, 1080.0), 1,
                               unreal.LinearColor(tint[0], tint[1], tint[2],
                                                  0.16 if theme_key != "lab" else 0.24))
            paint.header(SCREEN_TITLES[screen], None if theme_key == "minimal" else frame)

            payload = dict(content, title=SCREEN_TITLES[screen])
            if screen == "Title":
                draw_title_screen(paint, payload, frame)
            elif screen == "Map":
                draw_map_screen(paint, payload, frame, index)
            elif frame["kind"] == "cols" and theme_key != "minimal":
                COLS[index](paint, payload, frame)
            elif frame["kind"] == "cols":
                COLS[3](paint, payload, frame)
            else:
                WINDOWS[index](paint, payload, frame)

            SPEC[asset_name] = spec
            unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
            if unreal.EditorAssetLibrary.save_loaded_asset(blueprint, False):
                built += 1
            else:
                blocked += 1
                RESULT_LINES.append(f"SAVE BLOCKED {asset_name}")
    RESULT_LINES.append(f"built={built} blocked={blocked}")
    if MISSING_TEXTURES:
        RESULT_LINES.append("ASSET GAP:")
        RESULT_LINES.extend("  " + path for path in sorted(MISSING_TEXTURES))
except Exception as error:  # noqa: BLE001
    import traceback
    RESULT_LINES.append("FAILED: %s" % error)
    RESULT_LINES.append(traceback.format_exc())
finally:
    SPEC_PATH.write_text(json.dumps(SPEC, ensure_ascii=False, indent=2), encoding="utf-8")
    RESULT_PATH.write_text("\n".join(RESULT_LINES), encoding="utf-8")
