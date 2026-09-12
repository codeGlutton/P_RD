"""Check a fresh Unreal audit against the reviewed binding repair manifest."""
import argparse
from collections import Counter
import json
from pathlib import Path

def verify(audit, manifest):
    assets = {r["path"]: r for r in audit["assets"]}
    for change in manifest["bindings"]:
        actual = assets[change["asset"]]
        assert actual["loaded"], change["asset"]
        for field, expected in change.get("textures", {}).items():
            assert actual[field] and actual[field].split(".", 1)[0] == expected, (change["asset"], field, actual[field], expected)
        for field, expected in change.get("text", {}).items():
            assert actual[field] == expected, (change["asset"], field, actual[field], expected)
    counts = Counter()
    for path, row in assets.items():
        name = path.rsplit("/", 1)[1]
        if name.startswith("DA_TestArtifact_") or "_Spell_" in name and name.startswith("DA_Test") or name in ("DA_TestPlayerUnit", "DA_TestEnemyUnit"):
            continue
        kind = row["class"]
        if kind not in ("StaticArtifactData", "StaticUnitSkillData", "StaticSkillData", "StaticPlayerUnitSpawnData", "StaticEnemyUnitSpawnData"):
            continue
        assert row["loaded"], path
        icon = row.get("mIcon")
        assert icon and "T_nav_skill_icon" not in icon, ("missing or placeholder icon", path, icon)
        if kind.endswith("UnitSpawnData"):
            assert row.get("mPortrait"), ("missing portrait", path)
        counts[kind] += 1
    assert counts["StaticArtifactData"] >= 51, counts
    assert counts["StaticEnemyUnitSpawnData"] >= 25, counts
    assert counts["StaticPlayerUnitSpawnData"] >= 6, counts
    assert counts["StaticUnitSkillData"] >= 216, counts
    assert counts["StaticSkillData"] >= 8, counts
    return {"passed": True, "production_assets_checked": dict(counts), "explicit_bindings_checked": len(manifest["bindings"])}

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("audit", type=Path)
    parser.add_argument("manifest", type=Path)
    args = parser.parse_args()
    print(json.dumps(verify(json.loads(args.audit.read_text(encoding="utf-8")), json.loads(args.manifest.read_text(encoding="utf-8"))), indent=2))
