"""Move the Concept03/04 HUD art from the git content tree into the SVN mount.

This project keeps art in SVN, not git: Content/SVN is a junction to the SVN
working copy, so anything under it is addressed as /Game/SVN/... . The art was
authored under /Game/UI/CombatHUD/Production, which would put ~45MB of textures
into the git history.

Renaming through the editor (rather than moving files on disk) is what makes
the references survive -- the materials point at their textures by package
path, and a file move would leave every one of them dangling.

Reports any dependency that is missing before and after, because a broken
reference shows up as an untextured widget rather than as an error.
"""
import unreal

SOURCE_ROOT = "/Game/UI/CombatHUD/Production"
TARGET_ROOT = "/Game/SVN/OutSideAsset/UI/CombatHUD"

registry = unreal.AssetRegistryHelpers.get_asset_registry()


def scan(path):
    registry.scan_paths_synchronous([path], True)


def assets_under(root):
    return [str(p) for p in unreal.EditorAssetLibrary.list_assets(root, True, False)]


def report_missing(paths, stage):
    """List dependencies that are not loadable. Missing art fails silently."""
    missing = set()
    for path in paths:
        package = path.split(".")[0]
        for dependency in registry.get_dependencies(
                unreal.Name(package),
                unreal.AssetRegistryDependencyOptions(
                    include_soft_package_references=True,
                    include_hard_package_references=True)) or []:
            name = str(dependency)
            if not name.startswith("/Game"):
                continue
            if not unreal.EditorAssetLibrary.does_asset_exist(name):
                missing.add("{}  <-  {}".format(name, package))
    unreal.log("[Art] {}: {} assets, {} missing dependencies".format(
        stage, len(paths), len(missing)))
    for line in sorted(missing):
        unreal.log_warning("[Art]   missing: {}".format(line))
    return missing


scan(SOURCE_ROOT)
source_assets = assets_under(SOURCE_ROOT)
if not source_assets:
    raise RuntimeError("nothing staged under {}".format(SOURCE_ROOT))
report_missing(source_assets, "before move")

if not unreal.EditorAssetLibrary.does_directory_exist(TARGET_ROOT):
    unreal.EditorAssetLibrary.make_directory(TARGET_ROOT)

# Move asset by asset rather than renaming the directory: rename_directory
# stops at the first failure and leaves the rest behind, and a half-moved tree
# is worse than either end state.
moved, failed = [], []
for path in source_assets:
    target = path.replace(SOURCE_ROOT, TARGET_ROOT, 1)
    if unreal.EditorAssetLibrary.does_asset_exist(target):
        unreal.EditorAssetLibrary.delete_asset(target)
    if unreal.EditorAssetLibrary.rename_asset(path, target):
        moved.append(target)
    else:
        failed.append(path)

unreal.log("[Art] moved {} / {}".format(len(moved), len(source_assets)))
if failed:
    raise RuntimeError("could not move:\n  " + "\n  ".join(failed))

unreal.EditorAssetLibrary.save_directory(TARGET_ROOT, False, True)

scan(TARGET_ROOT)
report_missing(assets_under(TARGET_ROOT), "after move")

# The rename leaves redirectors behind in the git tree; that is exactly what we
# were trying to avoid putting there.
redirectors = [a for a in assets_under(SOURCE_ROOT)]
for path in redirectors:
    unreal.EditorAssetLibrary.delete_asset(path)
unreal.EditorAssetLibrary.delete_directory(SOURCE_ROOT)
unreal.log("[Art] cleaned {} leftovers from {}".format(
    len(redirectors), SOURCE_ROOT))

remaining = assets_under(TARGET_ROOT)
unreal.log("[Art] {} assets now under {}".format(len(remaining), TARGET_ROOT))
if len(remaining) != len(source_assets):
    raise RuntimeError("expected {} assets at target, found {}".format(
        len(source_assets), len(remaining)))
unreal.log("[Art] done")
