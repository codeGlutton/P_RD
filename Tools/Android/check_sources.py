"""Cheap Android portability/configuration checks; does not replace UE compilation."""
from pathlib import Path
import re
import sys


def main() -> None:
    root = Path(__file__).resolve().parents[2]
    engine = (root / "Config/DefaultEngine.ini").read_text(encoding="utf-8-sig")
    assert not re.search(r"^\s*r\.Streaming\.PoolSize\s*=", engine, re.M), "Project settings must not pin the streaming budget"
    assert re.search(r"^TargetSDKVersion=36\s*$", engine, re.M), "Review the Android target SDK policy before changing it"
    assert re.search(r"^bUseExternalFilesDir=True\s*$", engine, re.M), "Android saves must use app-specific storage, not the shared storage root"
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
