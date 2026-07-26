"""Audit isolated combat-layout captures and compare semantic components.

The element capture pass emits one full-canvas RGBA PNG per UWidget.  This
script first checks whether pixels are really confined to the widget's declared
rectangle, then compares meaningful component roots against manually verified
reference boxes from KK_HUD_Polish_01.png.

It intentionally does not rank leaf widgets against the same screen rectangle:
leaf captures can contain visual ancestors, and position error must not be
mistaken for design error.
"""

from __future__ import annotations

import json
import math
import re
from dataclasses import asdict, dataclass
from pathlib import Path

import cv2
import numpy as np
from PIL import Image, ImageDraw, ImageFont
from skimage.metrics import structural_similarity


PROJECT = Path(r"D:\UnrealProjects\P_RD_develop_20260726")
ELEMENTS = (
    PROJECT
    / "Saved/UI/CombatLayouts/Elements/WBP_CombatLayout_01_ClassicCRPG"
)
REFERENCE = Path(
    r"C:\Users\2009e\.codex\generated_images"
    r"\019f9aad-5208-7490-923f-096eb077c69f"
    r"\exec-3902092b-d641-498a-b7df-53f271795566.png"
)
OUT = (
    PROJECT
    / "Saved/UI/CombatLayouts/ElementAnalysis/WBP_CombatLayout_01_ClassicCRPG"
)
PAIR_OUT = OUT / "Pairs"
NAME_RE = re.compile(r"_(\d+)x(\d+)_at(-?\d+)_(-?\d+)\.png$")


@dataclass(frozen=True)
class Component:
    name: str
    actual_prefix: str
    reference_box: tuple[int, int, int, int]
    group: str


COMPONENTS = [
    Component("RoundPanel", "CanvasPanel_RoundPanel_", (15, 12, 260, 132), "top"),
    Component(
        "ObjectivePanel",
        "CanvasPanel_ObjectivePanel_",
        (1302, 12, 1654, 131),
        "top",
    ),
    Component("TurnToken_0", "CanvasPanel_TurnToken_0_", (491, 12, 629, 132), "top"),
    Component("TurnToken_1", "CanvasPanel_TurnToken_1_", (629, 12, 766, 132), "top"),
    Component("TurnToken_2", "CanvasPanel_TurnToken_2_", (766, 12, 904, 132), "top"),
    Component("TurnToken_3", "CanvasPanel_TurnToken_3_", (904, 12, 1041, 132), "top"),
    Component("TurnToken_4", "CanvasPanel_TurnToken_4_", (1041, 12, 1179, 132), "top"),
    Component("PartyCard_0", "CanvasPanel_PartyCard_0_", (15, 582, 456, 693), "bottom"),
    Component("PartyCard_1", "CanvasPanel_PartyCard_1_", (15, 693, 456, 804), "bottom"),
    Component("PartyCard_2", "CanvasPanel_PartyCard_2_", (15, 804, 456, 915), "bottom"),
    Component("CommandCard_0", "CanvasPanel_CommandCard_0_", (478, 617, 625, 923), "bottom"),
    Component("CommandCard_1", "CanvasPanel_CommandCard_1_", (625, 617, 772, 923), "bottom"),
    Component("CommandCard_2", "CanvasPanel_CommandCard_2_", (772, 617, 919, 923), "bottom"),
    Component("CommandCard_3", "CanvasPanel_CommandCard_3_", (919, 617, 1066, 923), "bottom"),
    Component("CommandCard_4", "CanvasPanel_CommandCard_4_", (1066, 617, 1213, 923), "bottom"),
    Component("CommandCard_5", "CanvasPanel_CommandCard_5_", (1213, 617, 1360, 923), "bottom"),
    Component("EnemyPanel", "CanvasPanel_EnemyPanel_", (1358, 617, 1661, 826), "bottom"),
    Component("EndTurn", "Button_EndTurnButton_", (1358, 836, 1663, 923), "bottom"),
]


