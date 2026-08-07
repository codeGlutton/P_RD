"""카탈로그(assets.json)의 텍스처가 아직 프로젝트에 있는지 훑는다.

카탈로그는 뽑던 날 기준이라 그 뒤 지워진 에셋이 남아 있다. 편집기에서
그런 걸 고르면 게임 반영이 조용히 기본값으로 떨어진다(0806 판때기 건).
없는 이름을 missing_assets.json 에 적어 편집기가 목록에서 빼게 한다.

Run headless:
    UnrealEditor-Cmd.exe <project> -run=pythonscript
        -script="Tools/UI/check_assets.py"
"""

import json
from pathlib import Path

import unreal

ROOT = Path("D:/UnrealProjects/P_RD_develop_20260803")
CATALOG = json.loads(
    (ROOT / "Tools/UI/mockups/assets.json").read_text(encoding="utf-8"))
OUT = ROOT / "Tools/UI/mockups/missing_assets.json"

missing = [entry["name"] for entry in CATALOG
           if unreal.EditorAssetLibrary.does_asset_exist(entry["asset"]) is False]
OUT.write_text(json.dumps(missing, ensure_ascii=False, indent=1),
               encoding="utf-8")
unreal.log(f"카탈로그 {len(CATALOG)}개 중 없는 것 {len(missing)}개: {missing}")
