"""Check what still references each asset on the removal list.

지우기 전에 반드시 본다
-----------------------
SVN 의 텍스처는 팀이 함께 쓰는 원본이다. 참조가 남은 것을 지우면 그 화면이
빈 칸으로 나오거나(런타임 로드) 쿠킹에서 깨진다. 되돌리기도 번거롭다.

세 곳을 본다.

    자산 참조   에셋 레지스트리가 아는 참조. WBP·머티리얼·데이터에셋이 여기 걸린다
    C++ 소스    ``Source/**`` 에서 이름을 문자열로 적어 둔 곳. 생성자 FObjectFinder 가 대표적이다
    도구 스크립트 ``Tools/**`` 에서 이름을 적어 둔 곳. 빌더가 이걸 보고 굽는다

**자산 참조만 보면 놓친다.** C++ 이 경로 문자열로 들고 있는 것은 레지스트리에
안 잡힌다. 실제로 지도 배경과 설정 부품이 그렇게 물려 있다.

결과는 셋으로 나눈다.

    지워도 됨    아무 데서도 안 씀
    막힘         참조가 남음. 무엇이 쓰는지 같이 적는다
    없음         이미 없는 이름

Run headless:
    UnrealEditor-Cmd.exe <project> -run=pythonscript
        -script="Tools/UI/check_asset_refs.py" -unattended -nop4 -nosplash -nullrhi
"""

import json
from pathlib import Path

import unreal

ROOT = Path("D:/UnrealProjects/P_RD_develop_20260803")
LIST = ROOT / "Tools/UI/remove_candidates.txt"
OUT = ROOT / "Saved/LegacyAudit/asset_refs.txt"
JSON_OUT = ROOT / "Saved/LegacyAudit/asset_refs.json"
DUMP_INDEX = ROOT / "Saved/UIKit/AssetDump/_index.json"

# 이 목록에 든 이름을 여기서 찾으면 "아직 쓰는 것" 이다.
TEXT_ROOTS = [ROOT / "Source", ROOT / "Tools"]
TEXT_SUFFIX = {".cpp", ".h", ".cs", ".py", ".md", ".ini", ".json"}
# 스스로를 참조로 세지 않는다. 지울 목록과 점검 도구는 빼야 한다.
SKIP_FILES = {"remove_candidates.txt", "check_asset_refs.py",
              "asset_refs.txt", "asset_refs.json", "_index.json"}
# 점검용으로 만든 것들. 여기엔 597개 이름이 다 적혀 있어서 그대로 두면
# 모든 후보가 "쓰는 곳 있음" 으로 잡힌다. 실제로 213개 전부 막혔다.
SKIP_DIRS = ("tools/ui/mockups", "saved/")

# 컨셉 시안 WBP. 고르려고 만든 버리는 자산이라 참조로 세지 않는다.
CONCEPT_PREFIX = "/Game/UI/Concepts/"


def text_hits(names, by_package):
    """이름 -> 그 이름을 적어 둔 파일 목록."""
    hits = {name: [] for name in names}
    lowered = {name.lower(): name for name in names}
    for root in TEXT_ROOTS:
        if not root.is_dir():
            continue
        for path in root.rglob("*"):
            if not path.is_file() or path.suffix.lower() not in TEXT_SUFFIX:
                continue
            if path.name in SKIP_FILES:
                continue
            relative = str(path.relative_to(ROOT)).replace("\\", "/").lower()
            if any(relative.startswith(skip) for skip in SKIP_DIRS):
                continue
            try:
                body = path.read_text(encoding="utf-8", errors="ignore").lower()
            except OSError:
                continue
            for lower, name in lowered.items():
                if lower not in body:
                    continue
                # 같은 이름이 두 경로에 있을 수 있다. 실제로
                # T_Inventory_Background_Current 는 /Game/UI/Art 와 /Game/SVN 에
                # 각각 있었고, 코드는 앞의 것을 쓰는데 지울 것은 뒤의 것이었다.
                # 이름만 보면 안 지워도 될 것을 막는다.
                package = by_package.get(name)
                if package is not None:
                    folder = package.rsplit("/", 1)[0].lower()
                    if folder not in body and lower in body:
                        # 이름은 있는데 그 폴더 경로가 없다 -> 다른 사본을 쓰는 중
                        continue
                hits[name].append(str(path.relative_to(ROOT)).replace("\\", "/"))
    return hits


