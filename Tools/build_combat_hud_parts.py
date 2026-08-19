from __future__ import annotations

import csv
import json
import math
import shutil
from dataclasses import asdict, dataclass
from pathlib import Path

import numpy as np
from PIL import Image, ImageChops, ImageDraw, ImageEnhance, ImageFilter
from scipy import ndimage


BUNDLE = Path(r"F:\코덱스이미지생성폴더\P_RD_UIParts_20260816_v1")
PROJECT = Path(r"D:\UnrealProjects\P_RD_develop")
SRC = BUNDLE / "src"
REFS = BUNDLE / "refs"
STAGING = PROJECT / "work" / "combat_hud_parts_20260816" / "staging"
PROJECT_OUT = PROJECT / "outputs" / "combat_hud_parts_20260816"
BUNDLE_OUT = BUNDLE / "out"
INPAINT_PANEL = Path(
    r"C:\Users\2009e\.codex\generated_images\01a00603-0e41-74e1-9ff1-a0632e868920"
    r"\exec-5e5c0fd3-393c-42bd-962c-11a7a1651459.png"
)


@dataclass
class Part:
    filename: str
    slot: tuple[int, int]
    method: str
    source: str
    note: str = ""
    frame_center_transparent: bool = False


def rgba(path: Path) -> Image.Image:
    return Image.open(path).convert("RGBA")


def trim(im: Image.Image, threshold: int = 1) -> Image.Image:
    a = im.getchannel("A").point(lambda p: 255 if p >= threshold else 0)
    box = a.getbbox()
    if not box:
        raise ValueError("image has no nontransparent pixels")
    return im.crop(box)


def opaque(im: Image.Image) -> Image.Image:
    out = im.convert("RGBA")
    out.putalpha(Image.new("L", out.size, 255))
    return out


