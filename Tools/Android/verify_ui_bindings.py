"""Check a fresh Unreal audit against the reviewed binding repair manifest."""
import argparse
from collections import Counter
import json
from pathlib import Path

CURRENT_MONSTER_BINDINGS = {
    # Later monster portraits replace values in the original repair manifest.
    "/Game/BP/DataAsset/Unit/EnemyUnit/Stage2/DA_SkeletonBirdUnit": {
        "mIcon": "/Game/SVN/OutSideAsset/AICreation/UI/Portraits/Variants_20260916/T_Portrait_SkeletonBird_v1",
        "mPortrait": "/Game/SVN/OutSideAsset/AICreation/UI/Portraits/Variants_20260916/T_Portrait_SkeletonBird_v1",
    },
    "/Game/BP/DataAsset/Unit/EnemyUnit/Stage2/DA_Red_SpiderUnit": {
        "mIcon": "/Game/SVN/OutSideAsset/AICreation/UI/Portraits/Variants_20260916/T_Portrait_RedSpider_v1",
        "mPortrait": "/Game/SVN/OutSideAsset/AICreation/UI/Portraits/Variants_20260916/T_Portrait_RedSpider_v1",
    },
    "/Game/BP/DataAsset/Unit/EnemyUnit/Stage2/DA_Slime_ExplosionUnit": {
        "mIcon": "/Game/SVN/OutSideAsset/AICreation/UI/Portraits/Variants_20260916/T_Portrait_SlimeExplosion_v1",
        "mPortrait": "/Game/SVN/OutSideAsset/AICreation/UI/Portraits/Variants_20260916/T_Portrait_SlimeExplosion_v1",
    },
    "/Game/BP/DataAsset/Unit/EnemyUnit/Stage3/DA_GhostUnit": {
        "mIcon": "/Game/SVN/OutSideAsset/AICreation/UI/Portraits/Variants_20260919/T_Portrait_Ghost_v1",
        "mPortrait": "/Game/SVN/OutSideAsset/AICreation/UI/Portraits/Variants_20260919/T_Portrait_Ghost_v1",
        "mShortCut": "/Game/SVN/OutSideAsset/AICreation/UI/MonsterCutIns/T_CutIn_Ghost_v1",
    },
}

def verify(audit, manifest):
    assets = {r["path"]: r for r in audit["assets"]}
    for change in manifest["bindings"]:
        actual = assets[change["asset"]]
        assert actual["loaded"], change["asset"]
        for field, expected in change.get("textures", {}).items():
            expected = CURRENT_MONSTER_BINDINGS.get(change["asset"], {}).get(field, expected)
            assert actual[field] and actual[field].split(".", 1)[0] == expected, (change["asset"], field, actual[field], expected)
        for field, expected in change.get("text", {}).items():
            assert actual[field] == expected, (change["asset"], field, actual[field], expected)
    for path, bindings in CURRENT_MONSTER_BINDINGS.items():
        actual = assets[path]
        assert actual["loaded"], path
        for field, expected in bindings.items():
            assert actual[field] and actual[field].split(".", 1)[0] == expected, (path, field, actual[field], expected)
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
    # 87679c16 removed 11 legacy artifacts; the reviewed production catalog has 40.
    assert counts["StaticArtifactData"] >= 40, counts
    assert counts["StaticEnemyUnitSpawnData"] >= 25, counts
    assert counts["StaticPlayerUnitSpawnData"] >= 6, counts
    assert counts["StaticUnitSkillData"] >= 216, counts
    assert counts["StaticSkillData"] >= 8, counts
    return {"passed": True, "production_assets_checked": dict(counts), "explicit_bindings_checked": len(manifest["bindings"]) + len(CURRENT_MONSTER_BINDINGS)}

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("audit", type=Path)
    parser.add_argument("manifest", type=Path)
    args = parser.parse_args()
    print(json.dumps(verify(json.loads(args.audit.read_text(encoding="utf-8")), json.loads(args.manifest.read_text(encoding="utf-8"))), indent=2))
