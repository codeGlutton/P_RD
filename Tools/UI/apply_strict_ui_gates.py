"""Apply approval-grade hard gates to the element comparison results."""

from __future__ import annotations

import json
import io
import sys
from pathlib import Path


ROOT = Path(
    r"D:\UnrealProjects\P_RD_develop_20260726\Saved\UI\CombatLayouts"
    r"\ElementAnalysis\WBP_CombatLayout_01_ClassicCRPG"
)
INPUT = ROOT / "analysis.json"

LIMITS = {
    "capture_outside_ratio_max": 0.005,
    "size_ratio_error_max": 0.03,
    "ssim_min": 0.75,
    "edge_f1_min": 0.90,
    "rgb_mae_max": 12.0,
    "edge_density_ratio_min": 0.85,
    "edge_density_ratio_max": 1.15,
    "material_delta_e_max": 2.0,
    "material_contrast_ratio_min": 0.85,
    "material_contrast_ratio_max": 1.15,
    "material_gradient_ratio_min": 0.85,
    "material_gradient_ratio_max": 1.15,
}

# Values measured from color-family masks on the current root capture.
MATERIALS = {
    "Parchment": {"delta_e": 0.924, "contrast_ratio": 0.482, "gradient_ratio": 1.715},
    "PartyWood": {"delta_e": 1.284, "contrast_ratio": 0.927, "gradient_ratio": 2.329},
    "SkillStone": {"delta_e": 0.973, "contrast_ratio": 0.802, "gradient_ratio": 0.632},
    "EnemyRed": {"delta_e": 1.659, "contrast_ratio": 0.909, "gradient_ratio": 0.723},
    "Silver": {"delta_e": 1.065, "contrast_ratio": 0.904, "gradient_ratio": 0.841},
    "EndTurn": {"delta_e": 13.404, "contrast_ratio": 0.753, "gradient_ratio": 1.405},
}

MANUAL_BLOCKERS = {
    "RoundPanel": ["corner bracket art mismatch", "title weight/scale mismatch"],
    "ObjectivePanel": ["flag scale/render mismatch", "corner bracket art mismatch"],
    "TurnToken_0": ["portrait scale/render mismatch"],
    "TurnToken_1": ["portrait scale/render mismatch"],
    "TurnToken_2": ["archer palette/portrait mismatch"],
    "TurnToken_3": ["portrait scale/render mismatch"],
    "TurnToken_4": ["mage palette/portrait mismatch"],
    "PartyCard_0": ["white unfilled HP track", "wood frequency mismatch"],
    "PartyCard_1": ["flat HP track", "wood frequency mismatch"],
    "PartyCard_2": ["white unfilled HP track", "wood frequency mismatch"],
    "CommandCard_0": ["extra circular icon bezel"],
    "CommandCard_1": ["extra circular icon bezel"],
    "CommandCard_2": ["extra circular icon bezel", "shield-bash scale mismatch"],
    "CommandCard_3": ["extra circular icon bezel", "wrong icon silhouette"],
    "CommandCard_4": ["extra circular icon bezel", "wrong disabled icon silhouette"],
    "CommandCard_5": ["extra circular icon bezel", "wrong icon silhouette"],
    "EnemyPanel": ["portrait/bar scale mismatch", "flat HP track"],
    "EndTurn": ["extra hourglass", "wrong surface/color/depth"],
}


def between(value: float, low: float, high: float) -> bool:
    return low <= value <= high


