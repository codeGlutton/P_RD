"""Verify the built skill/enemy detail WBPs: bounds, overlaps, required widgets.

Writes Saved/LegacyAudit/detail_verify.txt.
"""

from pathlib import Path

import unreal

OUT = Path("D:/UnrealProjects/P_RD_develop_20260803/Saved/LegacyAudit/detail_verify.txt")

SCREEN = (0.0, 0.0, 1920.0, 1080.0)

TARGETS = {
    "/Game/UI/CombatDetail/WBP_SkillDetail_Marchbound": {
        "required": [
            "SkillDetailBaseFrame", "SkillDetailTitleText", "SkillDetailCloseButton",
            "SkillIconImage", "SkillNameText", "SkillGradeText", "SkillKindText",
            "SkillChipCostValue", "SkillChipDamageValue", "SkillChipCooldownValue",
            "SkillChipHitsValue", "SkillChipRangeValue",
            "SkillAimBlockerValue", "SkillEffectBlockerValue", "SkillRuleNoteText",
            "SkillRangeCell_R2C2", "SkillAreaCell_R2C2",
            "SkillRangeCaption", "SkillAreaCaption",
            "SkillEffectText_0", "SkillEffectText_2", "SkillFlavorText",
        ],
        # 겹치면 글이 다른 칸 위로 흐르거나 클릭이 막히는 짝
        "no_overlap": [
            ("SkillIdentityWell", "SkillRuleWell"),
            ("SkillIdentityWell", "SkillRangeWell"),
            ("SkillRuleWell", "SkillEffectWell"),
            ("SkillRangeWell", "SkillEffectWell"),
            ("SkillIconFrame", "SkillNameText"),
            ("SkillRangeCell_R2C4", "SkillAreaHeading"),
            ("SkillAimBlockerValue", "SkillGrantHeading"),
        ],
        "inside": [
            ("SkillIconFrame", "SkillIdentityWell"),
            ("SkillChipRangeFrame", "SkillIdentityWell"),
            ("SkillRuleNoteText", "SkillRuleWell"),
            ("SkillRangeCaption", "SkillRangeWell"),
            ("SkillAreaCell_R4C4", "SkillRangeWell"),
            ("SkillFlavorText", "SkillEffectWell"),
        ],
    },
    "/Game/UI/CombatDetail/WBP_EnemyDetail_Marchbound": {
        "required": [
            "EnemyDetailBaseFrame", "EnemyDetailTitleText", "EnemyDetailCloseButton",
            "EnemyPortraitImage", "EnemyNameText", "EnemyLevelText",
            "EnemyHpBar", "EnemyHpText",
            "EnemyChipApValue", "EnemyChipSpeedValue", "EnemyChipDefenseValue",
            "EnemyChipThreatValue",
            "EnemyStatus_0Text", "EnemySkillRowName_0", "EnemySkillRowEffect_3",
            "EnemyThreatCell_R3C3", "EnemyLegendMoveText", "EnemyLegendAttackText",
            "EnemyPassiveText",
        ],
        "no_overlap": [
            ("EnemyIdentityWell", "EnemyStatusWell"),
            ("EnemyIdentityWell", "EnemySkillWell"),
            ("EnemyStatusWell", "EnemyThreatWell"),
            ("EnemySkillWell", "EnemyThreatWell"),
            ("EnemyPortraitFrame", "EnemyNameText"),
            ("EnemyThreatCell_R3C6", "EnemyLegendMoveSwatch"),
            ("EnemySkillRowName_0", "EnemySkillRowCost_0"),
            ("EnemySkillRowCost_0", "EnemySkillRowEffect_0"),
            ("EnemySkillRow_0", "EnemySkillRow_1"),
        ],
        "inside": [
            ("EnemyPortraitFrame", "EnemyIdentityWell"),
            ("EnemyChipThreatFrame", "EnemyIdentityWell"),
            ("EnemyStatus_3Text", "EnemyStatusWell"),
            ("EnemySkillRow_3", "EnemySkillWell"),
            ("EnemyThreatCell_R6C6", "EnemyThreatWell"),
            ("EnemyPassiveText", "EnemyThreatWell"),
        ],
    },
}

lines = []


def rect_of(widget):
    slot = widget.get_editor_property("slot")
    if not isinstance(slot, unreal.CanvasPanelSlot):
        return None
    layout = slot.get_editor_property("layout_data")
    offsets = layout.get_editor_property("offsets")
    return (offsets.left, offsets.top, offsets.right, offsets.bottom)


def overlaps(a, b):
    return not (a[0] + a[2] <= b[0] or b[0] + b[2] <= a[0]
                or a[1] + a[3] <= b[1] or b[1] + b[3] <= a[1])


def contains(outer, inner):
    return (inner[0] >= outer[0] - 0.5 and inner[1] >= outer[1] - 0.5
            and inner[0] + inner[2] <= outer[0] + outer[2] + 0.5
            and inner[1] + inner[3] <= outer[1] + outer[3] + 0.5)


for asset_path, spec in TARGETS.items():
    lines.append("=== %s ===" % asset_path.rsplit("/", 1)[-1])
    blueprint = unreal.EditorAssetLibrary.load_asset(asset_path)
    if blueprint is None:
        lines.append("  MISSING ASSET")
        continue

    rects = {}
    tree_prefix = blueprint.get_path_name() + ":WidgetTree."
    for obj in unreal.ObjectIterator():
        if isinstance(obj, unreal.Widget) and str(obj.get_path_name()).startswith(tree_prefix):
            rect = rect_of(obj)
            if rect is not None:
                rects[str(obj.get_name())] = rect
    lines.append("  canvas widgets: %d" % len(rects))

    missing = [name for name in spec["required"] if name not in rects]
    lines.append("  required missing: %s" % (missing if missing else "none"))

    offscreen = [
        name for name, rect in rects.items()
        if not contains(SCREEN, rect)
    ]
    lines.append("  off-screen: %s" % (offscreen if offscreen else "none"))

    collisions = [
        "%s x %s" % (first, second)
        for first, second in spec["no_overlap"]
        if first in rects and second in rects and overlaps(rects[first], rects[second])
    ]
    lines.append("  collisions: %s" % (collisions if collisions else "none"))

    escaped = [
        "%s not inside %s" % (inner, outer)
        for inner, outer in spec["inside"]
        if inner in rects and outer in rects and not contains(rects[outer], rects[inner])
    ]
    lines.append("  outside its section: %s" % (escaped if escaped else "none"))

OUT.write_text("\n".join(lines), encoding="utf-8")