def find_one(prefix: str) -> Path:
    # Component roots have a numeric size immediately after the prefix.
    # Names such as ``RoundPanel_Canvas`` are implementation children and must
    # not be mistaken for the semantic root.
    hits = sorted(ELEMENTS.glob(prefix + "[0-9]*.png"))
    if len(hits) != 1:
        raise RuntimeError(f"{prefix}: expected one capture, found {len(hits)}")
    return hits[0]


def alpha_bbox(rgba: np.ndarray) -> tuple[int, int, int, int]:
    ys, xs = np.where(rgba[:, :, 3] > 0)
    if not len(xs):
        return (0, 0, 0, 0)
    return int(xs.min()), int(ys.min()), int(xs.max() + 1), int(ys.max() + 1)


def crop_box(image: np.ndarray, box: tuple[int, int, int, int]) -> np.ndarray:
    x0, y0, x1, y1 = box
    return image[y0:y1, x0:x1]


def composite(rgba: np.ndarray, background=(31, 34, 39)) -> np.ndarray:
    rgb = rgba[:, :, :3].astype(np.float32)
    alpha = rgba[:, :, 3:4].astype(np.float32) / 255.0
    bg = np.array(background, dtype=np.float32).reshape(1, 1, 3)
    return np.clip(rgb * alpha + bg * (1.0 - alpha), 0, 255).astype(np.uint8)


def resize_rgb(image: np.ndarray, width: int, height: int) -> np.ndarray:
    return cv2.resize(image, (width, height), interpolation=cv2.INTER_LANCZOS4)


def edge_map(rgb: np.ndarray) -> np.ndarray:
    gray = cv2.cvtColor(rgb, cv2.COLOR_RGB2GRAY)
    return cv2.Canny(gray, 55, 135) > 0


def edge_f1_tolerance(actual: np.ndarray, target: np.ndarray, radius=2) -> float:
    a = edge_map(actual).astype(np.uint8)
    t = edge_map(target).astype(np.uint8)
    kernel = np.ones((radius * 2 + 1, radius * 2 + 1), np.uint8)
    ad = cv2.dilate(a, kernel) > 0
    td = cv2.dilate(t, kernel) > 0
    a_bool = a > 0
    t_bool = t > 0
    precision = float((a_bool & td).sum()) / max(1, int(a_bool.sum()))
    recall = float((t_bool & ad).sum()) / max(1, int(t_bool.sum()))
    return 2 * precision * recall / max(1e-9, precision + recall)


def image_stats(rgb: np.ndarray) -> dict[str, float]:
    gray = cv2.cvtColor(rgb, cv2.COLOR_RGB2GRAY).astype(np.float32)
    hsv = cv2.cvtColor(rgb, cv2.COLOR_RGB2HSV)
    gx = cv2.Sobel(gray, cv2.CV_32F, 1, 0, ksize=3)
    gy = cv2.Sobel(gray, cv2.CV_32F, 0, 1, ksize=3)
    gradient = np.hypot(gx, gy)
    return {
        "luminance_mean": float(gray.mean()),
        "luminance_stddev": float(gray.std()),
        "saturation_mean": float(hsv[:, :, 1].mean() / 255.0),
        "edge_density": float(edge_map(rgb).mean()),
        "gradient_p50": float(np.percentile(gradient, 50)),
        "gradient_p90": float(np.percentile(gradient, 90)),
    }


