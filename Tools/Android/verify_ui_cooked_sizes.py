"""Verify both UI budgets from packaged Android `ListTextures` log captures."""
import argparse
import json
from pathlib import Path
import re
import sys

from ui_policy import TARGETS, MINIMUMS


def verify_log(text):
    if "Cooked/OnDisk:" not in text:
        raise ValueError("Use ListTextures from a packaged Android run, not editor dimensions")
    observed = {}
    for line in text.splitlines():
        for asset, maximum in TARGETS.items():
            if asset not in line:
                continue
            # UE prints '?' authored bias only when RequiresCookedData() is true.
            match = re.search(r"(\d+)x(\d+) \(\d+ KB, \?\),", line)
            if match:
                dimensions = [int(match[1]), int(match[2])]
                if max(dimensions) > maximum:
                    raise ValueError(f"{asset}: cooked size {dimensions} exceeds {maximum}")
                minimum = MINIMUMS.get(asset, (1, 1))
                if any(actual < required for actual, required in zip(dimensions, minimum)):
                    raise ValueError(f"{asset}: cooked size {dimensions} is below required frame resolution {minimum}")
                observed[asset] = dimensions
    missing = set(TARGETS) - set(observed)
    if missing:
        raise ValueError("Both target textures must be loaded and captured; missing " + ", ".join(sorted(missing)))
    return {"actual_cooked_sizes": observed, "passed": True}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("log", type=Path)
    args = parser.parse_args()
    try:
        print(json.dumps(verify_log(args.log.read_text(encoding="utf-8-sig", errors="replace")), indent=2))
    except (ValueError, OSError) as error:
        print(f"Cooked UI verification failed: {error}", file=sys.stderr)
        sys.exit(1)
