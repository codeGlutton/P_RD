# UI icon restoration (2026-09-12)

The September 9 repair existed only locally. Its 60 textures are now in SVN r434.
This manifest restores 66 data assets using existing textures: 40 artifact icons,
12 enemy portraits, 8 gimmick icons, and 6 corrections to existing image references.
Only mIcon/mPortrait properties are changed. Four old skill-name changes were excluded.

Update SVN before using Tools/Android/restore_ui_icon_bindings.py in an Unreal
Python commandlet. It preserves current gameplay data rather than replacing data assets.
Tools/Android/audit_ui_bindings.py reads real assets; verify_ui_bindings.py checks the
manifest and all production icon/portrait fields. build_release.py now runs these
checks in the release snapshot before cooking and stops on missing bindings.
P_RD.UI.ShopFullGenerated.RestoredArtifactIcons checks Arcane Crystal's actual shop
image brush, and saves a rendered capture when an RHI is available.

Original artwork and backup:
D:/Builds/P_RD/AssetAudit_20260909/UIBindingRepair_sources_and_assets.zip
The original generated images are also preserved in the user's F: image directory.
