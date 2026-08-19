# -*- coding: utf-8 -*-
"""V12 보상 파츠 후처리: ImageGen 원본 -> 정규격 + 실제 알파 + 검증.

사용법:
    python Scripts/Editor/postprocess_reward_v12_parts.py

입력:  Saved/DesignAssets/RewardAtomicV12/RawGenerations/<파츠명>*.png
        (임의 해상도, 배경·투명영역이 순수 마젠타 #FF00FF 단색으로 생성된 것)
출력:  Saved/DesignAssets/RewardAtomicV12/GeneratedParts/reward_v12_<파츠명>_<WxH>.png
        (정확한 규격, 키잉으로 만든 실제 알파, 마젠타 잔광 제거)

검증(스펙 §5)을 통과하지 못한 파츠는 FAIL로 보고되며 재생성 대상이다.
전 파츠 PASS 전에는 임포트/조립을 진행하지 않는다.
"""
import hashlib
import os
import sys

import numpy as np
from PIL import Image

PROJECT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
RAW_DIR = os.path.join(PROJECT, "Saved", "DesignAssets", "RewardAtomicV12", "RawGenerations")
OUT_DIR = os.path.join(PROJECT, "Saved", "DesignAssets", "RewardAtomicV12", "GeneratedParts")

# 파츠명 -> (목표 W, H). 스펙 §3의 규격과 1:1.
MANIFEST = {
    "exp_row_plate": (1054, 132),
    "exp_portrait_ring": (116, 116),
    "exp_level_window": (96, 58),
    "exp_progress_track": (516, 22),
    "exp_xp_badge": (120, 64),
    "gold_panel_plate": (800, 228),
    "gold_coin_ring": (168, 168),
    "gold_amount_window": (300, 160),
    "card_frame": (240, 230),
    "card_name_plate": (240, 70),
    "card_selected_overlay": (240, 300),
    # §3-D 선택 파츠 (재생성 시)
    "modal_outer_frame": (1536, 864),
    "modal_background": (1536, 864),
    "header_plate": (720, 141),
    "step_plate": (380, 62),
    "cta_plate": (404, 70),
    "parchment_sheet": (1352, 498),
}

KEY = np.array([255.0, 0.0, 255.0])  # 순수 마젠타
FULL_T = 60.0    # 이 거리 이내는 완전 투명
FULL_O = 120.0   # 이 거리 밖은 완전 불투명
ASPECT_WARN = 0.10  # 트림 후 원본과 목표의 종횡비 오차 허용


def key_to_alpha(arr):
    rgb = arr[:, :, :3].astype(np.float64)
    dist = np.sqrt(((rgb - KEY) ** 2).sum(axis=2))
    alpha = np.clip((dist - FULL_T) / (FULL_O - FULL_T), 0.0, 1.0)
    return alpha, dist


def despill(rgb, alpha):
    """키 컬러와 섞인 반투명 픽셀에서 마젠타 성분을 역산한다."""
    a = alpha[:, :, None]
    edge = (a > 0.0) & (a < 1.0)
    safe_a = np.maximum(a, 1.0 / 255.0)
    unmixed = (rgb - (1.0 - a) * KEY[None, None, :]) / safe_a
    rgb = np.where(edge, unmixed, rgb)
    # ImageGen의 키 경계는 단순 알파 혼합보다 넓게 번질 수 있다. R/B가
    # 동시에 G보다 높은 성분만 제거하면 브라스·목재·청록은 보존하면서
    # 보라색 키 잔광만 중성화된다.
    excess = np.clip(
        np.minimum(rgb[:, :, 0], rgb[:, :, 2]) - rgb[:, :, 1], 0.0, None)
    rgb[:, :, 0] -= excess
    rgb[:, :, 2] -= excess
    rgb = np.where(a > 0.0, rgb, 0.0)
    return np.clip(rgb, 0, 255)


