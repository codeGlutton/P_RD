"""Cheap Android portability/configuration checks; does not replace UE compilation."""
import json
from pathlib import Path
import re
import sys


def main() -> None:
    root = Path(__file__).resolve().parents[2]
    project = json.loads((root / "P_RD.uproject").read_text(encoding="utf-8-sig"))
    assert any(plugin["Name"] == "Effekseer" and plugin.get("Enabled") for plugin in project["Plugins"]), "Effekseer must be enabled"
    plugin_root = root / "Plugins/Effekseer"
    assert (plugin_root / "Effekseer.uplugin").is_file(), "Vendored Effekseer descriptor is missing"
    build = (plugin_root / "Source/Effekseer/Effekseer.Build.cs").read_text(encoding="utf-8-sig")
    assert not re.search(r'PrivateDefinitions\.Add\("__EFFEKSEER_NETWORK_ENABLED__', build), "Optional Effekseer networking requires an explicit supported dependency policy"
    for path in (plugin_root / "Source/Effekseer").rglob("*"):
        if path.suffix in (".cpp", ".h"):
            # Some upstream comments use a legacy encoding; macro syntax is ASCII.
            assert not re.search(rb"^\s*#\s*(?:if|elif)\s+_WIN32\b", path.read_bytes(), re.M), f"Non-portable Windows macro: {path.relative_to(root)}"
    engine = (root / "Config/DefaultEngine.ini").read_text(encoding="utf-8-sig")
    assert not re.search(r"^\s*r\.Streaming\.PoolSize\s*=", engine, re.M), "Project settings must not pin the streaming budget"
    assert re.search(r"^TargetSDKVersion=36\s*$", engine, re.M), "Review the Android target SDK policy before changing it"
    profiles = (root / "Config/DefaultDeviceProfiles.ini").read_text(encoding="utf-8-sig")
    assert not re.search(r"^\s*\+?CVars=r\.Streaming\.PoolSize=", profiles, re.M), "Device profiles must not override the user's texture quality budget"
    scalability = (root / "Config/Android/AndroidScalability.ini").read_text(encoding="utf-8-sig")
    budgets = re.findall(r"\[TextureQuality@([0-3])\]\s+r\.Streaming\.PoolSize=(\d+)", scalability)
    assert len(budgets) == 4 and all(0 < int(budget) <= 1024 for _, budget in budgets), "All Android texture quality tiers need bounded memory budgets"
    assert [int(budget) for _, budget in budgets] == sorted(int(budget) for _, budget in budgets), "Memory budgets must increase with texture quality"
    print("Android source/configuration checks passed. Native compilation and device tests are separate checks.")


if __name__ == "__main__":
    try:
        main()
    except (AssertionError, OSError, ValueError) as error:
        print(f"Android source check failed: {error}", file=sys.stderr)
        sys.exit(1)