def nine_slice(
    im: Image.Image,
    size: tuple[int, int],
    margins: tuple[int, int, int, int],
) -> Image.Image:
    """Resize only the center/edge spans while preserving the four corners."""
    im = im.convert("RGBA")
    sw, sh = im.size
    dw, dh = size
    l, r, t, b = margins
    l = min(l, sw // 2 - 1, dw // 2 - 1)
    r = min(r, sw - l - 1, dw - l - 1)
    t = min(t, sh // 2 - 1, dh // 2 - 1)
    b = min(b, sh - t - 1, dh - t - 1)
    sx = [0, l, sw - r, sw]
    sy = [0, t, sh - b, sh]
    dx = [0, l, dw - r, dw]
    dy = [0, t, dh - b, dh]
    out = Image.new("RGBA", size)
    for yi in range(3):
        for xi in range(3):
            tile = im.crop((sx[xi], sy[yi], sx[xi + 1], sy[yi + 1]))
            tw = dx[xi + 1] - dx[xi]
            th = dy[yi + 1] - dy[yi]
            if tile.size != (tw, th):
                tile = tile.resize((tw, th), Image.Resampling.LANCZOS)
            out.alpha_composite(tile, (dx[xi], dy[yi]))
    return out


def expand_to_ratio(
    im: Image.Image,
    ratio: float,
    margins: tuple[int, int, int, int] | None = None,
) -> Image.Image:
    w, h = im.size
    if w / h < ratio:
        size = (math.ceil(h * ratio), h)
    else:
        size = (w, math.ceil(w / ratio))
    if size == im.size:
        return im
    if margins is None:
        margins = (max(1, w // 5), max(1, w // 5), max(1, h // 5), max(1, h // 5))
    return nine_slice(im, size, margins)


def aa_shape(size: tuple[int, int], draw_fn, scale: int = 4) -> Image.Image:
    hi = Image.new("L", (size[0] * scale, size[1] * scale), 0)
    d = ImageDraw.Draw(hi)
    draw_fn(d, scale)
    return hi.resize(size, Image.Resampling.LANCZOS)


def punch_rounded(im: Image.Image, box: tuple[int, int, int, int], radius: int) -> Image.Image:
    mask = aa_shape(
        im.size,
        lambda d, s: d.rounded_rectangle(tuple(v * s for v in box), radius=radius * s, fill=255),
    )
    out = im.copy()
    out.putalpha(ImageChops.multiply(out.getchannel("A"), ImageChops.invert(mask)))
    return out


def chamfer_points(box: tuple[int, int, int, int], c: int) -> list[tuple[int, int]]:
    x0, y0, x1, y1 = box
    return [(x0 + c, y0), (x1 - c, y0), (x1, y0 + c), (x1, y1 - c),
            (x1 - c, y1), (x0 + c, y1), (x0, y1 - c), (x0, y0 + c)]


def punch_chamfer(im: Image.Image, box: tuple[int, int, int, int], chamfer: int) -> Image.Image:
    mask = aa_shape(
        im.size,
        lambda d, s: d.polygon([(x * s, y * s) for x, y in chamfer_points(box, chamfer)], fill=255),
    )
    out = im.copy()
    out.putalpha(ImageChops.multiply(out.getchannel("A"), ImageChops.invert(mask)))
    return out


def keep_chamfer_ring(
    im: Image.Image,
    outer: tuple[int, int, int, int],
    inner: tuple[int, int, int, int],
    outer_c: int,
    inner_c: int,
) -> Image.Image:
    outer_m = aa_shape(
        im.size,
        lambda d, s: d.polygon([(x * s, y * s) for x, y in chamfer_points(outer, outer_c)], fill=255),
    )
    inner_m = aa_shape(
        im.size,
        lambda d, s: d.polygon([(x * s, y * s) for x, y in chamfer_points(inner, inner_c)], fill=255),
    )
    ring = ImageChops.multiply(outer_m, ImageChops.invert(inner_m))
    out = im.copy()
    out.putalpha(ImageChops.multiply(out.getchannel("A"), ring))
    return trim(out)


def keep_chamfer_plate(im: Image.Image, chamfer: int) -> Image.Image:
    w, h = im.size
    mask = aa_shape(
        im.size,
        lambda d, s: d.polygon(
            [(x * s, y * s) for x, y in chamfer_points((0, 0, w - 1, h - 1), chamfer)],
            fill=255,
        ),
    )
    out = im.copy()
    out.putalpha(ImageChops.multiply(out.getchannel("A"), mask))
    return trim(out)


def close_ring_with_texture(im: Image.Image, ring_alpha: Image.Image) -> Image.Image:
    """Make a geometrically closed ring, filling only alpha gaps from nearest source texels."""
    arr = np.array(im.convert("RGBA"))
    valid = arr[..., 3] > 8
    target = np.asarray(ring_alpha) > 0
    missing = target & ~valid
    if missing.any():
        nearest = ndimage.distance_transform_edt(~valid, return_distances=False, return_indices=True)
        arr[missing, :3] = arr[nearest[0][missing], nearest[1][missing], :3]
    arr[..., 3] = np.asarray(ring_alpha)
    return Image.fromarray(arr, "RGBA")


def closed_chamfer_ring(
    im: Image.Image,
    outer: tuple[int, int, int, int],
    inner: tuple[int, int, int, int],
    outer_c: int,
    inner_c: int,
) -> Image.Image:
    outer_m = aa_shape(
        im.size,
        lambda d, s: d.polygon([(x * s, y * s) for x, y in chamfer_points(outer, outer_c)], fill=255),
    )
    inner_m = aa_shape(
        im.size,
        lambda d, s: d.polygon([(x * s, y * s) for x, y in chamfer_points(inner, inner_c)], fill=255),
    )
    return close_ring_with_texture(im, ImageChops.multiply(outer_m, ImageChops.invert(inner_m)))


def closed_rounded_ring(
    im: Image.Image,
    outer: tuple[int, int, int, int],
    inner: tuple[int, int, int, int],
    outer_r: int,
    inner_r: int,
) -> Image.Image:
    outer_m = aa_shape(
        im.size,
        lambda d, s: d.rounded_rectangle(tuple(v * s for v in outer), radius=outer_r * s, fill=255),
    )
    inner_m = aa_shape(
        im.size,
        lambda d, s: d.rounded_rectangle(tuple(v * s for v in inner), radius=inner_r * s, fill=255),
    )
    return close_ring_with_texture(im, ImageChops.multiply(outer_m, ImageChops.invert(inner_m)))


def closed_chamfer_window(
    im: Image.Image,
    inner: tuple[int, int, int, int],
    chamfer: int,
    barrier_width: int,
) -> Image.Image:
    x0, y0, x1, y1 = inner
    outer = (max(0, x0 - barrier_width), max(0, y0 - barrier_width),
             min(im.width - 1, x1 + barrier_width), min(im.height - 1, y1 + barrier_width))
    outer_m = aa_shape(
        im.size,
        lambda d, s: d.polygon([(x * s, y * s) for x, y in chamfer_points(outer, chamfer + barrier_width)], fill=255),
    )
    inner_m = aa_shape(
        im.size,
        lambda d, s: d.polygon([(x * s, y * s) for x, y in chamfer_points(inner, chamfer)], fill=255),
    )
    barrier = ImageChops.multiply(outer_m, ImageChops.invert(inner_m))
    alpha = ImageChops.lighter(im.getchannel("A"), barrier)
    alpha = ImageChops.multiply(alpha, ImageChops.invert(inner_m))
    return close_ring_with_texture(im, alpha)


def closed_rounded_window(
    im: Image.Image,
    inner: tuple[int, int, int, int],
    radius: int,
    barrier_width: int,
) -> Image.Image:
    x0, y0, x1, y1 = inner
    outer = (max(0, x0 - barrier_width), max(0, y0 - barrier_width),
             min(im.width - 1, x1 + barrier_width), min(im.height - 1, y1 + barrier_width))
    outer_m = aa_shape(
        im.size,
        lambda d, s: d.rounded_rectangle(tuple(v * s for v in outer),
                                          radius=(radius + barrier_width) * s, fill=255),
    )
    inner_m = aa_shape(
        im.size,
        lambda d, s: d.rounded_rectangle(tuple(v * s for v in inner), radius=radius * s, fill=255),
    )
    barrier = ImageChops.multiply(outer_m, ImageChops.invert(inner_m))
    alpha = ImageChops.lighter(im.getchannel("A"), barrier)
    alpha = ImageChops.multiply(alpha, ImageChops.invert(inner_m))
    return close_ring_with_texture(im, alpha)


def keep_color_neighborhood(im: Image.Image, selector, dilation: int) -> Image.Image:
    arr = np.array(im.convert("RGBA"))
    selected = selector(arr[..., :3]).astype(np.uint8) * 255
    size = dilation if dilation % 2 == 1 else dilation + 1
    mask = Image.fromarray(selected, "L").filter(ImageFilter.MaxFilter(size))
    out = im.copy()
    out.putalpha(ImageChops.multiply(out.getchannel("A"), mask))
    return trim(out)


def solid_row_plate(row: Image.Image, title_source: Image.Image) -> Image.Image:
    wood = opaque(title_source.crop((85, 45, title_source.width - 85, title_source.height - 45)))
    wood = nine_slice(wood, row.size, (70, 70, 30, 30))
    w, h = row.size
    inner = aa_shape(
        row.size,
        lambda d, s: d.rounded_rectangle((20 * s, 18 * s, (w - 20) * s, (h - 18) * s),
                                          radius=36 * s, fill=255),
    )
    wood.putalpha(inner)
    wood.alpha_composite(row)
    return wood


def local_panel_inpaint(original: Image.Image, edited: Image.Image) -> Image.Image:
    if edited.size != original.size:
        edited = edited.resize(original.size, Image.Resampling.LANCZOS)
    w, h = original.size
    mask = Image.new("L", original.size, 0)
    d = ImageDraw.Draw(mask)
    d.rounded_rectangle((330, 35, 1205, 230), radius=32, fill=255)
    mask = mask.filter(ImageFilter.GaussianBlur(10))
    out = Image.composite(edited, original, mask)
    # The generated patch is RGB-opaque; retain the source texture's alpha so
    # no black canvas pixels are introduced around the real panel silhouette.
    out.putalpha(original.getchannel("A"))
    return out


def selected_variant(im: Image.Image) -> Image.Image:
    arr = np.array(im).astype(np.float32)
    rgb, a = arr[..., :3], arr[..., 3:4]
    blue = ((rgb[..., 2] > 75) & (rgb[..., 2] > rgb[..., 0] * 1.15) &
            (rgb[..., 1] > rgb[..., 0] * 0.85)).astype(np.uint8) * 255
    glow = np.array(Image.fromarray(blue, "L").filter(ImageFilter.GaussianBlur(7)), dtype=np.float32) / 255.0
    strength = (0.22 * glow[..., None]) * (a / 255.0)
    cyan = np.array([35, 205, 255], dtype=np.float32)
    rgb = np.clip(rgb * (1 - strength) + cyan * strength, 0, 255)
    arr[..., :3] = rgb
    return Image.fromarray(arr.astype(np.uint8), "RGBA")


def disabled_variant(im: Image.Image) -> Image.Image:
    return ImageEnhance.Brightness(ImageEnhance.Color(im).enhance(0.12)).enhance(0.68)


def add(parts: list[tuple[Part, Image.Image]], part: Part, im: Image.Image) -> None:
    ratio = part.slot[0] / part.slot[1]
    # Integer rounding on very small pip textures can move a mathematically exact
    # ratio by about one pixel. Keep results already inside the spec's ±2% band.
    if abs(im.width / im.height / ratio - 1) > 0.02:
        im = expand_to_ratio(im, ratio)
    parts.append((part, im))


def build() -> list[tuple[Part, Image.Image]]:
    p: list[tuple[Part, Image.Image]] = []
    card = rgba(SRC / "T_SkillCard_Frame_Combat.png")
    card_outer = trim(card)
    a1 = punch_chamfer(card_outer, (93, 104, card_outer.width - 93, card_outer.height - 105), 45)
    a1 = expand_to_ratio(a1, 200 / 228, (210, 210, 220, 220))
    add(p, Part("T_MB_Part_A1_CardOuterFrame_20260816_v1.png", (200, 228), "복원 분리", "T_SkillCard_Frame_Combat.png",
                "원본 링 픽셀 유지; 중앙 창 알파 분리", True), a1)
    add(p, Part("T_MB_Part_A1s_CardOuterFrameSelected_20260816_v1.png", (200, 228), "파생", "T_SkillCard_Frame_Combat.png",
                "A1 청색 계열 픽셀에만 국소 발광 강화", True), selected_variant(a1))
    add(p, Part("T_MB_Part_A1d_CardOuterFrameDisabled_20260816_v1.png", (200, 228), "파생", "T_SkillCard_Frame_Combat.png",
                "A1 저채도/저명도 예외 파생", True), disabled_variant(a1))

    a2 = opaque(card.crop((300, 255, 918, 1042)))
    a2 = expand_to_ratio(a2, 176 / 204, (150, 150, 190, 190))
    add(p, Part("T_MB_Part_A2_CardBackground_20260816_v1.png", (176, 204), "복원 분리", "T_SkillCard_Frame_Combat.png",
                "가림 요소가 없는 남색 캔버스 중심부에서 확장"), a2)

    row = trim(rgba(SRC / "T_KitA_Row_Plate.png"))
    title_source = trim(rgba(SRC / "T_KitA_Title_Plate.png"))
    row_solid = solid_row_plate(row, title_source)
    add(p, Part("T_MB_Part_A3_CardNameBand_20260816_v1.png", (172, 41), "대체 재사용", "T_KitA_Row_Plate.png",
                "원본 카드에 이름 띠 없음; 명세 fallback. KitA 목재 중심판으로 불투명 창 구성"),
        expand_to_ratio(row_solid, 172 / 41, (58, 58, 35, 35)))
    add(p, Part("T_MB_Part_A4_CardInfoBand_20260816_v1.png", (172, 36), "대체 재사용", "T_KitA_Row_Plate.png",
                "원본 카드에 정보 띠 없음; 명세 fallback. KitA 목재 중심판으로 불투명 창 구성"),
        expand_to_ratio(row_solid, 172 / 36, (58, 58, 35, 35)))

    socket_src = card.crop((225, 160, 992, 1135))
    a5 = keep_color_neighborhood(
        socket_src,
        lambda c: (c[..., 2] > 105) & (c[..., 2] > c[..., 0] * 1.25) &
                  (c[..., 1] > c[..., 0] * 0.80),
        21,
    )
    a5 = expand_to_ratio(a5, 1.0, (120, 120, 130, 130))
    a5 = closed_chamfer_window(a5, (88, 88, a5.width - 88, a5.height - 88), 55, 14)
    add(p, Part("T_MB_Part_A5_CardIconFrame_20260816_v1.png", (86, 86), "복원 분리", "T_SkillCard_Frame_Combat.png",
                "원본 청색 내부 프레임 질감을 정방형 소켓 링으로 9-slice 재구성", True), a5)

    panel = rgba(SRC / "T_MB_GenericDetailPanel.png")
    edited_panel = rgba(INPAINT_PANEL)
    panel_restored = local_panel_inpaint(panel, edited_panel)
    panel_outer = trim(panel_restored)
    b1 = expand_to_ratio(panel_outer, 600 / 430, (170, 170, 145, 145))
    b1 = closed_chamfer_window(b1, (56, 56, b1.width - 56, b1.height - 56), 34, 16)
    add(p, Part("T_MB_Part_B1_PanelOuterFrame_20260816_v1.png", (600, 430), "복원 분리", "T_MB_GenericDetailPanel.png",
                "상단 이름판 제거 영역에만 imagegen 복원 패치 합성; 그 외 원본 유지", True), b1)

    b2 = opaque(panel.crop((230, 270, 1305, 755)))
    b2 = expand_to_ratio(b2, 552 / 382, (180, 180, 180, 180))
    add(p, Part("T_MB_Part_B2_PanelBackground_20260816_v1.png", (552, 382), "복원 분리", "T_MB_GenericDetailPanel.png",
                "프레임/띠가 없는 중앙 가죽 질감만 9-slice 확장"), b2)

    b3 = keep_chamfer_plate(panel.crop((382, 58, 1154, 220)), 38)
    add(p, Part("T_MB_Part_B3_PanelNameBand_20260816_v1.png", (254, 38), "추출+규격 보정", "T_MB_GenericDetailPanel.png",
                "상단 이름 띠 추출 후 중앙 목재 구간만 가로 확장"),
        expand_to_ratio(b3, 254 / 38, (180, 180, 55, 55)))

    portrait = trim(rgba(SRC / "T_KitA_Portrait_Frame.png"))
    portrait = expand_to_ratio(portrait, 1.0, (58, 58, 58, 58))
    add(p, Part("T_MB_Part_B4_PortraitFrame_20260816_v1.png", (160, 160), "재사용+검사", "T_KitA_Portrait_Frame.png",
                "비대칭 투명 여백 제거 후 정방형 9-slice 보정", True), portrait)

    b5 = keep_chamfer_plate(panel.crop((190, 814, 1346, 918)), 34)
    add(p, Part("T_MB_Part_B5_HPBarBack_20260816_v1.png", (289, 49), "대체 복원", "T_MB_GenericDetailPanel.png",
                "원본에 HP 홈 없음; 하단 음각 트랙 질감 재활용"),
        expand_to_ratio(b5, 289 / 49, (165, 165, 42, 42)))

    ref2 = rgba(next(REFS.glob("REF_02*.png")))
    b6 = opaque(ref2.crop((1366, 220, 1435, 245)))
    add(p, Part("T_MB_Part_B6_HPBarFill_20260816_v1.png", (273, 33), "참조 추출", ref2.filename if hasattr(ref2, "filename") else "REF_02_CombatHUD_현재아트_스타일참조.png",
                "원본 패널 PNG에 붉은 채움 없음; 현재 아트 참조의 실제 HP 채움 픽셀 사용"),
        expand_to_ratio(b6, 273 / 33, (28, 28, 8, 8)))

    for code, name, slot in (("B7", "APPlate", (135, 49)), ("B8", "SpeedPlate", (131, 45)),
                             ("B10", "CritPlate", (190, 58))):
        add(p, Part(f"T_MB_Part_{code}_{name}_20260816_v1.png", slot, "재사용+규격 보정",
                    "T_KitA_Row_Plate.png", "KitA 행 판 모서리와 불투명 목재 중심판을 보존해 슬롯 비율 보정"),
            expand_to_ratio(row_solid, slot[0] / slot[1], (58, 58, 34, 34)))

    status = trim(rgba(SRC / "T_MB_StatusSlot_Frame.png"))
    status = punch_rounded(status, (118, 118, status.width - 118, status.height - 118), status.width // 2)
    add(p, Part("T_MB_Part_B9_StatusSlotFrame_20260816_v1.png", (64, 64), "재사용+검사",
                "T_MB_StatusSlot_Frame.png", "타이트 크롭 및 중앙 원형 창 알파 보정", True),
        expand_to_ratio(status, 1.0, (210, 210, 210, 210)))

    pip_lit = trim(rgba(SRC / "KK_HUD04_ap_pip_lit.png"))
    pip_dim = trim(rgba(SRC / "KK_HUD04_ap_pip_dim.png"))
    add(p, Part("T_MB_Part_B11_APPipOn_20260816_v1.png", (30, 30), "재사용+검사",
                "KK_HUD04_ap_pip_lit.png", "투명 여백 제거 후 정방형 중앙만 보정"),
        expand_to_ratio(pip_lit, 1.0, (7, 7, 7, 7)))
    add(p, Part("T_MB_Part_B11_APPipOff_20260816_v1.png", (30, 30), "재사용+검사",
                "KK_HUD04_ap_pip_dim.png", "투명 여백 제거 후 정방형 중앙만 보정"),
        expand_to_ratio(pip_dim, 1.0, (7, 7, 7, 7)))

    token = rgba(SRC / "T_MB_TurnToken_Frame.png")
    c1 = trim(token)
    c1 = punch_rounded(c1, (88, 125, c1.width - 88, 625), 70)
    c1 = punch_rounded(c1, (94, 674, c1.width - 94, 845), 48)
    c1 = expand_to_ratio(c1, 108 / 144, (125, 125, 155, 155))
    add(p, Part("T_MB_Part_C1_TurnTokenFrame_20260816_v1.png", (108, 144), "재사용+검사", "T_MB_TurnToken_Frame.png",
                "타이트 크롭 후 초상/속도 창 알파 보정; 명세의 비중앙 초상 위치 적용"), c1)

    ring_src = token.crop((78, 103, 653, 708))
    c2 = keep_color_neighborhood(
        ring_src,
        lambda c: (c[..., 0] > 175) & (c[..., 1] > 82) & (c[..., 2] < 105) &
                  (c[..., 0] > c[..., 1] * 1.35),
        21,
    )
    c2 = expand_to_ratio(c2, 85 / 86, (120, 120, 125, 125))
    c2 = closed_rounded_window(c2, (45, 45, c2.width - 45, c2.height - 45), 75, 14)
    add(p, Part("T_MB_Part_C2_TurnCurrentRing_20260816_v1.png", (85, 86), "복원 분리", "T_MB_TurnToken_Frame.png",
                "토큰 상단 황금 강조 링만 분리", True), c2)

    c3 = token.crop((82, 690, 649, 881))
    c3 = keep_chamfer_plate(c3, 38)
    add(p, Part("T_MB_Part_C3_TurnSpeedPlate_20260816_v1.png", (70, 31), "추출+규격 보정", "T_MB_TurnToken_Frame.png",
                "하단 속도 판 추출; 좌우 모서리 보존"),
        expand_to_ratio(c3, 70 / 31, (115, 115, 55, 55)))

    round_badge = trim(rgba(SRC / "T_MB_RoundBadge_Frame.png"))
    add(p, Part("T_MB_Part_C4_RoundBadge_20260816_v1.png", (218, 68), "재사용+검사",
                "T_MB_RoundBadge_Frame.png", "타이트 크롭"),
        expand_to_ratio(round_badge, 218 / 68, (260, 260, 115, 115)))
    c5 = round_badge.crop((10, 0, round_badge.width - 10, round_badge.height))
    add(p, Part("T_MB_Part_C5_RoundDivider_20260816_v1.png", (108, 34), "규격 보정",
                "T_MB_RoundBadge_Frame.png", "가로 띠만 추출하고 코너 보존"),
        expand_to_ratio(c5, 108 / 34, (250, 250, 110, 110)))

    button = trim(rgba(SRC / "T_Combat_Button_Wood_SkillConfirm_20260811_v3.png"))
    button = expand_to_ratio(button, 396 / 181, (220, 220, 200, 200))
    add(p, Part("T_MB_Part_D1_ActionButton_20260816_v1.png", (396, 181), "재사용+검사",
                "T_Combat_Button_Wood_SkillConfirm_20260811_v3.png", "타이트 크롭; 시각 중심 유지"), button)
    add(p, Part("T_MB_Part_D1p_ActionButtonPressed_20260816_v1.png", (396, 181), "파생", "T_Combat_Button_Wood_SkillConfirm_20260811_v3.png",
                "D1 명도 82% 예외 파생"), ImageEnhance.Brightness(button).enhance(0.82))
    add(p, Part("T_MB_Part_D1d_ActionButtonDisabled_20260816_v1.png", (396, 181), "파생", "T_Combat_Button_Wood_SkillConfirm_20260811_v3.png",
                "D1 저채도/저명도 예외 파생"), disabled_variant(button))

    apbar = trim(rgba(SRC / "KK_HUD04_ap_bar_steel.png"))
    add(p, Part("T_MB_Part_D2_TurnAPPlate_20260816_v1.png", (408, 69), "재사용+검사",
                "KK_HUD04_ap_bar_steel.png", "타이트 크롭 후 중앙 강철 구간만 비율 보정"),
        expand_to_ratio(apbar, 408 / 69, (125, 125, 85, 85)))
    add(p, Part("T_MB_Part_D3_TurnAPPipOn_20260816_v1.png", (30, 36), "재사용+검사",
                "KK_HUD04_ap_pip_lit.png", "투명 여백 제거"),
        expand_to_ratio(pip_lit, 30 / 36, (7, 7, 7, 7)))
    add(p, Part("T_MB_Part_D3_TurnAPPipOff_20260816_v1.png", (30, 36), "재사용+검사",
                "KK_HUD04_ap_pip_dim.png", "투명 여백 제거"),
        expand_to_ratio(pip_dim, 30 / 36, (7, 7, 7, 7)))

    rail = trim(rgba(SRC / "T_MB_OptionsRail_Frame.png"))
    windows = [(108, 79, 401, 445), (439, 79, 732, 445),
               (774, 79, 1069, 445), (1105, 79, 1402, 445)]
    for box in windows:
        rail = punch_rounded(rail, box, 28)
    rail = expand_to_ratio(rail, 470 / 173, (145, 145, 90, 90))
    add(p, Part("T_MB_Part_E1_OptionsRailFrame_20260816_v1.png", (470, 173), "복원 분리",
                "T_MB_OptionsRail_Frame.png", "4칸 소켓 내부를 알파 창으로 분리"), rail)

    for state in ("Normal", "Selected"):
        hire = trim(rgba(SRC / f"T_MB_HireRow{state}.png"))
        hire = expand_to_ratio(hire, 375 / 156, (130, 130, 76, 76))
        add(p, Part(f"T_MB_Part_F1_HireRowPlate{state}_20260816_v1.png", (375, 156), "규격 보정",
                    f"T_MB_HireRow{state}.png", "상하 테두리 고정; 중앙 몸통만 121→156 비례로 세로 확장"), hire)

    title = trim(rgba(SRC / "T_KitA_Title_Plate.png"))
    add(p, Part("T_MB_Part_F2_MercenaryNamePlate_20260816_v1.png", (565, 92), "재사용+규격 보정",
                "T_KitA_Title_Plate.png", "기존 판재 모서리 보존; 중앙 목재만 슬롯 비율 확장"),
        expand_to_ratio(title, 565 / 92, (150, 150, 55, 55)))
    return p


def validate_and_stage(parts: list[tuple[Part, Image.Image]]) -> list[dict]:
    if STAGING.exists():
        shutil.rmtree(STAGING)
    STAGING.mkdir(parents=True)
    rows: list[dict] = []
    for spec, im in parts:
        path = STAGING / spec.filename
        im.save(path, optimize=True)
        a = im.getchannel("A")
        bbox = a.getbbox()
        assert bbox is not None
        bbox_cov = ((bbox[2] - bbox[0]) * (bbox[3] - bbox[1])) / (im.width * im.height)
        target_ratio = spec.slot[0] / spec.slot[1]
        ratio_error = abs(im.width / im.height / target_ratio - 1)
        cx, cy = im.width // 2, im.height // 2
        center_alpha = a.getpixel((cx, cy))
        center_delta_x = ""
        center_delta_y = ""
        center_ok = True
        if spec.frame_center_transparent:
            labels, _ = ndimage.label(np.asarray(a) <= 8)
            label_id = labels[cy, cx]
            if center_alpha > 8 or label_id == 0:
                center_ok = False
            else:
                ys, xs = np.nonzero(labels == label_id)
                center_delta_x = round(float(xs.mean() - (im.width - 1) / 2), 4)
                center_delta_y = round(float(ys.mean() - (im.height - 1) / 2), 4)
                center_ok = abs(center_delta_x) <= 2 and abs(center_delta_y) <= 2
        rows.append({
            **asdict(spec),
            "width": im.width,
            "height": im.height,
            "actual_ratio": round(im.width / im.height, 6),
            "target_ratio": round(target_ratio, 6),
            "ratio_error_pct": round(ratio_error * 100, 4),
            "alpha_bbox_coverage_pct": round(bbox_cov * 100, 4),
            "center_alpha": center_alpha,
            "transparent_center_delta_x": center_delta_x,
            "transparent_center_delta_y": center_delta_y,
            "ratio_pass": ratio_error <= 0.02,
            "alpha_bbox_pass": bbox_cov >= 0.98,
            "center_pass": center_ok,
        })
    return rows


def publish(rows: list[dict]) -> None:
    for dest in (BUNDLE_OUT, PROJECT_OUT):
        dest.mkdir(parents=True, exist_ok=True)
        collisions = [p.name for p in STAGING.glob("*.png") if (dest / p.name).exists()]
        if collisions:
            raise FileExistsError(f"refusing to overwrite in {dest}: {collisions[:3]}")
        for src in STAGING.glob("*.png"):
            shutil.copy2(src, dest / src.name)

    columns = list(rows[0].keys())
    for dest in (BUNDLE_OUT, PROJECT_OUT):
        with (dest / "QA_CombatHUDParts_20260816_v1.csv").open("w", encoding="utf-8-sig", newline="") as f:
            w = csv.DictWriter(f, fieldnames=columns)
            w.writeheader()
            w.writerows(rows)
        with (dest / "MANIFEST_CombatHUDParts_20260816_v1.json").open("w", encoding="utf-8") as f:
            json.dump(rows, f, ensure_ascii=False, indent=2)


if __name__ == "__main__":
    built = build()
    qa = validate_and_stage(built)
    failed = [r for r in qa if not (r["ratio_pass"] and r["alpha_bbox_pass"] and r["center_pass"])]
    print(f"built={len(qa)} failed={len(failed)}")
    for row in failed:
        print(row["filename"], row["ratio_error_pct"], row["alpha_bbox_coverage_pct"], row["center_alpha"])
    if failed:
        raise SystemExit(2)
    publish(qa)
    print(f"published to {BUNDLE_OUT}")
    print(f"mirrored to {PROJECT_OUT}")
