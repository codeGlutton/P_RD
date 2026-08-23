# P_RD Ranger Skill Rail Coordinate Spec v2

## Locked decisions

1. The round number uses a second instance of the existing `ROUND` bar image directly below the first bar. No newly generated round-frame asset is required.
2. Annotation 12 removes only the empty lower bar. The main Ranger summary panel, its size, frame, title, portrait, HP, AP and speed row stay unchanged.
3. English and Korean use the same card geometry and title safe areas.

## Measurement basis

- English source: `09-skill-rail-ranger-en.jpg`
- Korean source: `08-skill-rail-ranger-ko.jpg`
- Measurement canvas: **900 x 415 px**
- Original capture: **2340 x 1080 px**
- Scale to capture: `X x 2.6`, `Y x 2.6024096`
- Generated concepts are visual references only. All coordinates come from overlays on the untouched source captures.

## Deliverables

- `prd_ranger_skill_rail_corrected_concept_en_v2.png`
- `prd_ranger_skill_rail_corrected_concept_ko_v2.png`
- `prd_ranger_skill_rail_corrected_concepts_en_ko_v2.png`
- `prd_ranger_skill_rail_coordinate_overlay_en_v2.png`
- `prd_ranger_skill_rail_coordinate_overlay_ko_v2.png`
- `prd_ranger_skill_rail_coordinates_v2.csv`

## Annotation map

Rect format is `X, Y, Width, Height`, with a top-left origin.

| No. | Element/action | 900 x 415 rect | Normalized rect | 2340 x 1080 rect | WBP guidance |
|---:|---|---|---|---|---|
| 1 | Duplicate existing ROUND bar for `02` | `8, 44, 70, 28` | `0.0089, 0.1060, 0.0778, 0.0675` | `21, 115, 182, 73` | Duplicate the existing bar Image widget, keep the same size/style and center a dynamic two-digit TextBlock over it. |
| 2 | Remove turn-order speed values | `106, 52, 151, 13` | `0.1178, 0.1253, 0.1678, 0.0313` | `276, 135, 393, 34` | Collapse the speed child in each turn-order entry. Keep portrait positions unchanged. |
| 3 | Remove `1/tile` / `1/칸` | `467, 108, 32, 15` | `0.5189, 0.2602, 0.0356, 0.0361` | `1214, 281, 83, 39` | Collapse the suffix TextBlock and center `Move` / `이동` in the title safe area. |
| 4 | Move / 이동 title safe area | `425, 94, 52, 23` | `0.4722, 0.2265, 0.0578, 0.0554` | `1105, 245, 135, 60` | Shared local centered title slot. |
| 5 | Warm-Up / 준비운동 title safe area | `311, 147, 61, 20` | `0.3456, 0.3542, 0.0678, 0.0482` | `809, 383, 159, 52` | Both currently fit; keep downscale support for future strings. |
| 6 | Flying Kick / 날려차기 title safe area | `526, 148, 58, 20` | `0.5844, 0.3566, 0.0644, 0.0482` | `1368, 385, 151, 52` | Both currently fit. |
| 7 | Snipe / 저격 title safe area | `312, 253, 62, 20` | `0.3467, 0.6096, 0.0689, 0.0482` | `811, 658, 161, 52` | Both currently fit without shrinking. |
| 8 | Hurricane Kick / 허리케인 킥 title safe area | `524, 252, 62, 20` | `0.5822, 0.6072, 0.0689, 0.0482` | `1362, 656, 161, 52` | Both languages overflow in the source and require one-line downscaling. |
| 9 | Piercing Volley / 관통 일제사격 title safe area | `419, 302, 62, 20` | `0.4656, 0.7277, 0.0689, 0.0482` | `1089, 786, 161, 52` | Both languages overflow in the source and require one-line downscaling. |
| 12 | Remove only the empty summary lower bar | `696, 166, 193, 38` | `0.7733, 0.4000, 0.2144, 0.0916` | `1810, 432, 502, 99` | Hide/collapse only this child bar. Do not resize or reposition the outer summary panel and do not remove the speed row. |

## Korean verdict

Korean is not problem-free, but it fails in fewer places than a general fixed-font implementation might suggest.

- Fits in the source: `이동`, `준비운동`, `날려차기`, `저격`.
- Overflows or touches the card boundary: `허리케인 킥`, `관통 일제사격`.
- The same shared ScaleBox-based title slot can therefore handle both English and Korean. Per-language coordinates are unnecessary.

## Recommended WBP title structure

Use one card-local title component rather than per-language Canvas coordinates:

1. Fixed title `SizeBox` matching the safe area.
2. `ScaleBox` with `Stretch = Scale To Fit` and `Stretch Direction = Down Only`.
3. Centered TextBlock with wrapping disabled.
4. Card-local horizontal padding of approximately `8 px` on the 900 x 415 reference layout.
5. Keep the entire card size and radial card positions unchanged.

This leaves short names at their designed font size and shrinks only the two long strings in each language.

## Image-generation prompts

Built-in image editing mode was used. The final prompts requested the following exact changes while preserving all other content:

- Duplicate the existing ROUND bar below itself and center `02` in the duplicate.
- Remove portrait speed values.
- Remove `1/tile` or `1/칸`.
- Fit the exact English or Korean skill names inside the unchanged cards on one line.
- Keep the entire summary panel and its speed row unchanged; remove only the empty bottom bar.
