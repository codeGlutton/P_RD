# P_RD Ranger Skill Rail Coordinate Spec v3

## V3 corrections

1. The duplicated `02` plate directly touches the existing `ROUND` plate. The target top-left is `8, 30`; there is no background gap between the two outer borders.
2. Occupied turn-order entries no longer reserve a lower speed row. The bottom `13 px` strip is removed from the entry layout, while top alignment, portrait size and horizontal placement stay fixed.
3. Every skill title safe area moves upward by `5 px` relative to v2 and remains horizontally centered.
4. The upper orange `Skill` / `스킬` button uses horizontal and vertical center alignment, plus an optional `RenderTransform.Translation.Y = -2` optical correction for Korean glyph mass.
5. The Ranger summary panel remains untouched. Only its separate empty lower bar is removed, as locked in v2.

## Measurement basis

- English source: `09-skill-rail-ranger-en.jpg`
- Korean source: `08-skill-rail-ranger-ko.jpg`
- Measurement canvas: **900 x 415 px**
- Original capture: **2340 x 1080 px**
- Scale to capture: `X x 2.6`, `Y x 2.6024096`
- Generated V3 concepts are visual references only. WBP coordinates come from overlays on the untouched 900 x 415 source captures.

## Deliverables

- `prd_ranger_skill_rail_corrected_concept_en_v3.png`
- `prd_ranger_skill_rail_corrected_concept_ko_v3.png`
- `prd_ranger_skill_rail_corrected_concepts_en_ko_v3.png`
- `prd_ranger_skill_rail_coordinate_overlay_en_v3.png`
- `prd_ranger_skill_rail_coordinate_overlay_ko_v3.png`
- `prd_ranger_skill_rail_coordinates_v3.csv`

## Annotation map

Rect format is `X, Y, Width, Height`, with a top-left origin.

| No. | Element/action | 900 x 415 rect | Normalized rect | 2340 x 1080 rect | WBP guidance |
|---:|---|---|---|---|---|
| 1 | Duplicate existing ROUND bar for `02`, touching | `8, 30, 70, 28` | `0.0089, 0.0723, 0.0778, 0.0675` | `21, 78, 182, 73` | Duplicate the existing bar Image widget. Anchor it directly after the header with `Spacing = 0`, then center a two-digit TextBlock over it. |
| 2 | Remove occupied turn-entry lower segment | `106, 52, 151, 13` | `0.1178, 0.1253, 0.1678, 0.0313` | `276, 135, 393, 34` | Collapse the speed child and reduce each occupied entry height by the same lower-strip height. Keep top position and portrait size fixed. |
| 3 | Remove `1/tile` / `1/칸` | `467, 108, 32, 15` | `0.5189, 0.2602, 0.0356, 0.0361` | `1214, 281, 83, 39` | Collapse the suffix TextBlock. Do not compensate by shifting the card. |
| 4 | Move / 이동 title safe area | `425, 89, 52, 23` | `0.4722, 0.2145, 0.0578, 0.0554` | `1105, 232, 135, 60` | Center horizontally; move title slot up `5 px` from v2. |
| 5 | Warm-Up / 준비운동 title safe area | `311, 142, 61, 20` | `0.3456, 0.3422, 0.0678, 0.0482` | `809, 370, 159, 52` | Center horizontally; move title slot up `5 px`. |
| 6 | Flying Kick / 날려차기 title safe area | `526, 143, 58, 20` | `0.5844, 0.3446, 0.0644, 0.0482` | `1368, 372, 151, 52` | Center horizontally; move title slot up `5 px`. |
| 7 | Snipe / 저격 title safe area | `312, 248, 62, 20` | `0.3467, 0.5976, 0.0689, 0.0482` | `811, 645, 161, 52` | Center horizontally; move title slot up `5 px`. |
| 8 | Hurricane Kick / 허리케인 킥 title safe area | `524, 247, 62, 20` | `0.5822, 0.5952, 0.0689, 0.0482` | `1362, 643, 161, 52` | One line, centered, `Scale To Fit`, `Down Only`; move slot up `5 px`. |
| 9 | Piercing Volley / 관통 일제사격 title safe area | `419, 297, 62, 20` | `0.4656, 0.7157, 0.0689, 0.0482` | `1089, 773, 161, 52` | One line, centered, `Scale To Fit`, `Down Only`; move slot up `5 px`. |
| 12 | Remove only the empty summary lower bar | `696, 166, 193, 38` | `0.7733, 0.4000, 0.2144, 0.0916` | `1810, 432, 502, 99` | Hide/collapse only this child bar. Do not resize or reposition the main summary panel. |
| 13 | Skill / 스킬 button text safe area | `753, 289, 135, 39` | `0.8367, 0.6964, 0.1500, 0.0940` | `1958, 752, 351, 101` | `HAlign = Center`, `VAlign = Center`; use `Y = -2 px` only as an optical correction, not a new canvas offset. |

## Shared skill-title widget

Use the same title component for English and Korean:

1. Fixed card-local `SizeBox` matching the safe area.
2. `ScaleBox`: `Stretch = Scale To Fit`, `Stretch Direction = Down Only`.
3. TextBlock: one line, wrapping disabled, horizontal and vertical center.
4. About `8 px` horizontal padding on the 900 x 415 reference.
5. Long strings shrink; short strings keep the designed font size.

## Image editing record

Mode: built-in image editing, `precise-object-edit`.

The final prompt set locked all non-target UI and requested only these changes: make the two round plates touch with zero background gap; shorten occupied turn portraits from the bottom after speed removal; move every English/Korean skill title upward and center it; optically center `Skill` / `스킬` in the orange button. A targeted second pass changed only the round gap and occupied portrait-frame height.
