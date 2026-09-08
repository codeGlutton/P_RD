"""Extract object alpha without regenerating original chest color pixels.

Usage: python build_silhouette.py <exported-original-atlas.tga> <output.png>
Requires numpy, Pillow and opencv-python. Coordinates refer to 682x455 cells.
Manual outline guides constrain GrabCut; optical flow moves those guides to
the intervening original frames. Source color is never written or rescaled.
"""
import argparse
import hashlib
import json
from pathlib import Path
import cv2
import numpy as np
from PIL import Image

parser = argparse.ArgumentParser()
parser.add_argument('source', type=Path)
parser.add_argument('output', type=Path)
parser.add_argument('--preview', type=Path)
args = parser.parse_args()
atlas = np.array(Image.open(args.source).convert('RGBA'))
assert atlas.shape == (2730, 4092, 4), atlas.shape
assert hashlib.sha256(atlas.tobytes()).hexdigest() == '64ff43567a9d89cbd0e6570fbbb87a9686ac2c2647c390a5ba22fe3d0c6a1648', 'The original atlas changed; review the outline guides before rebuilding.'
frames = [atlas[i // 6 * 455:(i // 6 + 1) * 455, i % 6 * 682:(i % 6 + 1) * 682] for i in range(33)]
guides = {int(k): np.array(v, np.int32) for k, v in json.loads(Path(__file__).with_name('outline_guides.json').read_text()).items()}
exclusions = json.loads(Path(__file__).with_name('exclusion_guides.json').read_text())
grays = [cv2.cvtColor(f[:, :, :3], cv2.COLOR_RGB2GRAY) for f in frames]
out = np.zeros((2730, 4092), np.uint8)
cv2.setRNGSeed(42)
for i, source in enumerate(frames):
    anchor = min(guides, key=lambda k: abs(k - i))
    polygon = np.zeros((455, 682), np.uint8)
    cv2.fillPoly(polygon, [guides[anchor]], 255)
    if anchor != i:
        # Reverse flow maps this frame's pixels onto the nearest reviewed guide.
        flow = cv2.calcOpticalFlowFarneback(grays[i], grays[anchor], None, .5, 4, 25, 5, 7, 1.5, 0)
        yy, xx = np.mgrid[:455, :682].astype(np.float32)
        polygon = cv2.remap(polygon, xx + flow[:, :, 0], yy + flow[:, :, 1], cv2.INTER_LINEAR)
    if i >= 25:
        # Detached lock: retain its dark metal, not only the bright gold pixels.
        lock = np.array([[434,367],[446,356],[473,350],[492,349],[523,353],
                         [537,363],[535,378],[511,385],[477,393],[446,389],[431,382]], np.int32)
        cv2.fillPoly(polygon, [lock], 255)
    core = (polygon > 127).astype(np.uint8)
    inside = cv2.distanceTransform(core, cv2.DIST_L2, 5)
    outside = cv2.distanceTransform(1 - core, cv2.DIST_L2, 5)
    labels = np.where(core, cv2.GC_PR_FGD, cv2.GC_PR_BGD).astype(np.uint8)
    labels[inside > 5] = cv2.GC_FGD
    labels[outside > 3] = cv2.GC_BGD
    cv2.grabCut(source[:, :, :3].copy(), labels, None, np.zeros((1, 65)), np.zeros((1, 65)), 3, cv2.GC_INIT_WITH_MASK)
    body = ((labels == cv2.GC_FGD) | (labels == cv2.GC_PR_FGD)).astype(np.uint8)
    # Bright physical coins below the box are separate objects, not its glow.
    # Light-ray frames use tighter guide bounds; their external glow is omitted.
    rgb = source[:, :, :3]
    gold = (rgb[:, :, 0] > 175) & (rgb[:, :, 1] > 115) & (rgb[:, :, 0] > rgb[:, :, 2] * 1.20)
    yy, xx = np.mgrid[:455, :682]
    coins = gold & (yy > (230 if i >= 18 else 305)) if i >= 16 else np.zeros((455, 682), bool)
    if i < 18:
        coins &= (xx > 135) & (xx < 550) & (yy < 374)
        # Strong baked rays beneath the body cannot be distinguished by color.
        coins &= cv2.dilate(body, np.ones((13, 13), np.uint8)).astype(bool)
    count, regions, stats, _ = cv2.connectedComponentsWithStats(coins.astype(np.uint8))
    selected = np.zeros_like(body)
    for region in range(1, count):
        x, y, width, height, area = stats[region]
        if area >= 7 and (i >= 18 or width < 80 or area > 600):
            selected[regions == region] = 1
    combined = np.maximum(body, selected)
    # Fill the interior of physical coin piles without including exterior haze.
    combined = cv2.morphologyEx(combined, cv2.MORPH_CLOSE, np.ones((3, 3), np.uint8))
    for points in exclusions.get(str(i), []):
        cv2.fillPoly(combined, [np.array(points, np.int32)], 0)
    alpha = cv2.GaussianBlur(combined.astype(np.float32), (3, 3), .5)
    alpha[:3] = alpha[-3:] = 0
    alpha[:, :3] = alpha[:, -3:] = 0
    final = np.round(alpha * 255).astype(np.uint8)
    out[i // 6 * 455:(i // 6 + 1) * 455, i % 6 * 682:(i % 6 + 1) * 682] = final
    if args.preview:
        args.preview.mkdir(parents=True, exist_ok=True)
        a = alpha[:, :, None]
        preview = np.round(rgb * a + 160 * (1 - a)).astype(np.uint8)
        Image.fromarray(preview).save(args.preview / f'frame-{i:02d}.png')
args.output.parent.mkdir(parents=True, exist_ok=True)
Image.fromarray(out).save(args.output)
print(f'Wrote {args.output}: {out.shape[1]}x{out.shape[0]}, 33 original-size masks')
