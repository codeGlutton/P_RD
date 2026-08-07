"""Find wiring that silently fails: names C++ looks up but no asset provides.

``GetWidgetFromName`` 은 못 찾아도 조용히 nullptr 을 준다. 관대해서 좋지만
오타나 자산 교체로 끊긴 배선이 눈에 안 띈다.

판정 기준
---------
이름에 붙은 번호는 코드 루프가 자산보다 넉넉히 도는 일이 흔하다(스킬 칸 4개를
도는데 자산엔 3개). 그건 끊김이 아니다. 그래서 **번호를 지운 이름(가족)이 자산에
아예 없을 때만** 끊김으로 본다 -- 오타나 이름 변경은 가족 전체가 사라진다.

Run headless:
    UnrealEditor-Cmd.exe <project> -run=pythonscript
        -script="Tools/UI/audit_widget_bindings.py"
"""

import re
from pathlib import Path

import unreal

PROJECT = Path("D:/UnrealProjects/P_RD_develop_20260803")
OUT = PROJECT / "Saved/LegacyAudit/binding_audit.txt"

# 코드에서 위젯을 찾을 때 쓰는 변수 -> 그 이름이 있어야 할 자산
OWNERS = {
    "self": "/Game/UI/CombatLayouts/WBP_CombatHUD04",
    "mDetailOverlayWidget": "/Game/UI/CombatDetail/WBP_CombatDetailOverlay",
    "mMonsterTabWidget": "/Game/UI/MonsterTab/WBP_MonsterTab_Marchbound",
    # 머리 위 HP 바는 HUD 가 아니라 별도 WBP 다(Bar.mRoot->...).
    "mRoot": "/Game/UI/CombatHUD/UnitHpBar/WBP_CombatUnitHpBar",
}

NAME_RE = re.compile(
    r'(\w+)->GetWidgetFromName\(\s*(?:FName\(\*)?(?:TEXT\("([^"%]+)"\)'
    r'|FString::Printf\(TEXT\("([^"]+)"\)\s*,)')

# 전투 HUD 본체는 GetWidgetFromName 이 아니라 Find<T>(WidgetTree, ...) 헬퍼를 쓴다.
# 이걸 빼면 가장 큰 배선 면(HUD 자신)을 통째로 못 본다.
SELF_RE = re.compile(
    r'Find<[^>]+>\(\s*WidgetTree\s*,\s*(?:FName\(\*)?(?:TEXT\("([^"%]+)"\)'
    r'|FString::Printf\(TEXT\("([^"]+)"\)\s*,)')


def family(name):
    """번호를 지운 이름. MonsterRow_2 · MonsterRow_0 · MonsterRow_%d 는 같은 가족."""
    normalized = re.sub(r"%0?\d*d", "#", name)   # printf 자리표시자 먼저
    return re.sub(r"\d+", "#", normalized)


def widget_names(asset_path):
    blueprint = unreal.EditorAssetLibrary.load_asset(asset_path)
    if blueprint is None:
        return None
    tree = unreal.find_object(None, blueprint.get_path_name() + ":WidgetTree")
    if tree is None:
        return None
    prefix = tree.get_path_name() + "."
    return {
        str(obj.get_name()) for obj in unreal.ObjectIterator()
        if isinstance(obj, unreal.Widget) and str(obj.get_path_name()).startswith(prefix)
    }


wanted = {key: set() for key in OWNERS}
for path in PROJECT.glob("Source/P_RD/UI/Combat/CombatLayoutHUDWidget*.cpp"):
    text = path.read_text(encoding="utf-8", errors="ignore")
    for match in NAME_RE.finditer(text):
        owner, plain, template = match.groups()
        bucket = owner if owner in OWNERS else "self"
        wanted[bucket].add(plain if plain else template)
    for match in SELF_RE.finditer(text):
        plain, template = match.groups()
        wanted["self"].add(plain if plain else template)

lines = []
total_broken = 0
for bucket, asset in OWNERS.items():
    have = widget_names(asset)
    lines.append(f"=== {asset.rsplit('/', 1)[-1]} ===")
    if have is None:
        lines.append("  자산을 못 읽음")
        lines.append("")
        continue
    # 코드가 TEXT("PartyCard") + Suffix 처럼 접두사만 적는 자리가 많다.
    # 정확히 같은 이름 · 같은 가족 · 접두사로 시작하는 이름 중 하나라도 있으면 붙은 것이다.
    def resolved(requested):
        target = family(requested)
        for name in have:
            if name == requested or name.startswith(requested) or family(name) == target:
                return True
        return False

    broken = sorted(name for name in wanted[bucket] if not resolved(name))
    total_broken += len(broken)
    lines.append(f"  코드가 찾는 이름 {len(wanted[bucket])}종 / 자산에 가족이 없는 것 {len(broken)}종")
    for name in broken:
        lines.append(f"    끊김: {name}")
    lines.append("")

lines.append(f"합계 끊김 {total_broken}종")
OUT.write_text("\n".join(lines), encoding="utf-8")
