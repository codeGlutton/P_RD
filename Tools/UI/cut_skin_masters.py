# -*- coding: utf-8 -*-
"""Cut the skin masters into the part kit.

The masters are the whole HUD drawn empty on magenta. Panel rectangles are
recovered from the chroma itself (seed + rectangle growth), so nobody measures
coordinates by hand -- the drift the generator may have is absorbed here.

Cut policy per part family:
- Frames: corners from the panel's own corners, rails as thin strips stretched
  into the link canvas. The whole strip is the moulding, matching how the
  runtime tiles link art at rail thickness.
- Fills: clean face regions, mirror-padded into a tileable square.
- Button: normal master gives _Up, state master gives _Down.
- Select ring and veil: from the state-vs-normal difference.
- Small props (gems, badges, tags, icons, faces, bars): keep the existing kit.
"""
import io
import os

import numpy as np
from PIL import Image

SRC = r"D:/UnrealProjects/P_RD_develop/Tools/UI/KayKitUIKit/Kit5B/Processed"
OUT = r"D:/UnrealProjects/P_RD_develop/Tools/UI/KayKitUIKit/Processed"
BACKUP = r"D:/UnrealProjects/P_RD_develop/Tools/UI/KayKitUIKit/Kit5B/Backup_PreCut"
os.makedirs(BACKUP, exist_ok=True)

normal = np.array(Image.open(os.path.join(SRC, "KK_SkinMaster_Normal.png")).convert("RGB")).astype(np.int32)
state = np.array(Image.open(os.path.join(SRC, "KK_SkinMaster_State.png")).convert("RGB")).astype(np.int32)
H, W = normal.shape[:2]

def chroma_mask(img):
    r, g, b = img[:, :, 0], img[:, :, 1], img[:, :, 2]
    return ~((r > 235) & (b > 235) & (g < 25))

mask = chroma_mask(normal)

def grow(seed, limit):
    """Grow a rectangle from a seed while the adjacent line still has panel."""
    sx, sy = seed
    lx0, ly0, lx1, ly1 = limit
    x0 = x1 = sx
    y0 = y1 = sy
    changed = True
    while changed:
        changed = False
        if x0 > lx0 and mask[y0:y1 + 1, x0 - 1].mean() > 0.4:
            x0 -= 1; changed = True
        if x1 < lx1 - 1 and mask[y0:y1 + 1, x1 + 1].mean() > 0.4:
            x1 += 1; changed = True
        if y0 > ly0 and mask[y0 - 1, x0:x1 + 1].mean() > 0.4:
            y0 -= 1; changed = True
        if y1 < ly1 - 1 and mask[y1 + 1, x0:x1 + 1].mean() > 0.4:
            y1 += 1; changed = True
    return (x0, y0, x1 + 1, y1 + 1)

# 씨앗은 원판을 보고 찍었고, limit은 이웃 패널로 번지는 것만 막는다.
RECTS = {
    "round":     grow((140, 60),   (0, 0, 400, 140)),
    "objective": grow((1716, 60),  (1450, 0, 1920, 140)),
    "party0":    grow((270, 760),  (0, 690, 545, 833)),
    "party1":    grow((270, 895),  (0, 833, 545, 962)),
    "party2":    grow((270, 1020), (0, 962, 545, 1080)),
    "skill0":    grow((630, 900),  (548, 700, 715, 1080)),
    "skill1":    grow((795, 900),  (715, 700, 880, 1080)),
    "skill4":    grow((1290, 900), (1210, 700, 1375, 1080)),
    "enemy":     grow((1740, 880), (1560, 740, 1920, 1000)),
    "button":    grow((1740, 1040),(1560, 1000, 1920, 1080)),
}
for k, r in RECTS.items():
    print("%-10s %s  %dx%d" % (k, r, r[2] - r[0], r[3] - r[1]))

def crop(img, r):
    # 경계의 안티앨리어싱 헤일로가 3~4px까지 번진다. 4px 인셋.
    return img[r[1] + 4:r[3] - 4, r[0] + 4:r[2] - 4]

def despill(arr):
    """남은 마젠타 픽셀을 이웃 평균으로 치환한다."""
    a = arr.astype(np.float64)
    r, g, b = a[:, :, 0], a[:, :, 1], a[:, :, 2]
    # 헤일로는 경계에서 안쪽으로 그라데이션이라 한 번으로 안 빠진다.
    # 임계를 낮추고 두 번 돌린다. (지난번 실측: b-g=44가 기준 45에 1 차이로
    # 살아남아 화면에 보라 줄로 남았다)
    for _ in range(2):
        r, g, b = a[:, :, 0], a[:, :, 1], a[:, :, 2]
        bad = (r > g + 28) & (b > g + 28)
        if not bad.any():
            break
        blur = a.copy()
        for axis, shift in ((0, 1), (0, -1), (1, 1), (1, -1)):
            blur += np.roll(a, shift, axis=axis)
        blur /= 5.0
        a[bad] = blur[bad]
    return a

