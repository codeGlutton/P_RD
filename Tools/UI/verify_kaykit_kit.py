"""Independently check the KayKit parts against what assembly actually needs.

The generator's own QA shares the generator's assumptions. These are the
measurements that decide whether pieces butt together without a seam, so they
get taken again here, from the pixels.
"""
import os
from PIL import Image
import numpy as np

ROOT = r"D:/UnrealProjects/P_RD_develop/Tools/UI/KayKitUIKit/Processed"

def load(name):
    return np.array(Image.open(os.path.join(ROOT, name + ".png")).convert("RGBA"))

def seam(name, axis):
    """How different the two facing edges are. A tile must wrap seamlessly."""
    a = load(name).astype(int)
    if axis == "h":
        left, right = a[:, 0], a[:, -1]
    else:
        left, right = a[0, :], a[-1, :]
    # Compare only where both are opaque; a fully transparent row is a match.
    m = (left[:, 3] > 8) | (right[:, 3] > 8)
    if not m.any():
        return 0.0
    return float(np.abs(left[m] - right[m]).max())

def band(name, edge):
    """Thickness of the opaque run on one edge, in pixels."""
    a = load(name)
    col = {"left": a[:, 0], "right": a[:, -1], "top": a[0, :], "bottom": a[-1, :]}[edge]
    return int((col[:, 3] > 8).sum())

def bbox(name):
    a = load(name)
    ys, xs = np.where(a[:, :, 3] > 8)
    return (int(xs.min()), int(ys.min()), int(xs.max()) + 1, int(ys.max()) + 1)

def size(name):
    return load(name).shape[1], load(name).shape[0]

fails = []
def check(label, ok, detail):
    print("%-46s %s  %s" % (label, "OK " if ok else "FAIL", detail))
    if not ok:
        fails.append(label)

print("=== 1. 캔버스 크기 ===")
EXPECT = {
    "KK_HFrame_Corner_T": (256,256), "KK_HFrame_Corner_B": (256,256),
    "KK_HFrame_Link_T": (128,128), "KK_HFrame_Link_B": (128,128),
    "KK_HFrame_Link_S": (128,128), "KK_HFrame_Joint": (128,128),
    "KK_LFrame_Corner_T": (128,128), "KK_LFrame_Corner_B": (128,128),
    "KK_LFrame_Link_T": (64,64), "KK_LFrame_Link_B": (64,64),
    "KK_LFrame_Link_S": (64,64),
    "KK_Fill_Stone": (512,512), "KK_Fill_Wood": (512,512),
    "KK_Fill_Parchment": (512,512),
    "KK_Select_Corner": (128,128), "KK_Select_Link_H": (64,64),
    "KK_Select_Link_V": (64,64),
    "KK_Bar_Cap": (64,128), "KK_Bar_Link": (64,128),
    "KK_Bar_Track_Cap": (64,128), "KK_Bar_Track_Link": (64,128),
    "KK_Ring_Portrait": (512,512), "KK_Ring_Enemy": (512,512),
    "KK_Gem_On": (128,128), "KK_Gem_Off": (128,128),
    "KK_Badge_Round": (128,128), "KK_Rivet": (64,64), "KK_Bracket": (128,128),
    "KK_Veil_Disabled": (128,128),
}
for n, want in EXPECT.items():
    got = size(n)
    check(n, got == want, "%s (기대 %s)" % (got, want))

print("\n=== 2. 타일 이음매 (마주 보는 끝 픽셀줄 차이, <=2 통과) ===")
for n, ax in [("KK_HFrame_Link_T","h"), ("KK_HFrame_Link_B","h"),
              ("KK_HFrame_Link_S","v"), ("KK_LFrame_Link_T","h"),
              ("KK_LFrame_Link_B","h"), ("KK_LFrame_Link_S","v"),
              ("KK_Select_Link_H","h"), ("KK_Select_Link_V","v"),
              ("KK_Bar_Link","h"), ("KK_Bar_Track_Link","h"),
              ("KK_Fill_Stone","h"), ("KK_Fill_Stone","v"),
              ("KK_Fill_Wood","h"), ("KK_Fill_Wood","v"),
              ("KK_Fill_Parchment","h"), ("KK_Fill_Parchment","v"),
              ("KK_Veil_Disabled","h"), ("KK_Veil_Disabled","v")]:
    d = seam(n, ax)
    check("%s [%s]" % (n, ax), d <= 2, "차이 %.0f" % d)

print("\n=== 3. 이음면 두께 = 캔버스의 1/2 ===")
JOINTS = [
    ("KK_HFrame_Corner_T","right",128), ("KK_HFrame_Corner_T","bottom",128),
    ("KK_HFrame_Corner_B","right",128), ("KK_HFrame_Corner_B","top",128),
    ("KK_HFrame_Link_T","left",64), ("KK_HFrame_Link_T","right",64),
    ("KK_HFrame_Link_S","top",64), ("KK_HFrame_Link_S","bottom",64),
    ("KK_LFrame_Corner_T","right",64), ("KK_LFrame_Corner_T","bottom",64),
    ("KK_LFrame_Link_T","left",32), ("KK_LFrame_Link_T","right",32),
    ("KK_LFrame_Link_S","top",32), ("KK_LFrame_Link_S","bottom",32),
    ("KK_Select_Corner","right",32), ("KK_Select_Corner","bottom",32),
    ("KK_Select_Link_H","left",32), ("KK_Select_Link_H","right",32),
]
for n, e, want in JOINTS:
    got = band(n, e)
    check("%s.%s" % (n, e), abs(got-want) <= 2, "%dpx (기대 %d)" % (got, want))

print("\n=== 4. 젬 On/Off 알파 일치 ===")
on, off = load("KK_Gem_On")[:,:,3] > 8, load("KK_Gem_Off")[:,:,3] > 8
diff = int((on ^ off).sum())
check("Gem alpha", diff <= 64, "다른 픽셀 %d개" % diff)

print("\n=== 5. 링 구멍 지름 = 캔버스의 11/16 ===")
for n in ("KK_Ring_Portrait", "KK_Ring_Enemy"):
    a = load(n)
    mid = a[a.shape[0]//2, :, 3]
    clear = np.where(mid <= 8)[0]
    inner = clear[(clear > 20) & (clear < a.shape[1]-20)]
    d = (inner.max() - inner.min() + 1) if len(inner) else 0
    want = a.shape[1] * 11 // 16
    check(n, abs(d-want) <= 8, "구멍 %dpx (기대 %d)" % (d, want))

print("\n=== 6. 바 4종 세로 위치·높이 일치 ===")
boxes = {n: bbox(n) for n in ("KK_Bar_Cap","KK_Bar_Link","KK_Bar_Track_Cap","KK_Bar_Track_Link")}
tops = {n:(b[1],b[3]) for n,b in boxes.items()}
same = len(set(tops.values())) == 1
check("Bar vertical", same, str(tops))

print("\n=== 7. 아이콘 여백 (가운데 192 안) ===")
for f in sorted(os.listdir(ROOT)):
    if not f.startswith("KK_Icon_"):
        continue
    n = f[:-4]
    x0,y0,x1,y1 = bbox(n)
    ok = x0 >= 30 and y0 >= 30 and x1 <= 226 and y1 <= 226
    check(n, ok, "bbox %s" % ((x0,y0,x1,y1),))

print("\n=== 결과 ===")
print("실패 %d개" % len(fails))
for f in fails:
    print("  " + f)
