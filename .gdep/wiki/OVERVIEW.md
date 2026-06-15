---
title: Project Overview
type: overview
engine: UNREAL · Unreal Engine 5.7 · (C++)
updated: 2026-06-15
stale: false
---

# P_RD — Project Overview

> Engine: UNREAL · Unreal Engine 5.7 · (C++)  |  Source: `D:\UnrealProjects\P_RD_refactor_reorganize-gamemodes\Source`
> Generated: 2026-06-15
> Regenerate: `gdep init <project_path> --force`

## Codebase Snapshot

| Metric | Value |
|--------|-------|
| Files | **99** |
| Classes | **105** |
| Circular deps | **0** |
| Dead code candidates | **0** |

> **Dead code** = classes with in-degree 0 (no other class references them in code).
> Unity: prefab/scene asset references are also checked — a class used only from a prefab is NOT dead code.
> UE5: Blueprint, ABP, Montage, and GAS .uasset references are also checked — a class used only from a .uasset is NOT dead code.
> Generic C++: only source-code references are checked.

### High-coupling classes (likely God Objects — modify with care)

- `URDUserWidget` — 12 dependents
- `FTileTransform` — 5 dependents
- `AUnit` — 4 dependents
- `FRoom` — 4 dependents
- `IRunDataWriter` — 4 dependents

## GAS Summary

- Abilities (C++): 1
- Effects (C++): 2
- AttributeSets: 3
- GameplayTags (in assets): 2 high-confidence
- GAS .uassets — IS-A: GA 0 / GE 0 / AS 0 / ABP 0  |  Referencers: 0
- Analysis confidence: **HIGH** (cpp_source_regex + binary_pattern_match)
- Coverage: 73/73 assets (100.0%)

---
See [[index]] for the full wiki page catalog.