def save(name, arr):
    Image.fromarray(arr.astype(np.uint8)).save(os.path.join(OUT, name + ".png"))

def backup(name):
    src = os.path.join(OUT, name + ".png")
    if os.path.exists(src):
        Image.open(src).save(os.path.join(BACKUP, name + ".png"))

def resize(arr, w, h, mode=Image.LANCZOS):
    return np.array(Image.fromarray(arr.astype(np.uint8)).resize((w, h), mode))

# ── 프레임 절단 ───────────────────────────────────────────────────────────────

def cut_frame(rect, img, corner_px, rail_px, prefixes):
    """corners + rails from one panel into the given (light, heavy) prefixes."""
    p = despill(crop(img, rect))
    ph, pw = p.shape[:2]
    c = corner_px
    pieces = {
        "Corner_T": p[0:c, 0:c],
        "Corner_B": p[ph - c:ph, 0:c],
        "Link_T": p[0:rail_px, c:pw - c],
        "Link_B": p[ph - rail_px:ph, c:pw - c],
        "Link_S": p[c:ph - c, 0:rail_px],
    }
    light, heavy = prefixes
    sizes = {"light": {"Corner": 128, "Link": 64}, "heavy": {"Corner": 256, "Link": 128}}
    for tag, arr in pieces.items():
        kind = "Corner" if tag.startswith("Corner") else "Link"
        for label, prefix in (("light", light), ("heavy", heavy)):
            s = sizes[label][kind]
            if kind == "Link":
                if tag == "Link_S":
                    # 세로 레일: 가로로 얇다 -> 세로 타일 축 유지
                    out = resize(arr, s, s)
                else:
                    out = resize(arr, s, s)
            else:
                out = resize(arr, s, s)
            name = "{}_{}".format(prefix, tag)
            backup(name)
            save(name, despill(out))

cut_frame(RECTS["party1"], normal, 34, 12, ("KK_LFrame", "KK_HFrame"))
cut_frame(RECTS["skill1"], normal, 34, 12, ("KK_MLFrame", "KK_MFrame"))
print("프레임 절단 완료")

# ── 면 절단 -> 타일화 ─────────────────────────────────────────────────────────

def tileable(arr, out=512):
    """Mirror-pad a face crop into a seamless square."""
    a = arr.astype(np.float64)
    # 좌우 미러 이어붙이기 -> 상하 미러 -> 축소
    row = np.concatenate([a, a[:, ::-1]], axis=1)
    quad = np.concatenate([row, row[::-1, :]], axis=0)
    img = resize(quad, out, out)
    return img

def inset(rect, l, t, r, b):
    return (rect[0] + l, rect[1] + t, rect[2] - r, rect[3] - b)

def clean_face(rect, img, l, t, r, b):
    return despill(crop(img, inset(rect, l, t, r, b)))

fills = {
    # 파티: 링(왼쪽 ~130px)과 바 줄을 피해서 오른쪽 위 면
    "KK_Fill_Wood": clean_face(RECTS["party1"], normal, 398, 16, 26, 16),
    "KK_Fill_Wood_Active": clean_face(RECTS["party0"], normal, 398, 16, 26, 16),
    # 스킬: 배지(우상단)를 피해 아래쪽 넓은 면
    "KK_Fill_Stone_Skill": clean_face(RECTS["skill1"], normal, 18, 70, 18, 18),
    "KK_Fill_Stone": clean_face(RECTS["skill0"], normal, 18, 70, 18, 18),
    # 적: 링 오른쪽 아래 면
    "KK_Fill_Stone_Enemy": clean_face(RECTS["enemy"], normal, 150, 90, 24, 20),
    # 양피지: 현판 안쪽
    "KK_Fill_Parchment": clean_face(RECTS["round"], normal, 24, 16, 24, 16),
}
for name, arr in fills.items():
    backup(name)
    save(name, tileable(arr))
    print("면 %-24s 원본 %dx%d" % (name, arr.shape[1], arr.shape[0]))

# ── 버튼 (Normal=_Up, State=_Down) ────────────────────────────────────────────