def main() -> None:
    sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding="utf-8")
    source = json.loads(INPUT.read_text(encoding="utf-8"))
    capture_rows = source["capture_audit"]["captures"]
    clean_captures = [
        row
        for row in capture_rows
        if float(row["outside_declared_ratio"])
        <= LIMITS["capture_outside_ratio_max"]
    ]

    components = []
    for item in source["semantic_comparisons"]:
        edge_density_ratio = (
            item["actual_stats"]["edge_density"]
            / item["reference_stats"]["edge_density"]
        )
        gates = {
            "geometry": (
                abs(item["width_ratio_actual_to_reference"] - 1.0)
                <= LIMITS["size_ratio_error_max"]
                and abs(item["height_ratio_actual_to_reference"] - 1.0)
                <= LIMITS["size_ratio_error_max"]
            ),
            "ssim": item["ssim_rgb"] >= LIMITS["ssim_min"],
            "edge_f1": item["edge_f1_tolerance_2px"] >= LIMITS["edge_f1_min"],
            "rgb_mae": item["rgb_mae"] <= LIMITS["rgb_mae_max"],
            "edge_density": between(
                edge_density_ratio,
                LIMITS["edge_density_ratio_min"],
                LIMITS["edge_density_ratio_max"],
            ),
            "manual": not MANUAL_BLOCKERS.get(item["name"]),
        }
        components.append(
            {
                "name": item["name"],
                "gates": gates,
                "pass": all(gates.values()),
                "manual_blockers": MANUAL_BLOCKERS.get(item["name"], []),
                "values": {
                    "width_ratio": item["width_ratio_actual_to_reference"],
                    "height_ratio": item["height_ratio_actual_to_reference"],
                    "ssim": item["ssim_rgb"],
                    "edge_f1": item["edge_f1_tolerance_2px"],
                    "rgb_mae": item["rgb_mae"],
                    "edge_density_ratio": edge_density_ratio,
                },
            }
        )

    materials = []
    for name, values in MATERIALS.items():
        gates = {
            "color": values["delta_e"] <= LIMITS["material_delta_e_max"],
            "contrast": between(
                values["contrast_ratio"],
                LIMITS["material_contrast_ratio_min"],
                LIMITS["material_contrast_ratio_max"],
            ),
            "gradient": between(
                values["gradient_ratio"],
                LIMITS["material_gradient_ratio_min"],
                LIMITS["material_gradient_ratio_max"],
            ),
        }
        materials.append(
            {"name": name, "values": values, "gates": gates, "pass": all(gates.values())}
        )

    result = {
        "limits": LIMITS,
        "capture_gate": {
            "pass": len(clean_captures) == len(capture_rows),
            "passed": len(clean_captures),
            "total": len(capture_rows),
        },
        "component_gate": {
            "passed": sum(item["pass"] for item in components),
            "total": len(components),
            "components": components,
        },
        "material_gate": {
            "passed": sum(item["pass"] for item in materials),
            "total": len(materials),
            "materials": materials,
        },
    }
    (ROOT / "strict_gates.json").write_text(
        json.dumps(result, ensure_ascii=False, indent=2), encoding="utf-8"
    )

    lines = [
        "# Approval-grade strict gates",
        "",
        "## Thresholds",
        "",
        "- geometry size error: <= 3%",
        "- SSIM: >= 0.75",
        "- edge F1 (2 px tolerance): >= 0.90",
        "- RGB MAE: <= 12",
        "- edge-density ratio: 0.85–1.15",
        "- material median ΔE00: <= 2",
        "- material contrast and gradient ratios: 0.85–1.15",
        "- capture alpha outside declared widget rect: <= 0.5%",
        "- extra/missing/wrong semantic elements: zero tolerance",
        "",
        "## Result",
        "",
        f"- clean leaf captures: **{len(clean_captures)}/{len(capture_rows)} PASS**",
        f"- semantic components: **{sum(item['pass'] for item in components)}/{len(components)} PASS**",
        f"- materials: **{sum(item['pass'] for item in materials)}/{len(materials)} PASS**",
        "",
        "## Component gate matrix",
        "",
        "| Component | G | SSIM | Edge | MAE | Density | Manual | Final |",
        "|---|:---:|:---:|:---:|:---:|:---:|:---:|:---:|",
    ]
    mark = lambda value: "PASS" if value else "FAIL"
    for item in components:
        gates = item["gates"]
        lines.append(
            f"| {item['name']} | {mark(gates['geometry'])} | "
            f"{mark(gates['ssim'])} | {mark(gates['edge_f1'])} | "
            f"{mark(gates['rgb_mae'])} | {mark(gates['edge_density'])} | "
            f"{mark(gates['manual'])} | **{mark(item['pass'])}** |"
        )
    lines += [
        "",
        "## Material gate matrix",
        "",
        "| Material | ΔE | contrast | gradient | Final |",
        "|---|:---:|:---:|:---:|:---:|",
    ]
    for item in materials:
        gates = item["gates"]
        lines.append(
            f"| {item['name']} | {mark(gates['color'])} | "
            f"{mark(gates['contrast'])} | {mark(gates['gradient'])} | "
            f"**{mark(item['pass'])}** |"
        )
    lines += [
        "",
        "## Interpretation",
        "",
        "Nothing is approved under the strict reproduction gate. RoundPanel and",
        "ObjectivePanel pass geometry only; all components fail visual similarity",
        "and edge agreement. Every material fails at least one frequency/contrast",
        "gate. Base color alone is not sufficient for approval.",
        "",
    ]
    (ROOT / "strict_gates.md").write_text("\n".join(lines), encoding="utf-8")
    print("\n".join(lines))


if __name__ == "__main__":
    main()
