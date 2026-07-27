"""Report portrait properties on gameplay unit data assets."""

import json
import os

import unreal


ROOT = "/Game/BP/DataAsset/Unit"
OUTPUT = os.path.join(
    unreal.Paths.project_saved_dir(),
    "UI",
    "unit_portrait_audit.json",
)

rows = []
for object_path in unreal.EditorAssetLibrary.list_assets(ROOT, True, False):
    package = str(object_path).split(".", 1)[0]
    asset = unreal.EditorAssetLibrary.load_asset(package)
    if asset is None:
        continue
    row = {
        "asset": package,
        "class": asset.get_class().get_name(),
        "portrait_property": None,
        "portrait": None,
    }
    for candidate in ("portrait", "m_portrait"):
        try:
            portrait = asset.get_editor_property(candidate)
            row["portrait_property"] = candidate
            row["portrait"] = portrait.get_path_name() if portrait else None
            break
        except Exception:
            pass
    rows.append(row)

os.makedirs(os.path.dirname(OUTPUT), exist_ok=True)
with open(OUTPUT, "w", encoding="utf-8") as handle:
    json.dump(rows, handle, ensure_ascii=False, indent=2)
unreal.log("[PortraitAudit] wrote {} rows to {}".format(len(rows), OUTPUT))
