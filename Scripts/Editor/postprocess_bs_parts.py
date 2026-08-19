# -*- coding: utf-8 -*-
"""Reward BS 파츠 후처리.

검증된 C03 후처리 구현을 그대로 호출하며 BS 경로, MANIFEST, 출력 접두사만
교체한다. 기준틀 탐지·마젠타 키잉·despill·premultiplied resize 로직은
postprocess_c03_parts.py가 단일 원본이다.
"""
import hashlib
import os
import sys

import numpy as np

import postprocess_c03_parts as core


PROJECT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
RAW_DIR = os.path.join(PROJECT, "Saved", "DesignAssets", "RewardBS", "RawGenerations")
OUT_DIR = os.path.join(PROJECT, "Saved", "DesignAssets", "RewardBS", "GeneratedParts")
MANIFEST = {
    "battle_backdrop": (1536, 170),
    "sheet_background": (1472, 694),
    "sheet_frame": (1472, 694),
    "title_plate": (360, 84),
    "stage_tab": (160, 58),
    "step_track": (760, 22),
    "step_fill": (744, 12),
    "step_coin_active": (76, 76),
    "step_coin_inactive": (64, 64),
    "cta_button": (360, 88),
    "parchment_window": (500, 282),
    "xp_track": (330, 40),
    "xp_fill": (314, 24),
    "card_blank": (280, 320),
    "selection_glow": (292, 332),
    "chest_burst": (420, 360),
}


def main():
    if not os.path.isdir(RAW_DIR):
        print(f"입력 폴더 없음: {RAW_DIR}")
        sys.exit(1)
    os.makedirs(OUT_DIR, exist_ok=True)
    raw_files = sorted(f for f in os.listdir(RAW_DIR) if f.lower().endswith(".png"))
    results, hashes = [], {}
    for part, target in MANIFEST.items():
        matches = [f for f in raw_files if f.startswith(part)]
        if not matches:
            continue
        out, note = core.process(part, os.path.join(RAW_DIR, matches[0]), target)
        if out is None:
            results.append((part, "FAIL", note))
            continue
        name = f"bs_{part}_{target[0]}x{target[1]}.png"
        path = os.path.join(OUT_DIR, name)
        out.save(path)
        alpha = np.array(out)[:, :, 3]
        checks = []
        if alpha.min() > 0:
            checks.append("외곽 투명 영역 없음(알파 min>0)")
        digest = hashlib.md5(open(path, "rb").read()).hexdigest()
        if digest in hashes:
            checks.append(f"중복: {hashes[digest]}와 동일 파일")
        hashes[digest] = name
        if note:
            checks.append(note)
        results.append((part, "FAIL" if checks else "PASS", "; ".join(checks) or name))

    width = max((len(part) for part, _, _ in results), default=10)
    fails = 0
    for part, status, message in results:
        fails += status == "FAIL"
        print(f"{part:<{width}}  {status}  {message}")
    missing = [part for part in MANIFEST if not any(row[0] == part for row in results)]
    if missing:
        print(f"\n미생성: {', '.join(missing)}")
        fails += len(missing)
    print(f"\n{len(results)}개 처리, FAIL {fails}건 -> {OUT_DIR}")
    sys.exit(1 if fails else 0)


if __name__ == "__main__":
    main()
