"""UE Python commandlet: modify ONLY the marked disposable release project copy."""
import json
from pathlib import Path
import sys

import unreal

sys.path.insert(0, str(Path(__file__).resolve().parent))
from ui_policy import TARGETS, android_downscale


def main():
    project = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())).resolve()
    marker = project / ".android-release-copy.json"
    if not marker.is_file():
        raise RuntimeError("UI preparation is allowed only in the release builder's marked project copy")
    metadata = json.loads(marker.read_text(encoding="utf-8"))
    if Path(metadata["copy_root"]).resolve() != project or Path(metadata["source_root"]).resolve() == project:
        raise RuntimeError("Invalid release copy marker")
    rows = []
    for asset_path, maximum in TARGETS.items():
        file_path = (project / "Content" / (asset_path.removeprefix("/Game/") + ".uasset")).resolve()
        if not file_path.is_relative_to(project / "Content") or not file_path.is_file():
            raise RuntimeError("Target UI asset is missing or points outside the disposable project copy")
        texture = unreal.load_asset(asset_path)
        if not isinstance(texture, unreal.Texture2D):
            raise RuntimeError("Release UI target is not a Texture2D: " + asset_path)
        if texture.get_editor_property("mip_gen_settings") != unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS:
            raise RuntimeError("Review release UI policy after changing mip settings: " + asset_path)
        width, height = texture.blueprint_get_size_x(), texture.blueprint_get_size_y()
        setting = texture.get_editor_property("downscale")
        default = float(setting.get_editor_property("default"))
        platforms = {str(key): float(value) for key, value in dict(setting.get_editor_property("per_platform")).items()}
        prior = platforms.get("Android", platforms.get("Mobile", default))
        scale = android_downscale(width, height, maximum, prior)
        platforms["Android"] = scale
        updated = unreal.PerPlatformFloat()
        updated.set_editor_property("default", default)
        updated.set_editor_property("per_platform", platforms)
        texture.modify()
        texture.set_editor_property("downscale", updated)
        if not unreal.EditorAssetLibrary.save_loaded_asset(texture, only_if_is_dirty=False):
            raise RuntimeError("Could not save copied UI asset: " + asset_path)
        rows.append({"asset": asset_path, "editor_dimensions": [width, height], "maximum_cooked_dimension": maximum,
                     "previous_android_downscale": prior, "android_downscale": scale,
                     "actual_cooked_size_verified": False})
    report = project / "Saved/AndroidRelease/ui-preparation.json"
    report.parent.mkdir(parents=True, exist_ok=True)
    report.write_text(json.dumps(rows, indent=2) + "\n", encoding="utf-8")
    unreal.log("Android release UI overrides saved in the disposable project copy; cooked/device sizes still require verification.")


main()