def contain(image: Image.Image, size: tuple[int, int], background) -> Image.Image:
    canvas = Image.new("RGB", size, background)
    source = image.convert("RGB")
    source.thumbnail((size[0] - 12, size[1] - 12), Image.Resampling.LANCZOS)
    canvas.paste(source, ((size[0] - source.width) // 2, (size[1] - source.height) // 2))
    return canvas


def heatmap(actual: np.ndarray, target: np.ndarray) -> np.ndarray:
    delta = np.max(
        np.abs(actual.astype(np.int16) - target.astype(np.int16)), axis=2
    ).astype(np.uint8)
    colored = cv2.applyColorMap(delta, cv2.COLORMAP_INFERNO)
    return cv2.cvtColor(colored, cv2.COLOR_BGR2RGB)


def pair_image(
    name: str, actual: np.ndarray, target: np.ndarray, delta: np.ndarray
) -> Image.Image:
    cell = (360, 250)
    header = 30
    out = Image.new("RGB", (cell[0] * 3, cell[1] + header), (23, 25, 29))
    draw = ImageDraw.Draw(out)
    draw.text((8, 8), f"{name} | actual isolated", fill=(235, 235, 235))
    draw.text((cell[0] + 8, 8), "reference", fill=(235, 235, 235))
    draw.text((cell[0] * 2 + 8, 8), "max-channel delta", fill=(235, 235, 235))
    for index, array in enumerate((actual, target, delta)):
        panel = contain(Image.fromarray(array), cell, (31, 34, 39))
        out.paste(panel, (index * cell[0], header))
    return out


def audit_captures() -> dict[str, object]:
    rows = []
    for path in sorted(ELEMENTS.glob("*.png")):
        match = NAME_RE.search(path.name)
        if not match:
            continue
        width, height, x, y = map(int, match.groups())
        rgba = np.asarray(Image.open(path).convert("RGBA"))
        nonzero = rgba[:, :, 3] > 0
        total = int(nonzero.sum())
        x0, y0 = max(0, x), max(0, y)
        x1, y1 = min(rgba.shape[1], x + width), min(rgba.shape[0], y + height)
        inside = int(nonzero[y0:y1, x0:x1].sum()) if x1 > x0 and y1 > y0 else 0
        outside_ratio = 1.0 if total == 0 else (total - inside) / total
        rows.append(
            {
                "file": path.name,
                "widget_type": path.name.split("_", 1)[0],
                "declared_rect": [x, y, width, height],
                "alpha_bbox": list(alpha_bbox(rgba)),
                "alpha_pixels": total,
                "outside_declared_ratio": outside_ratio,
            }
        )
    by_type: dict[str, dict[str, float | int]] = {}
    for kind in sorted({row["widget_type"] for row in rows}):
        values = [
            float(row["outside_declared_ratio"])
            for row in rows
            if row["widget_type"] == kind
        ]
        by_type[kind] = {
            "count": len(values),
            "median_outside_ratio": float(np.median(values)),
            "over_5_percent": sum(value > 0.05 for value in values),
            "over_50_percent": sum(value > 0.50 for value in values),
        }
    return {
        "capture_count": len(rows),
        "empty_count": sum(row["alpha_pixels"] == 0 for row in rows),
        "outside_over_5_percent": sum(
            float(row["outside_declared_ratio"]) > 0.05 for row in rows
        ),
        "outside_over_50_percent": sum(
            float(row["outside_declared_ratio"]) > 0.50 for row in rows
        ),
        "by_widget_type": by_type,
        "captures": rows,
    }


def compare_components() -> list[dict[str, object]]:
    reference = np.asarray(Image.open(REFERENCE).convert("RGB"))
    results = []
    sheets: dict[str, list[Image.Image]] = {"top": [], "bottom": []}
    PAIR_OUT.mkdir(parents=True, exist_ok=True)
    for component in COMPONENTS:
        actual_path = find_one(component.actual_prefix)
        actual_rgba_full = np.asarray(Image.open(actual_path).convert("RGBA"))
        actual_box = alpha_bbox(actual_rgba_full)
        actual_rgba = crop_box(actual_rgba_full, actual_box)
        actual_rgb = composite(actual_rgba)
        target_rgb = crop_box(reference, component.reference_box)
        target_height, target_width = target_rgb.shape[:2]
        actual_normalized = resize_rgb(actual_rgb, target_width, target_height)
        delta = heatmap(actual_normalized, target_rgb)
        metrics = {
            "name": component.name,
            "group": component.group,
            "actual_file": actual_path.name,
            "actual_alpha_bbox": list(actual_box),
            "reference_box": list(component.reference_box),
            "actual_visible_size": [actual_box[2] - actual_box[0], actual_box[3] - actual_box[1]],
            "reference_size": [target_width, target_height],
            "width_ratio_actual_to_reference": (
                (actual_box[2] - actual_box[0]) / max(1, target_width)
            ),
            "height_ratio_actual_to_reference": (
                (actual_box[3] - actual_box[1]) / max(1, target_height)
            ),
            "rgb_mae": float(
                np.abs(
                    actual_normalized.astype(np.float32) - target_rgb.astype(np.float32)
                ).mean()
            ),
            "ssim_rgb": float(
                structural_similarity(
                    actual_normalized, target_rgb, channel_axis=2, data_range=255
                )
            ),
            "edge_f1_tolerance_2px": edge_f1_tolerance(
                actual_normalized, target_rgb
            ),
            "actual_stats": image_stats(actual_normalized),
            "reference_stats": image_stats(target_rgb),
        }
        results.append(metrics)
        pair = pair_image(component.name, actual_normalized, target_rgb, delta)
        pair.save(PAIR_OUT / f"{component.name}.png")
        sheets[component.group].append(pair)

    for group, panels in sheets.items():
        width = max(panel.width for panel in panels)
        height = sum(panel.height for panel in panels)
        sheet = Image.new("RGB", (width, height), (17, 19, 22))
        y = 0
        for panel in panels:
            sheet.paste(panel, (0, y))
            y += panel.height
        sheet.save(OUT / f"semantic_pairs_{group}.png")
    return results


def markdown_report(audit: dict[str, object], comparisons: list[dict[str, object]]) -> str:
    lines = [
        "# Element capture audit and semantic comparison",
        "",
        f"- Captures: {audit['capture_count']}",
        f"- Empty: {audit['empty_count']}",
        f"- More than 5% alpha outside declared widget rect: {audit['outside_over_5_percent']}",
        f"- More than 50% outside: {audit['outside_over_50_percent']}",
        "",
        "Leaf captures with large outside ratios contain visual ancestors. They are useful",
        "for placement inspection, but must not be treated as isolated asset pixels.",
        "",
        "## Isolation by widget type",
        "",
        "| Type | count | median outside | >5% | >50% |",
        "|---|---:|---:|---:|---:|",
    ]
    for kind, value in audit["by_widget_type"].items():
        lines.append(
            f"| {kind} | {value['count']} | {value['median_outside_ratio']:.3f} | "
            f"{value['over_5_percent']} | {value['over_50_percent']} |"
        )
    lines += [
        "",
        "## Semantic component comparison",
        "",
        "Actual semantic roots are alpha-cropped and resized to separately defined",
        "reference boxes. Screen position is therefore excluded from SSIM and edge F1.",
        "",
        "| Component | size W | size H | SSIM | edge F1 | RGB MAE | edge density A/T |",
        "|---|---:|---:|---:|---:|---:|---:|",
    ]
    for value in comparisons:
        lines.append(
            f"| {value['name']} | {value['width_ratio_actual_to_reference']:.2f}× | "
            f"{value['height_ratio_actual_to_reference']:.2f}× | "
            f"{value['ssim_rgb']:.3f} | {value['edge_f1_tolerance_2px']:.3f} | "
            f"{value['rgb_mae']:.1f} | "
            f"{value['actual_stats']['edge_density']:.3f}/"
            f"{value['reference_stats']['edge_density']:.3f} |"
        )
    return "\n".join(lines) + "\n"


def main() -> None:
    OUT.mkdir(parents=True, exist_ok=True)
    audit = audit_captures()
    comparisons = compare_components()
    report = {
        "inputs": {
            "elements": str(ELEMENTS),
            "reference": str(REFERENCE),
        },
        "capture_audit": audit,
        "semantic_comparisons": comparisons,
    }
    (OUT / "analysis.json").write_text(
        json.dumps(report, ensure_ascii=False, indent=2), encoding="utf-8"
    )
    (OUT / "analysis.md").write_text(
        markdown_report(audit, comparisons), encoding="utf-8"
    )
    print(markdown_report(audit, comparisons))


if __name__ == "__main__":
    main()
