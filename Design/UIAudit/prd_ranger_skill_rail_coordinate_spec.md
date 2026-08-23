# P_RD Ranger Skill Rail UI Coordinate Spec

## Purpose

This pilot converts the corrected visual concept into measurable targets for the Ranger skill-rail HUD.

- Measurement source: `09-skill-rail-ranger-en.jpg`
- Measurement canvas: **900 x 415 px**
- Original capture canvas: **2340 x 1080 px**
- Scale from measurement canvas to capture canvas: `X x 2.6`, `Y x 2.6024096`
- The generated corrected concept is a visual reference only. Coordinates come from the untouched source screenshot overlay.

## Deliverables

- `prd_ranger_skill_rail_corrected_concept_v1.png`: generated corrected visual concept.
- `prd_ranger_skill_rail_coordinate_overlay_v1.png`: deterministic coordinate overlay on the untouched source image.
- `prd_round_counter_plate_v1.png`: transparent two-digit round-number backing plate.

## Annotation map

Coordinates use top-left origin. Rect format is `X, Y, Width, Height`.

| No. | Element/action | 900 x 415 rect | Normalized rect | 2340 x 1080 rect | WBP guidance |
|---:|---|---|---|---|---|
| 1 | Round number plate target | `17, 42, 60, 38` | `0.0189, 0.1012, 0.0667, 0.0916` | `44, 109, 156, 99` | Top-left anchor. Use the plate as a 9-slice Box and center the dynamic two-digit text. |
| 2 | Remove turn-order speed values | `106, 52, 151, 13` | `0.1178, 0.1253, 0.1678, 0.0313` | `276, 135, 393, 34` | Collapse the speed child in every turn-order entry; do not move the portraits horizontally. |
| 3 | Remove `1/tile` | `467, 108, 32, 15` | `0.5189, 0.2602, 0.0356, 0.0361` | `1214, 281, 83, 39` | Remove/collapse this TextBlock. |
| 4 | Move title safe area | `425, 94, 52, 23` | `0.4722, 0.2265, 0.0578, 0.0554` | `1105, 245, 135, 60` | Center `Move`; no per-tile suffix. |
| 5 | Warm-Up title safe area | `311, 147, 61, 20` | `0.3456, 0.3542, 0.0678, 0.0482` | `809, 383, 159, 52` | Scale down only when required. |
| 6 | Flying Kick title safe area | `526, 148, 58, 20` | `0.5844, 0.3566, 0.0644, 0.0482` | `1368, 385, 151, 52` | Scale down only when required. |
| 7 | Snipe title safe area | `312, 253, 62, 20` | `0.3467, 0.6096, 0.0689, 0.0482` | `811, 658, 161, 52` | Keep existing font size because it already fits. |
| 8 | Hurricane Kick title safe area | `524, 252, 62, 20` | `0.5822, 0.6072, 0.0689, 0.0482` | `1362, 656, 161, 52` | Shrink to fit within this rect. |
| 9 | Piercing Volley title safe area | `419, 302, 62, 20` | `0.4656, 0.7277, 0.0689, 0.0482` | `1089, 786, 161, 52` | Shrink to fit within this rect. |
| 10 | Compact Ranger summary target | `696, 67, 193, 100` | `0.7733, 0.1614, 0.2144, 0.2410` | `1810, 174, 502, 260` | Preserve the top-right anchor and top edge. Reduce only the bottom extent. |
| 11 | Optional: remove summary speed row | `753, 136, 37, 19` | `0.8367, 0.3277, 0.0411, 0.0458` | `1958, 354, 96, 49` | Apply only if the speed-removal agreement covers the summary panel as well as the turn-order strip. |
| 12 | Collapse unused summary lower bar | `696, 166, 193, 38` | `0.7733, 0.4000, 0.2144, 0.0916` | `1810, 432, 502, 99` | Collapse/remove the lower container when no AP-icon content exists. |

## Recommended skill-title widget structure

Do not place skill-name TextBlocks directly on the Canvas with independent screen coordinates. Use one shared card-local layout:

1. Add a fixed title `SizeBox` inside the skill card.
2. Give the title area roughly `8 px` horizontal card-local padding on the 900 x 415 reference layout.
3. Put a `ScaleBox` inside it with `Stretch = Scale To Fit` and `Stretch Direction = Down Only`.
4. Put the TextBlock inside the ScaleBox with horizontal and vertical center alignment.
5. Keep wrapping disabled for the radial card. If the minimum readable scale is exceeded, switch to a deliberate two-line title variant rather than enlarging the whole card.

This keeps short names such as `Snipe` unchanged while shrinking only long names such as `Hurricane Kick` and `Piercing Volley`.

## Pixel-to-WBP conversion

Use the screenshot rectangle as the target screen-space result, not as a raw Canvas Slot position.

1. Convert physical screenshot pixels to Slate units using the active viewport DPI scale.
2. Preserve the current parent panel and anchor whenever possible.
3. For a Canvas child, account for both the anchor origin and alignment pivot before writing the slot offset.
4. Skill-title rects should be implemented in card-local space, so they should not require per-resolution screen-coordinate changes.

## Image-generation prompts

Built-in image generation/editing mode was used.

### Round plate

Create one isolated transparent fantasy wooden-and-bronze HUD plate matching the supplied P_RD screenshot. The plate sits beneath `ROUND`, contains no text or numbers, is front-facing and symmetrical, and has a clean two-digit safe area.

### Corrected HUD concept

Edit only the supplied Ranger skill-rail screenshot: add a framed backing behind `02`; remove portrait speed values; remove `1/tile`; fit all skill names inside their cards; remove the summary speed row; and collapse the unused summary lower portion while preserving all other HUD and battlefield content.