def resize_rgba_premultiplied(rgb, alpha, target):
    """투명 픽셀의 RGB가 LANCZOS 필터에 번지지 않도록 premultiplied resize."""
    premul = np.clip(rgb, 0, 255) * alpha[:, :, None] / 255.0
    channels = []
    for channel in range(3):
        plane = Image.fromarray(premul[:, :, channel].astype(np.float32), "F")
        channels.append(np.array(plane.resize(target, Image.LANCZOS), dtype=np.float64))
    alpha_plane = Image.fromarray(alpha.astype(np.float32), "F")
    resized_alpha = np.clip(
        np.array(alpha_plane.resize(target, Image.LANCZOS), dtype=np.float64), 0.0, 1.0)
    resized_premul = np.stack(channels, axis=2)
    denominator = np.maximum(resized_alpha[:, :, None], 1.0 / 255.0)
    resized_rgb = np.where(
        resized_alpha[:, :, None] > 0.0,
        resized_premul * 255.0 / denominator,
        0.0)
    return np.dstack([
        np.clip(resized_rgb, 0, 255).astype(np.uint8),
        (resized_alpha * 255.0).astype(np.uint8)])


def find_fiducial_interior(arr):
    """초록 기준틀(#00FF00) 안쪽 영역을 찾는다. 없으면 None."""
    r = arr[:, :, 0].astype(np.int32)
    g = arr[:, :, 1].astype(np.int32)
    b = arr[:, :, 2].astype(np.int32)
    green = (g > 190) & (r < 90) & (b < 90)
    if green.sum() < 500:
        return None
    ys, xs = np.where(green)
    x0, x1, y0, y1 = xs.min(), xs.max() + 1, ys.min(), ys.max() + 1
    # 틀 두께만큼 가장자리에서 안쪽으로 조인다. 좌우 프레임 기둥 때문에
    # any() 기준은 전부 소진되므로, 행/열의 초록 '비율'로 띠를 판정한다.
    sub = green[y0:y1, x0:x1]
    # ImageGen may anti-alias the outermost edge or scale the guide border, so the
    # first/last bbox row is not guaranteed to be a majority-green row. Locate the
    # two actual horizontal/vertical fiducial bands and crop past their inner edges.
    row_frac = sub.mean(axis=1)
    strong_rows = np.where(row_frac > 0.5)[0]
    top_rows = strong_rows[strong_rows < sub.shape[0] // 2]
    bottom_rows = strong_rows[strong_rows >= sub.shape[0] // 2]
    if len(top_rows) == 0 or len(bottom_rows) == 0:
        return None
    top = int(top_rows.max()) + 1
    bottom = int(bottom_rows.min())

    col_frac = sub[top:bottom].mean(axis=0)
    strong_cols = np.where(col_frac > 0.5)[0]
    left_cols = strong_cols[strong_cols < sub.shape[1] // 2]
    right_cols = strong_cols[strong_cols >= sub.shape[1] // 2]
    if len(left_cols) == 0 or len(right_cols) == 0:
        return None
    left = int(left_cols.max()) + 1
    right = int(right_cols.min())
    if right - left < 16 or bottom - top < 8:
        return None
    # 잔여 초록 픽셀(안티앨리어싱)을 피해 2px 더 조인다.
    return (x0 + left + 2, y0 + top + 2, x0 + right - 2, y0 + bottom - 2)


def process(part, src_path, target):
    im = Image.open(src_path).convert("RGB")
    arr = np.array(im)
    tw, th = target
    dst_aspect = tw / th
    note = None

    interior = find_fiducial_interior(arr)
    if interior is not None:
        # 기준틀 모드: 틀 안쪽을 그대로 목표 규격으로 리샘플 → 종횡비 기하학적 보장.
        ix0, iy0, ix1, iy1 = interior
        arr = arr[iy0:iy1, ix0:ix1]
        alpha, _ = key_to_alpha(arr)
        if (alpha > 0.01).mean() < 0.05:
            return None, "기준틀 안이 비어 있음 (마젠타뿐)"
        ys, xs = np.where(alpha > 0.01)
        fill_x = (xs.max() + 1 - xs.min()) / arr.shape[1]
        fill_y = (ys.max() + 1 - ys.min()) / arr.shape[0]
        if min(fill_x, fill_y) < 0.80:
            return None, (f"파츠가 기준틀을 가득 채우지 않음 "
                          f"(가로 {fill_x:.0%}, 세로 {fill_y:.0%}) — edge-to-edge로 재생성")
        if min(fill_x, fill_y) < 0.90:
            note = f"채움 비율 낮음 (가로 {fill_x:.0%}, 세로 {fill_y:.0%})"
        rgb = despill(arr.astype(np.float64), alpha)
        a8 = (alpha * 255).astype(np.uint8)
    else:
        # 레거시 모드: 기준틀 없이 생성된 경우 — 내용 bbox 트림 + 종횡비 검사.
        alpha, _ = key_to_alpha(arr)
        key_ratio = float((alpha <= 0.01).mean())
        if key_ratio < 0.02:
            return None, (f"키 컬러(#FF00FF) 배경이 감지되지 않음 ({key_ratio:.1%}) "
                          "— 가이드 템플릿 기반으로 재생성 필요")
        ys, xs = np.where(alpha > 0.01)
        if len(xs) == 0:
            return None, "내용이 비어 있음"
        x0, x1, y0, y1 = xs.min(), xs.max() + 1, ys.min(), ys.max() + 1
        src_aspect = (x1 - x0) / (y1 - y0)
        aspect_err = abs(src_aspect - dst_aspect) / dst_aspect
        if aspect_err > ASPECT_WARN:
            return None, (f"기준틀 없음 + 종횡비 오차 {aspect_err:.0%} (>{ASPECT_WARN:.0%}) "
                          "— 가이드 템플릿 기반으로 재생성 필요")
        rgb = despill(arr[y0:y1, x0:x1].astype(np.float64), alpha[y0:y1, x0:x1])
        a8 = (alpha[y0:y1, x0:x1] * 255).astype(np.uint8)
        note = "기준틀 미사용(레거시 트림 모드)"

    rgba = resize_rgba_premultiplied(rgb, a8.astype(np.float64) / 255.0, (tw, th))
    out = Image.fromarray(rgba, "RGBA")
    return out, note


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
            continue  # 아직 생성 안 된 파츠는 건너뜀 (필수 여부는 리포트에서 판단)
        out, note = process(part, os.path.join(RAW_DIR, matches[0]), target)
        if out is None:
            results.append((part, "FAIL", note))
            continue
        name = f"reward_v12_{part}_{target[0]}x{target[1]}.png"
        path = os.path.join(OUT_DIR, name)
        out.save(path)

        a = np.array(out)[:, :, 3]
        checks = []
        if part not in ("modal_background",) and a.min() > 0:
            checks.append("외곽 투명 영역 없음(알파 min>0)")
        digest = hashlib.md5(open(path, "rb").read()).hexdigest()
        if digest in hashes:
            checks.append(f"중복: {hashes[digest]}와 동일 파일")
        hashes[digest] = name
        if note:
            checks.append(note)
        results.append((part, "FAIL" if checks else "PASS", "; ".join(checks) or name))

    width = max(len(p) for p, _, _ in results) if results else 10
    fails = 0
    for part, status, msg in results:
        fails += status == "FAIL"
        print(f"{part:<{width}}  {status}  {msg}")
    missing = [p for p in list(MANIFEST)[:11] if not any(r[0] == p for r in results)]
    if missing:
        print(f"\n미생성(필수 후보): {', '.join(missing)}")
    print(f"\n{len(results)}개 처리, FAIL {fails}건 -> {OUT_DIR}")
    sys.exit(1 if fails else 0)


if __name__ == "__main__":
    main()
