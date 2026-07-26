"""Assemble real panels from the parts and look at the result.

Numbers say the joints line up; only assembly says whether the seams show.
Everything is composed at 1:1 authoring scale (1U = 128px) so a seam that
exists is visible rather than resampled away.
"""
import os
from PIL import Image

ROOT = r"D:/UnrealProjects/P_RD_develop/Tools/UI/KayKitUIKit/Processed"
def P(n): return Image.open(os.path.join(ROOT, n + ".png")).convert("RGBA")

def heavy_panel(units_w, units_h, fill="KK_Fill_Stone"):
    """A panel units_w x units_h in U, framed with the heavy set. 1U = 128px."""
    U = 128
    w, h = units_w * U, units_h * U
    out = Image.new("RGBA", (w, h), (0, 0, 0, 0))

    tile = P(fill)
    for y in range(0, h, tile.height):
        for x in range(0, w, tile.width):
            out.alpha_composite(tile, (x, y))

    ct, cb = P("KK_HFrame_Corner_T"), P("KK_HFrame_Corner_B")
    lt, lb, ls = P("KK_HFrame_Link_T"), P("KK_HFrame_Link_B"), P("KK_HFrame_Link_S")
    c = ct.width                      # 2U
    # top and bottom runs
    for i in range((w - 2 * c) // U):
        out.alpha_composite(lt, (c + i * U, 0))
        out.alpha_composite(lb, (c + i * U, h - lb.height))
    # side runs
    for i in range((h - 2 * c) // U):
        out.alpha_composite(ls, (0, c + i * U))
        out.alpha_composite(ls.transpose(Image.FLIP_LEFT_RIGHT), (w - ls.width, c + i * U))
    out.alpha_composite(ct, (0, 0))
    out.alpha_composite(ct.transpose(Image.FLIP_LEFT_RIGHT), (w - c, 0))
    out.alpha_composite(cb, (0, h - c))
    out.alpha_composite(cb.transpose(Image.FLIP_LEFT_RIGHT), (w - c, h - c))
    return out

def light_panel(units_w, units_h, fill="KK_Fill_Wood"):
    """Same, with the light set. 1U here is 64px (half-scale pieces)."""
    U = 64
    w, h = units_w * U, units_h * U
    out = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    tile = P(fill)
    for y in range(0, h, tile.height):
        for x in range(0, w, tile.width):
            out.alpha_composite(tile, (x, y))
    ct, cb = P("KK_LFrame_Corner_T"), P("KK_LFrame_Corner_B")
    lt, lb, ls = P("KK_LFrame_Link_T"), P("KK_LFrame_Link_B"), P("KK_LFrame_Link_S")
    c = ct.width
    for i in range((w - 2 * c) // U):
        out.alpha_composite(lt, (c + i * U, 0))
        out.alpha_composite(lb, (c + i * U, h - lb.height))
    for i in range((h - 2 * c) // U):
        out.alpha_composite(ls, (0, c + i * U))
        out.alpha_composite(ls.transpose(Image.FLIP_LEFT_RIGHT), (w - ls.width, c + i * U))
    out.alpha_composite(ct, (0, 0))
    out.alpha_composite(ct.transpose(Image.FLIP_LEFT_RIGHT), (w - c, 0))
    out.alpha_composite(cb, (0, h - c))
    out.alpha_composite(cb.transpose(Image.FLIP_LEFT_RIGHT), (w - c, h - c))
    return out

def select_ring(w_units, h_units):
    U = 32
    w, h = w_units * U, h_units * U
    out = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    c = P("KK_Select_Corner")
    lh, lv = P("KK_Select_Link_H"), P("KK_Select_Link_V")
    cw = c.width
    for i in range((w - 2 * cw) // lh.width):
        out.alpha_composite(lh, (cw + i * lh.width, 0))
        out.alpha_composite(lh.transpose(Image.FLIP_TOP_BOTTOM), (cw + i * lh.width, h - lh.height))
    for i in range((h - 2 * cw) // lv.height):
        out.alpha_composite(lv, (0, cw + i * lv.height))
        out.alpha_composite(lv.transpose(Image.FLIP_LEFT_RIGHT), (w - lv.width, cw + i * lv.height))
    out.alpha_composite(c, (0, 0))
    out.alpha_composite(c.transpose(Image.FLIP_LEFT_RIGHT), (w - cw, 0))
    out.alpha_composite(c.transpose(Image.FLIP_TOP_BOTTOM), (0, h - cw))
    out.alpha_composite(c.transpose(Image.ROTATE_180), (w - cw, h - cw))
    return out

def bar(length_units, filled=0.7):
    cap, link = P("KK_Bar_Cap"), P("KK_Bar_Link")
    tcap, tlink = P("KK_Bar_Track_Cap"), P("KK_Bar_Track_Link")
    U = cap.width
    w = length_units * U
    out = Image.new("RGBA", (w, cap.height), (0, 0, 0, 0))
    for i in range(length_units - 2):
        out.alpha_composite(tlink, ((i + 1) * U, 0))
    out.alpha_composite(tcap, (0, 0))
    out.alpha_composite(tcap.transpose(Image.FLIP_LEFT_RIGHT), (w - U, 0))
    n = max(2, int(length_units * filled))
    for i in range(n - 2):
        out.alpha_composite(link, ((i + 1) * U, 0))
    out.alpha_composite(cap, (0, 0))
    out.alpha_composite(cap.transpose(Image.FLIP_LEFT_RIGHT), ((n - 1) * U, 0))
    return out

sheet = Image.new("RGBA", (2100, 1500), (26, 26, 32, 255))
sheet.alpha_composite(heavy_panel(10, 6), (40, 40))
sheet.alpha_composite(light_panel(12, 8), (40, 860))
sheet.alpha_composite(select_ring(20, 12), (860, 900))
sheet.alpha_composite(bar(14, 0.7), (860, 1320))
sheet.alpha_composite(P("KK_Ring_Portrait").resize((256, 256)), (1400, 40))
sheet.alpha_composite(P("KK_Gem_On").resize((96, 96)), (1700, 60))
sheet.alpha_composite(P("KK_Gem_Off").resize((96, 96)), (1810, 60))
sheet.alpha_composite(P("KK_Icon_ShieldBash").resize((180, 180)), (1400, 330))
sheet.alpha_composite(P("KK_Icon_Move").resize((180, 180)), (1600, 330))
sheet.alpha_composite(P("KK_Icon_EndTurn").resize((180, 180)), (1800, 330))
sheet.convert("RGB").save("assembled.png")
print("saved", sheet.size)