def cut_button(img, suffix):
    r = RECTS["button"]
    p = despill(crop(img, r))
    ph, pw = p.shape[:2]
    c, rail = 30, 12
    outs = {
        "Corner_" + suffix: resize(p[0:c, 0:c], 128, 128),
        "Link_H_" + suffix: resize(p[0:rail, c:pw - c], 64, 64),
        "Link_V_" + suffix: resize(p[c:ph - c, 0:rail], 64, 64),
    }
    for tag, arr in outs.items():
        name = "KK_Button_" + tag
        backup(name)
        save(name, despill(arr))

cut_button(normal, "Up")
cut_button(state, "Down")
# 버튼 얼굴도 면으로 잘라 둔다 -- 턴 종료의 면이 파티 나무를 빌려 쓰면
# 스킨의 주황이 아니라 어두운 갈색이 된다.
face = clean_face(RECTS["button"], normal, 34, 14, 34, 14)
backup("KK_Fill_ButtonWood")
save("KK_Fill_ButtonWood", tileable(face))
print("버튼 절단 완료 (면 포함, 원본 %dx%d)" % (face.shape[1], face.shape[0]))

# ── 상태판에서 선택 테두리와 비활성 덮개 ──────────────────────────────────────

diff = (np.abs(state - normal).max(axis=2) > 12)
print("상태 변화 픽셀:", int(diff.sum()))

# 선택 테두리: 기사 카드 주변의 변화 영역
p0 = RECTS["party0"]
sel_region = (max(0, p0[0] - 12), max(0, p0[1] - 12), p0[2] + 12, p0[3] + 12)
sd = diff[sel_region[1]:sel_region[3], sel_region[0]:sel_region[2]]
ys, xs = np.where(sd)
if len(xs):
    bx0, by0 = xs.min(), ys.min()
    bx1, by1 = xs.max() + 1, ys.max() + 1
    sel = crop(state, (sel_region[0] + bx0, sel_region[1] + by0,
                       sel_region[0] + bx1, sel_region[1] + by1))
    selmask = sd[by0:by1, bx0:bx1]
    sh, sw = sel.shape[:2]
    thick = 18
    def with_alpha(rgb, m):
        out = np.zeros((rgb.shape[0], rgb.shape[1], 4), np.uint8)
        out[:, :, :3] = rgb
        out[:, :, 3] = np.where(m, 255, 0)
        return out
    corner = with_alpha(sel[0:2 * thick, 0:2 * thick], selmask[0:2 * thick, 0:2 * thick])
    link_h = with_alpha(sel[0:thick, sw // 2 - 32:sw // 2 + 32],
                        selmask[0:thick, sw // 2 - 32:sw // 2 + 32])
    link_v = with_alpha(sel[sh // 2 - 32:sh // 2 + 32, 0:thick],
                        selmask[sh // 2 - 32:sh // 2 + 32, 0:thick])
    for name, arr in (("KK_Select_Corner", resize(corner, 128, 128)),
                      ("KK_Select_Link_H", resize(link_h, 64, 64)),
                      ("KK_Select_Link_V", resize(link_v, 64, 64))):
        backup(name)
        save(name, arr)
    print("선택 테두리 절단: 두께 추정 %dpx, 영역 %dx%d" % (thick, sw, sh))

# 비활성 덮개: 다섯 번째 스킬 카드의 State/Normal 색 비율에서 역산
s4 = RECTS["skill4"]
n_face = crop(normal, inset(s4, 20, 70, 20, 20)).reshape(-1, 3).astype(np.float64)
s_face = crop(state, inset(s4, 20, 70, 20, 20)).reshape(-1, 3).astype(np.float64)
# state = normal*(1-a) + veil*a  ->  두 픽셀 그룹(밝은/어두운)으로 a, veil 추정
bright = n_face.mean(axis=1) > np.median(n_face.mean(axis=1))
n1, n2 = n_face[bright].mean(axis=0), n_face[~bright].mean(axis=0)
s1, s2 = s_face[bright].mean(axis=0), s_face[~bright].mean(axis=0)
a = float(np.clip(1.0 - ((s1 - s2) / np.where(n1 - n2 == 0, 1, n1 - n2)).mean(), 0.2, 0.9))
veil = np.clip((s1 - (1 - a) * n1) / a, 0, 255)
tile = np.zeros((128, 128, 4), np.uint8)
tile[:, :, :3] = veil.astype(np.uint8)
tile[:, :, 3] = int(a * 255)
backup("KK_Veil_Disabled")
save("KK_Veil_Disabled", tile)
print("비활성 덮개: 색 (%.0f,%.0f,%.0f) 알파 %.2f" % (*veil, a))

print("절단 전체 완료")
