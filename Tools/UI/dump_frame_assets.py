"""Export every panel/frame-ish UI texture to PNG so their regions can be measured.

프레임은 '있다/없다'가 아니라 '어떤 칸을 강제하는가'가 중요하다. 칸을 재려면
그림이 필요하므로 후보를 한 번에 PNG 로 뽑아 둔다.

Writes: Saved/LegacyAudit/FrameDump/*.png and frame_assets.json
"""

import json
import re
from pathlib import Path

import unreal

OUT_DIR = Path("D:/UnrealProjects/P_RD_develop_20260803/Saved/LegacyAudit/FrameDump")
INDEX_PATH = OUT_DIR.parent / "frame_assets.json"
UI_ROOTS = ["/Game/SVN/OutSideAsset/AICreation/UI", "/Game/UI/Art"]

# 판·틀·배경으로 보이는 이름들. 아이콘/글리프/초상화는 뺀다.
NAME_HINT = re.compile(
    r"(frame|panel|_bg|background|shell|base|scrim|card|plate|tray|strip|row|"
    r"summary|popup|backplate|track|banner|listframe|partyframe|roster)",
    re.IGNORECASE)
SKIP_HINT = re.compile(r"(icon|glyph|symbol|face|portrait|knob|thumb|dot|marker|node)",
                       re.IGNORECASE)

MIN_EDGE = 240
MIN_AREA = 200_000


def size_of(asset):
    try:
        return int(asset.blueprint_get_size_x()), int(asset.blueprint_get_size_y())
    except Exception:  # noqa: BLE001
        return 0, 0


OUT_DIR.mkdir(parents=True, exist_ok=True)
index = []
seen = set()

for root in UI_ROOTS:
    for object_path in unreal.EditorAssetLibrary.list_assets(root, True, False):
        package = str(object_path).split(".", 1)[0]
        if package in seen:
            continue
        seen.add(package)
        leaf = package.rsplit("/", 1)[-1]
        if not NAME_HINT.search(leaf) or SKIP_HINT.search(leaf):
            continue
        asset = unreal.EditorAssetLibrary.load_asset(package)
        if asset is None or "Texture" not in str(asset.get_class().get_name()):
            continue
        width, height = size_of(asset)
        if min(width, height) < MIN_EDGE or width * height < MIN_AREA:
            continue

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

        index.append({
            "asset": package,
            "name": leaf,
            "size": [width, height],
            "aspect": round(width / height, 3) if height else None,
            "png": out.name if ok else None,
            "folder": package.rsplit("/", 2)[-2],
        })

INDEX_PATH.write_text(json.dumps(index, ensure_ascii=False, indent=2), encoding="utf-8")
