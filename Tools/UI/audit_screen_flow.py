"""화면을 **게임이 도는 순서대로** 세우고, 안 쓰이는 것을 골라낸다.

왜 순서인가
-----------
목록을 이름순으로 늘어놓으면 "이게 어느 화면이지" 를 매번 되짚어야 한다. 게임이
지나가는 순서 -- 인트로 · 타이틀 · 지도 · 전투 · 보상 -- 로 세우면 빠진 화면과
안 쓰이는 화면이 눈에 띈다.

순서는 어디서 오나
------------------
짐작하지 않는다. 게임모드 블루프린트가 ``mHUDClass`` 로 자기 화면의 뿌리 위젯을
물고 있으므로, 그 값을 읽어 시작점으로 삼는다. 거기서 에셋 레지스트리를 타고
**그 위젯이 데려오는 위젯**을 따라 내려간다.

무엇을 골라내나
---------------
    쓰임        어느 게임모드 아래에 달렸나
    떠 있음     WBP 인데 아무도 안 부른다 -- 지울 후보
    코드가 만듦 C++ 이 위젯을 직접 만들어 판이 없다 -- WBP 화 후보

Run headless:
    UnrealEditor-Cmd.exe <project> -run=pythonscript
        -script="Tools/UI/audit_screen_flow.py"
"""

import json
from pathlib import Path

import unreal

OUT = Path("D:/UnrealProjects/P_RD_develop_20260803/Saved/LegacyAudit/screen_flow.json")
REPORT = Path("D:/UnrealProjects/P_RD_develop_20260803/Saved/LegacyAudit/screen_flow.txt")

# 게임이 지나가는 순서. 게임모드 블루프린트 경로와 사람이 읽을 이름.
# 순서만 사람이 정한다 -- 무엇이 그 아래 달리는지는 판에서 읽는다.
STAGES = [
    ("/Game/BP/GameMode/BP_IntroGameMode", "인트로"),
    ("/Game/BP/GameMode/BP_FrontendGameMode", "타이틀 · 지도"),
    ("/Game/BP/GameMode/BP_CombatGameMode", "전투"),
    ("/Game/BP/GameMode/BP_ShopGameMode", "상점"),
    ("/Game/BP/GameMode/BP_TreasureGameMode", "보물"),
]

# C++ 이 경로 문자열로 무는 위젯.
#
# 레지스트리는 에셋끼리의 참조만 안다. C++ 이 ``ConstructorHelpers`` 나
# ``LoadClass`` 로 경로를 적어 두면 그 위젯은 레지스트리상 **아무도 안 부르는
# 것**으로 보인다. 실제로 처음 돌렸을 때 61개가 그렇게 나왔다.
# 이 목록은 Source 를 훑어 만든다.
SOURCE_ROOT = Path("D:/UnrealProjects/P_RD_develop_20260803/Source")

LINES = []


def code_referenced():
    """C++ 이 경로로 물고 있는 /Game/UI/... 패키지들."""
    import re
    found = {}
    for path in SOURCE_ROOT.rglob("*.*"):
        if path.suffix not in (".cpp", ".h"):
            continue
        text = path.read_text(encoding="utf-8", errors="ignore")
        for hit in re.findall(r"/Game/[A-Za-z0-9_/]+", text):
            package = hit.split(".")[0]
            found.setdefault(package, set()).add(path.name)
    return found


def say(text):
    LINES.append(text)
    unreal.log(text)


def widget_assets():
    """/Game 아래 모든 WidgetBlueprint 의 패키지 이름."""
    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    found = registry.get_assets_by_class(
        unreal.TopLevelAssetPath("/Script/UMGEditor", "WidgetBlueprint"), True)
    return sorted({str(a.package_name) for a in found})


def hud_class(blueprint_path):
    """게임모드 블루프린트가 물고 있는 뿌리 위젯. 없으면 None."""
    blueprint = unreal.EditorAssetLibrary.load_asset(blueprint_path)
    if blueprint is None:
        return None
    generated = getattr(blueprint, "generated_class", None)
    generated = generated() if callable(generated) else generated
    if generated is None:
        return None
    default = unreal.get_default_object(generated)
    if default is None:
        return None
    try:
        widget_class = default.get_editor_property("m_hud_class")
    except Exception:  # noqa: BLE001
        try:
            widget_class = default.get_editor_property("mHUDClass")
        except Exception:  # noqa: BLE001
            return None
    if widget_class is None:
        return None
    return unreal.SystemLibrary.get_path_name(widget_class).split(".")[0]


def dependencies(package):
    """이 패키지가 데려오는 패키지들."""
    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    options = unreal.AssetRegistryDependencyOptions(
        include_soft_package_references=True, include_hard_package_references=True,
        include_searchable_names=False, include_soft_management_references=False,
        include_hard_management_references=False)
    return [str(name) for name in (registry.get_dependencies(package, options) or [])]


def referencers(package):
    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    options = unreal.AssetRegistryDependencyOptions(
        include_soft_package_references=True, include_hard_package_references=True,
        include_searchable_names=False, include_soft_management_references=False,
        include_hard_management_references=False)
    return [str(name) for name in (registry.get_referencers(package, options) or [])]


def main():
    every = set(widget_assets())
    from_code = code_referenced()
    say(f"WidgetBlueprint {len(every)}개 · C++ 이 경로로 무는 것 "
        f"{len(set(from_code) & every)}개")

    order, seen = [], set()

    def walk(package, stage, depth):
        """위젯이 데려오는 위젯을 따라 내려간다."""
        if package in seen or package not in every:
            return
        seen.add(package)
        order.append({"package": package, "stage": stage, "depth": depth})
        for child in sorted(dependencies(package)):
            if child in every:
                walk(child, stage, depth + 1)

    for blueprint_path, label in STAGES:
        root = hud_class(blueprint_path)
        say(f"\n[{label}] {blueprint_path.rsplit('/', 1)[-1]}"
            f"  뿌리 위젯: {root.rsplit('/', 1)[-1] if root else '없음(코드가 만듦)'}")
        if root is None:
            continue
        walk(root, label, 0)
        # 그 게임모드 자체가 데려오는 위젯도 그 화면 것이다.
        for child in sorted(dependencies(blueprint_path)):
            if child in every:
                walk(child, label, 0)

    # 어느 화면에도 안 달린 것들. 누가 부르는지 한 번 더 본다.
    leftovers = []
    for package in sorted(every - seen):
        callers = [r for r in referencers(package) if r != package]
        leftovers.append({"package": package, "callers": callers})

    for item in leftovers:
        item["code"] = sorted(from_code.get(item["package"], []))
    floating = [x for x in leftovers if not x["callers"] and not x["code"]]
    linked = [x for x in leftovers if x["callers"] or x["code"]]

    say(f"\n흐름에 달린 것 {len(order)}개")
    say(f"흐름 밖이지만 누가 부르는 것 {len(linked)}개")
    say(f"아무도 안 부르는 것 {len(floating)}개")
    for item in floating:
        say(f"    떠 있음: {item['package']}")
    for item in linked:
        who = [c.rsplit("/", 1)[-1] for c in item["callers"][:2]] + item["code"][:2]
        say(f"    흐름 밖: {item['package'].rsplit('/', 1)[-1]}  <- {', '.join(who)}")

    OUT.parent.mkdir(parents=True, exist_ok=True)
    OUT.write_text(json.dumps(
        {"order": order, "floating": floating, "linked": linked},
        ensure_ascii=False, indent=1), encoding="utf-8")
    REPORT.write_text("\n".join(LINES) + "\n", encoding="utf-8")


main()