def main():
    wanted = [line.strip() for line in LIST.read_text(encoding="utf-8").splitlines()
              if line.strip()]
    index = json.loads(DUMP_INDEX.read_text(encoding="utf-8"))
    by_name = {entry["name"]: entry["asset"] for entry in index}

    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    hits = text_hits(wanted, by_name)

    free, blocked, missing = [], [], []
    for name in wanted:
        package = by_name.get(name)
        if package is None:
            missing.append(name)
            continue
        # 이미 지운 것은 목록(_index.json)에 남아 있어도 자산이 없다.
        # 예전엔 이걸 "참조 조회 실패" 로 적어 막힘으로 셌다 -- 181개가 그렇게
        # 잡혀 보고가 통째로 노이즈가 됐다.
        if not unreal.EditorAssetLibrary.does_asset_exist(package):
            missing.append(name)
            continue
        referencers = []
        try:
            for ref in registry.get_referencers(
                    package, unreal.AssetRegistryDependencyOptions()):
                text = str(ref)
                if text != package:
                    referencers.append(text)
        except Exception:  # noqa: BLE001
            referencers.append("(참조 조회 실패)")
        # 문서에 이름이 적힌 것은 막힘이 아니다. 지우고 문서를 고치면 된다.
        # 코드와 빌더에 적힌 것은 막힘이다 -- 지우면 그 자리가 빈다.
        # Source 의 C++ 만 진짜 막힘이다. 문서(.md)와 Tools 의 한 번 쓰는 스크립트는
        # 지운 뒤 고치면 되는 것이지, 지우지 못할 이유가 아니다. 이걸 막힘으로 세면
        # 내가 만든 도구가 스스로를 막아 아무것도 못 지운다 -- 실제로 그랬다.
        found = hits.get(name, [])
        code = sorted({ref for ref in found if ref.startswith("Source/")})
        docs = sorted({ref for ref in found if not ref.startswith("Source/")})
        # 컨셉 시안은 고르려고 만든 버리는 WBP 다. 참조로 세면 아무것도 못 지운다.
        real = sorted({ref for ref in referencers if not ref.startswith(CONCEPT_PREFIX)})
        concept = sorted({ref for ref in referencers if ref.startswith(CONCEPT_PREFIX)})
        if real or code:
            blocked.append(dict(name=name, asset=package, assets=real,
                                code=code, docs=docs, concept=concept))
        else:
            free.append(dict(name=name, asset=package, docs=docs, concept=concept))

    lines = [f"# 지울 후보 {len(wanted)}개",
             f"# 지워도 됨 {len(free)} · 막힘 {len(blocked)} · 이미 없음 {len(missing)}",
             ""]
    if blocked:
        lines.append("== 막힘 (아직 쓰는 곳이 있다) ==")
        for entry in blocked:
            lines.append(f"  {entry['name']}")
            for ref in entry["assets"]:
                lines.append(f"      자산  {ref}")
            for ref in entry["code"]:
                lines.append(f"      코드  {ref}")
        lines.append("")
    if missing:
        lines.append("== 이미 없음 ==")
        lines.append("  " + ", ".join(missing))
        lines.append("")
    lines.append("== 지워도 됨 ==")
    for entry in free:
        marks = []
        if entry["concept"]:
            marks.append(f"컨셉 시안 {len(entry['concept'])}곳")
        if entry["docs"]:
            marks.append(f"문서·도구 {len(entry['docs'])}곳")
        note = f"   ({' · '.join(marks)}만 참조 — 지운 뒤 고칠 것)" if marks else ""
        lines.append(f"  {entry['asset']}{note}")

    OUT.write_text("\n".join(lines) + "\n", encoding="utf-8")
    JSON_OUT.write_text(json.dumps(
        dict(free=free, blocked=blocked, missing=missing), ensure_ascii=False, indent=1),
        encoding="utf-8")


main()
