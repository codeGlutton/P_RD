"""Delete the assets that check_asset_refs.py cleared, after backing them up.

안전 장치
---------
1. **막힘 목록은 손대지 않는다.** check_asset_refs.py 가 "지워도 됨" 으로
   가른 것만 지운다. 그 판단은 자산 참조 + C++ + 빌더 스크립트를 다 본 것이다.
2. **지우기 직전에 다시 확인한다.** 목록을 만든 뒤 무언가 바뀌었을 수 있다.
   지금 참조가 하나라도 있으면 그건 건너뛴다.
3. **먼저 백업한다.** SVN 은 팀이 함께 쓰는 원본이다. .uasset 을 통째로 복사해
   두고 지운다.

SVN 커밋은 하지 않는다. 작업 사본에서만 지우고, 올리는 것은 사람이 정한다.

Run headless:
    UnrealEditor-Cmd.exe <project> -run=pythonscript
        -script="Tools/UI/delete_free_assets.py" -unattended -nop4 -nosplash -nullrhi
"""

import json
import shutil
from pathlib import Path

import unreal

ROOT = Path("D:/UnrealProjects/P_RD_develop_20260803")
REFS = ROOT / "Saved/LegacyAudit/asset_refs.json"
BACKUP = Path("D:/UnrealProjects/SVN_UIAssetBackup_20260804")
OUT = ROOT / "Saved/LegacyAudit/asset_delete.txt"
CONCEPT_PREFIX = "/Game/UI/Concepts/"


def package_file(package):
    """/Game/... 를 디스크 경로로. Content/SVN 은 SVN 작업 사본으로 이어진 정션이다."""
    relative = package[len("/Game/"):]
    return ROOT / "Content" / (relative + ".uasset")


def main():
    data = json.loads(REFS.read_text(encoding="utf-8"))
    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    BACKUP.mkdir(parents=True, exist_ok=True)

    deleted, skipped, failed = [], [], []
    for entry in data["free"]:
        package = entry["asset"]

        # 목록을 만든 뒤 누가 다시 물었을 수도 있다. 지우기 직전에 한 번 더 본다.
        live = []
        try:
            for ref in registry.get_referencers(
                    package, unreal.AssetRegistryDependencyOptions()):
                text = str(ref)
                if text != package and not text.startswith(CONCEPT_PREFIX):
                    live.append(text)
        except Exception:  # noqa: BLE001
            live.append("(참조 조회 실패)")
        if live:
            skipped.append(f"{package}  <- {', '.join(sorted(set(live)))}")
            continue

        source = package_file(package)
        if source.is_file():
            target = BACKUP / source.name
            try:
                shutil.copyfile(source, target)
            except OSError as error:
                failed.append(f"{package}  백업 실패 {error}")
                continue

        if unreal.EditorAssetLibrary.delete_asset(package):
            deleted.append(package)
        else:
            failed.append(f"{package}  지우기 실패")

    lines = [f"# 지움 {len(deleted)} · 건너뜀 {len(skipped)} · 실패 {len(failed)}",
             f"# 백업 {BACKUP}", ""]
    if skipped:
        lines.append("== 건너뜀 (지우려는 순간 참조가 있었다) ==")
        lines += [f"  {row}" for row in skipped]
        lines.append("")
    if failed:
        lines.append("== 실패 ==")
        lines += [f"  {row}" for row in failed]
        lines.append("")
    lines.append("== 지움 ==")
    lines += [f"  {row}" for row in deleted]
    OUT.write_text("\n".join(lines) + "\n", encoding="utf-8")


main()
