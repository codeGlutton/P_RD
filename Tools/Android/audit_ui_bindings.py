"""Read actual Unreal data assets and their UI bindings; does not modify assets."""
import json
import os
from pathlib import Path
import unreal

output = Path(os.environ.get("RD_UI_AUDIT_OUTPUT", str(Path(unreal.Paths.project_saved_dir()) / "UIBindingAudit.json")))
registry = unreal.AssetRegistryHelpers.get_asset_registry()
registry.search_all_assets(True)
rows = []
fields = ("mName", "mDisplayName", "mShortCut", "mDescription", "mIcon", "mPortrait", "mJobType", "mRarityType", "mPrice", "mSkillDatas", "mStaticPassiveData", "mStatModifiers", "mViewClass", "mModelClass")

def value_json(value):
    if value is None or isinstance(value, (bool, int, float)):
        return value
    if isinstance(value, unreal.Object):
        return value.get_path_name()
    if isinstance(value, unreal.Array):
        return [value_json(v) for v in value]
    if isinstance(value, unreal.StructBase):
        return value.export_text()
    return str(value)

for data in registry.get_assets_by_path("/Game/BP/DataAsset", recursive=True):
    class_name = str(data.asset_class_path.asset_name)
    if class_name not in ("StaticArtifactData", "StaticUnitSkillData", "StaticSkillData", "StaticPlayerUnitSpawnData", "StaticEnemyUnitSpawnData", "StaticPassiveData", "StaticObstacleSpawnData", "StaticEquipmentData"):
        continue
    asset = data.get_asset()
    row = {"path": str(data.package_name), "class": class_name, "loaded": asset is not None}
    if asset:
        for field in fields:
            try:
                row[field] = value_json(asset.get_editor_property(field))
            except Exception:
                pass
    rows.append(row)

textures = [str(data.package_name) for data in registry.get_assets_by_path("/Game/SVN", recursive=True)
            if str(data.asset_class_path.asset_name) == "Texture2D" and any(s in str(data.package_name).lower() for s in ("portrait", "icon", "artifact"))]
output.parent.mkdir(parents=True, exist_ok=True)
output.write_text(json.dumps({"assets": sorted(rows, key=lambda r: r["path"]), "textures": sorted(textures)}, ensure_ascii=False, indent=2), encoding="utf-8")
unreal.log("UI_BINDING_AUDIT_COMPLETE assets=%d textures=%d output=%s" % (len(rows), len(textures), output))
