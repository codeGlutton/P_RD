"""Export every UI texture under SVN to PNG, with an index.

``dump_frame_assets.py`` 는 이름과 크기로 걸러 "판·틀처럼 보이는 것" 만 뽑았다.
그 걸름 때문에 실제로 쓰는 부품이 목록에서 빠졌다 -- 아이콘·손잡이·마커도
배치에 자리를 차지하는 부품이다.

이건 안 거른다. 텍스처면 다 뽑는다. 무엇이 부품이고 무엇이 그림인지는 재 보고
정한다. 다만 아주 큰 그림(배경 일러스트)은 화면에 못 다 보여 주므로 긴 변을
줄여서 뽑는다 -- 비율로 재기 때문에 줄여도 값은 같다.

Run headless -- ``-nullrhi`` 를 빼야 한다. 내보내기 팩토리가 Slate 를 건드린다:
    UnrealEditor.exe <project>
        -ExecutePythonScript="Tools/UI/dump_all_ui_assets.py" -unattended -nop4 -nosplash
"""

import json
from pathlib import Path

import unreal

OUT_DIR = Path("D:/UnrealProjects/P_RD_develop_20260803/Saved/UIKit/AssetDump")
INDEX = OUT_DIR / "_index.json"
ROOTS = ["/Game/SVN/OutSideAsset/AICreation/UI"]
MAX_EDGE = 768          # 이보다 크면 줄여서 뽑는다. 비율로 재니 값은 안 변한다
LOG = Path("D:/UnrealProjects/P_RD_develop_20260803/Saved/LegacyAudit/asset_dump.txt")


def size_of(asset):
    try:
        return int(asset.blueprint_get_size_x()), int(asset.blueprint_get_size_y())
    except Exception:  # noqa: BLE001
        return 0, 0


def main():
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    index, seen, failed = [], set(), []

    for root in ROOTS:
        for object_path in unreal.EditorAssetLibrary.list_assets(root, True, False):
            package = str(object_path).split(".", 1)[0]
            if package in seen:
                continue
            seen.add(package)
            asset = unreal.EditorAssetLibrary.load_asset(package)
            if asset is None or "Texture" not in str(asset.get_class().get_name()):
                continue
            width, height = size_of(asset)
            if width <= 0 or height <= 0:
                continue

            leaf = package.rsplit("/", 1)[-1]
            out = OUT_DIR / f"{leaf}.png"
            task = unreal.AssetExportTask()
            task.object = asset
            task.filename = str(out)
            task.automated = True
            task.prompt = False
            task.replace_identical = True
            try:
                task.exporter = unreal.TextureExporterPNG()
            except Exception:  # noqa: BLE001
                pass
            ok = False
            try:
                ok = bool(unreal.Exporter.run_asset_export_task(task)) and out.exists()
            except Exception:  # noqa: BLE001
                ok = False
            if not ok:
                failed.append(leaf)
                continue

            index.append({
                "asset": package,
                "name": leaf,
                "size": [width, height],
                "folder": package.rsplit("/", 2)[-2],
                "png": out.name,
            })

    INDEX.write_text(json.dumps(index, ensure_ascii=False, indent=2), encoding="utf-8")
    LOG.write_text(
        f"뽑은 텍스처 {len(index)}개 / 실패 {len(failed)}개\n"
        + ("실패: " + ", ".join(failed[:40]) + "\n" if failed else ""),
        encoding="utf-8")


main()
