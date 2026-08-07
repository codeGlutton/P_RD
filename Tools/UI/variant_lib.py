"""Shared widget-painting helpers and the five layout templates.

화면마다 담을 내용(content)은 build_screen_variants.py 가 정의하고, 여기서는
그것을 다섯 가지 배치로 그린다. 다섯 안은 기존 uionly 시안의 관례를 따른다.

    v01 안전안   완성된 화면(용병 선택 / 전투 HUD)의 좌우 2단 구조
    v02 재배치   위쪽 요약 띠 + 아래 2단
    v03 야간     큰 아트 왼쪽 + 어두운 남색 톤
    v04 미니멀   장식 프레임 없이 여백과 글자 크기로 위계
    v05 실험     탭 + 카드 + 오른쪽 섹션

템플릿은 없는 항목을 그냥 건너뛴다. 그래서 같은 템플릿이 열 화면 모두에 쓰인다.
"""

import unreal


UIROOT = "/Game/SVN/OutSideAsset/AICreation/UI"
W, H = 1920.0, 1080.0

SLOT_SQUARE = f"{UIROOT}/Marchbound/Combat/T_MB_StatusSlot_Frame"
ARTIFACT_SLOT = f"{UIROOT}/Marchbound/Combat/T_MB_ArtifactSlot_Frame"
ACTION_SLOT = f"{UIROOT}/Marchbound/Common/T_MB_ActionButtonFrame"
PORTRAIT_FRAME = f"{UIROOT}/Marchbound/RewardSettlement/T_RS_PortraitFrame"
TITLE_PLATE = f"{UIROOT}/Marchbound/Hire/T_MB_HireTitlePlate"
BACK_BUTTON = f"{UIROOT}/Marchbound/Hire/T_MB_HireBackButton"
SKILL_CARD = f"{UIROOT}/Marchbound/Combat/T_SkillCard_Frame_Combat"
ROW_PLATE = f"{UIROOT}/Marchbound/Hire/T_MB_HireRowNormal"


def rgba(r, g, b, a=1.0):
    return unreal.LinearColor(r, g, b, a)


MOVE_BAND = rgba(0.28, 0.60, 0.95, 0.95)
ATTACK_FILL = rgba(0.90, 0.32, 0.30, 0.95)
CASTER_MARK = rgba(0.98, 0.80, 0.35, 0.95)
EMPTY_CELL = rgba(0.10, 0.085, 0.065, 0.55)
OUTLINE = rgba(0.02, 0.01, 0.0, 0.95)

THEMES = {
    "safe": dict(base=f"{UIROOT}/Marchbound/MonsterTab/T_MT_BaseFrame",
                 well=rgba(0.05, 0.035, 0.02, 0.62), text=rgba(1.0, 0.94, 0.82),
                 sub=rgba(0.86, 0.80, 0.68), accent=rgba(0.95, 0.78, 0.42), ornate=True),
    "shift": dict(base=f"{UIROOT}/Marchbound/MonsterTab/T_MT_BaseFrame",
                  well=rgba(0.05, 0.035, 0.02, 0.62), text=rgba(1.0, 0.94, 0.82),
                  sub=rgba(0.86, 0.80, 0.68), accent=rgba(0.95, 0.78, 0.42), ornate=True),
    "night": dict(base=f"{UIROOT}/Marchbound/MonsterTab/T_MT_BaseFrame",
                  well=rgba(0.02, 0.035, 0.07, 0.82), text=rgba(0.88, 0.94, 1.0),
                  sub=rgba(0.62, 0.74, 0.88), accent=rgba(0.40, 0.82, 1.0), ornate=True),
    "minimal": dict(base=None, well=rgba(0.07, 0.07, 0.08, 0.72), text=rgba(0.96, 0.96, 0.95),
                    sub=rgba(0.70, 0.72, 0.75), accent=rgba(0.98, 0.72, 0.30), ornate=False),
    "lab": dict(base=f"{UIROOT}/Marchbound/Combat/T_MercenaryRoster_Shell",
                well=rgba(0.04, 0.05, 0.04, 0.72), text=rgba(1.0, 0.96, 0.88),
                sub=rgba(0.80, 0.84, 0.76), accent=rgba(0.62, 0.92, 0.55), ornate=True),
}

TEXTURE_PATHS = {}
MISSING_TEXTURES = set()


def css(color):
    def channel(value):
        return int(round(max(0.0, min(1.0, float(value))) ** (1.0 / 2.2) * 255))
    return "rgba(%d,%d,%d,%.3f)" % (
        channel(color.r), channel(color.g), channel(color.b), float(color.a))


def load_texture(path):
    if path is None:
        return None
    leaf = path.rsplit("/", 1)[-1]
    asset = unreal.load_object(None, f"{path}.{leaf}")
    if asset is None:
        MISSING_TEXTURES.add(path)
        return None
    TEXTURE_PATHS[asset] = f"{path}.{leaf}"
    return asset


class Painter:
    """한 캔버스 위에 위젯을 놓고, 그린 그대로를 미리보기용으로 기록한다."""

    def __init__(self, tree, canvas, theme, spec):
        self.tree = tree
        self.canvas = canvas
        self.theme = theme
        self.spec = spec
        self.used = {}

    # -- 기본 ------------------------------------------------------------
    def uid(self, name):
        count = self.used.get(name, 0)
        self.used[name] = count + 1
        return name if count == 0 else f"{name}_{count}"

    def record(self, kind, name, pos, box, z, **extra):
        entry = {"name": name, "class": kind,
                 "rect": [float(pos[0]), float(pos[1]), float(box[0]), float(box[1])],
                 "z": int(z)}
        entry.update({k: v for k, v in extra.items() if v is not None})
        self.spec.append(entry)

    def _place(self, child, pos, box, z):
        slot = self.canvas.add_child_to_canvas(child)
        slot.set_anchors(unreal.Anchors(unreal.Vector2D(0.0, 0.0), unreal.Vector2D(0.0, 0.0)))
        slot.set_alignment(unreal.Vector2D(0.0, 0.0))
        slot.set_editor_property("auto_size", False)
        slot.set_position(unreal.Vector2D(pos[0], pos[1]))
        slot.set_size(unreal.Vector2D(box[0], box[1]))
        slot.set_z_order(z)

    def text(self, name, value, size, pos, box, z=12, color=None, align="center", wrap=False):
        name = self.uid(name)
        color = color or self.theme["text"]
        justify = {"left": unreal.TextJustify.LEFT,
                   "right": unreal.TextJustify.RIGHT}.get(align, unreal.TextJustify.CENTER)
        block = unreal.new_object(unreal.TextBlock, outer=self.tree, name=name)
        block.set_text(unreal.Text(str(value)))
        font = block.get_editor_property("font")
        font.set_editor_property("size", size)
        outline = font.get_editor_property("outline_settings")
        outline.set_editor_property("outline_size", 2 if size >= 26 else 1)
        outline.set_editor_property("outline_color", OUTLINE)
        font.set_editor_property("outline_settings", outline)
        block.set_editor_property("font", font)
        block.set_color_and_opacity(unreal.SlateColor(color))
        block.set_editor_property("justification", justify)
        block.set_editor_property("shadow_offset", unreal.Vector2D(1.5, 1.5))
        block.set_editor_property("shadow_color_and_opacity", rgba(0.02, 0.01, 0.0, 0.8))
        block.set_visibility(unreal.SlateVisibility.SELF_HIT_TEST_INVISIBLE)
        if wrap:
            block.set_editor_property("auto_wrap_text", True)
            block.set_editor_property("wrap_text_at", box[0])
        self._place(block, pos, box, z)
        self.record("TextBlock", name, pos, box, z, text=str(value), fontSize=size,
                    color=css(color), justify=align, wrap=wrap)

    def image(self, name, path, pos, box, z=10):
        name = self.uid(name)
        source = load_texture(path)
        image = unreal.new_object(unreal.Image, outer=self.tree, name=name)
        if source is not None:
            image.set_brush_from_texture(source, False)
        image.set_color_and_opacity(rgba(1.0, 1.0, 1.0))
        image.set_visibility(unreal.SlateVisibility.SELF_HIT_TEST_INVISIBLE)
        self._place(image, pos, box, z)
        self.record("Image", name, pos, box, z, texture=TEXTURE_PATHS.get(source))

    def well(self, name, pos, box, z=5, color=None):
        name = self.uid(name)
        color = color or self.theme["well"]
        border = unreal.new_object(unreal.Border, outer=self.tree, name=name)
        border.set_brush_color(color)
        border.set_editor_property("padding", unreal.Margin(0.0, 0.0, 0.0, 0.0))
        border.set_visibility(unreal.SlateVisibility.SELF_HIT_TEST_INVISIBLE)
        self._place(border, pos, box, z)
        self.record("Border", name, pos, box, z, color=css(color))

    def button(self, name, pos, box, z=30):
        name = self.uid(name)
        button = unreal.new_object(unreal.Button, outer=self.tree, name=name)
        style = button.get_editor_property("widget_style")
        empty = unreal.SlateBrush()
        empty.set_editor_property("draw_as", unreal.SlateBrushDrawType.NO_DRAW_TYPE)
        for state in ("normal", "hovered", "pressed", "disabled"):
            style.set_editor_property(state, empty)
        button.set_editor_property("widget_style", style)
        self._place(button, pos, box, z)
        self.record("Button", name, pos, box, z)

    def bar(self, name, pos, box, percent, color, z=11):
        name = self.uid(name)
        widget = unreal.new_object(unreal.ProgressBar, outer=self.tree, name=name)
        widget.set_percent(percent)
        widget.set_fill_color_and_opacity(color)
        widget.set_visibility(unreal.SlateVisibility.SELF_HIT_TEST_INVISIBLE)
        self._place(widget, pos, box, z)
        self.record("ProgressBar", name, pos, box, z, percent=percent, color=css(color))

    # -- 묶음 ------------------------------------------------------------
    def section(self, title, rect, z=5, pad=True):
        """칸 하나를 우물로 깔고 제목을 얹는다. rect 는 프레임이 정한 칸이다."""
        self.well("Section", (rect[0], rect[1]), (rect[2], rect[3]), z)
        if title:
            self.text("SectionHeading", title, 30, (rect[0] + 24.0, rect[1] + 12.0),
                      (rect[2] - 48.0, 42.0), z + 7, self.theme["accent"], "left")

    def header(self, title, frame=None):
        """이름표와 닫기. 다섯 안이 같은 자리에 둬 안을 넘겨 봐도 눈이 안 흔들린다.

        프레임에 제목판이 이미 그려져 있으면 판을 덧그리지 않고 그 자리에 글자만
        얹는다 -- 덧그리면 판이 두 장 겹쳐 보인다.
        """
        painted = (frame or {}).get("titlePlate")
        if painted:
            x, y, w, h = painted
            self.text("TitleText", title, 52, (x, y + h * 0.28), (w, h * 0.5), 12)
        elif self.theme["ornate"]:
            self.image("TitlePlate", TITLE_PLATE, (650.0, 26.0), (620.0, 154.0), 10)
            self.text("TitleText", title, 56, (650.0, 64.0), (620.0, 78.0), 12)
        else:
            self.text("TitleText", title, 54, (70.0, 56.0), (760.0, 78.0), 12, align="left")

        if self.theme["ornate"]:
            self.image("CloseArt", BACK_BUTTON, (1630.0, 36.0), (230.0, 91.0), 10)
        else:
            self.well("CloseWell", (1650.0, 40.0), (200.0, 84.0), 9)
        self.text("CloseLabel", "닫기", 34, (1630.0, 60.0), (230.0, 46.0), 12)
        self.button("CloseButton", (1630.0, 36.0), (230.0, 91.0), 30)

    def chips(self, items, origin, extent, gap=16.0, columns=None, framed=True):
        """수치를 라벨+값 정사각 칩으로. 값 자리가 고정돼 화면을 바꿔도 같은 곳에서 읽힌다."""
        columns = columns or max(1, len(items))
        for index, (label, value) in enumerate(items):
            x = origin[0] + (extent + gap) * (index % columns)
            y = origin[1] + (extent + gap) * (index // columns)
            if framed:
                self.image("Chip", SLOT_SQUARE, (x, y), (extent, extent), 10)
            else:
                self.well("Chip", (x, y), (extent, extent), 10)
            self.text("ChipLabel", label, max(16, int(extent * 0.17)),
                      (x, y + extent * 0.18), (extent, extent * 0.24), 12, self.theme["accent"])
            self.text("ChipValue", value, max(20, int(extent * 0.26)),
                      (x, y + extent * 0.44), (extent, extent * 0.36), 12)

    def rows(self, items, origin, width, height=82.0, gap=10.0, icon=ACTION_SLOT):
        """아이콘 + 이름 + 값 한 줄. item 의 4번째 자리에 아이콘 경로를 넣을 수 있다."""
        for index, item in enumerate(items):
            y = origin[1] + (height + gap) * index
            base = self.theme["well"]
            self.well("Row", (origin[0], y), (width, height), 8,
                      rgba(base.r, base.g, base.b, 0.55))
            if icon:
                slot = height - 18.0
                self.image("RowIcon", icon, (origin[0] + 12.0, y + 9.0), (slot, slot), 10)
                art = item[3] if len(item) > 3 else None
                if art:
                    inset = slot * 0.16
                    self.image("RowArt", art, (origin[0] + 12.0 + inset, y + 9.0 + inset),
                               (slot - inset * 2, slot - inset * 2), 11)
            self.text("RowName", item[0], 30, (origin[0] + 92.0, y + height * 0.2),
                      (width * 0.42, height * 0.6), 12, align="left")
            if len(item) > 1:
                self.text("RowValue", item[1], 26, (origin[0] + width * 0.52, y + height * 0.22),
                          (width * 0.16, height * 0.56), 12, self.theme["accent"], "left")
            if len(item) > 2:
                self.text("RowNote", item[2], 24, (origin[0] + width * 0.70, y + height * 0.22),
                          (width * 0.28, height * 0.56), 12, self.theme["sub"], "left")

    def grid(self, origin, extent, gap, count, pattern, z=10):
        """사거리/영향/위협 범위를 실제 칸으로. 글로만 적으면 머릿속으로 그려야 한다."""
        palette = {"caster": CASTER_MARK, "range": MOVE_BAND, "area": ATTACK_FILL}
        step = extent + gap
        for row in range(count):
            for column in range(count):
                self.well(f"Cell_R{row}C{column}",
                          (origin[0] + step * column, origin[1] + step * row),
                          (extent, extent), z,
                          palette.get(pattern.get((row, column)), EMPTY_CELL))

    def tags(self, items, origin, extent=96.0, gap=18.0, framed=True):
        """상태/효과 태그. item 은 라벨 문자열이거나 (라벨, 아이콘 경로) 다."""
        for index, item in enumerate(items):
            label, art = item if isinstance(item, (tuple, list)) else (item, None)
            x = origin[0] + (extent + gap) * index
            if framed:
                self.image("TagFrame", ARTIFACT_SLOT, (x, origin[1]), (extent, extent), 10)
            else:
                self.well("TagFrame", (x, origin[1]), (extent, extent), 10)
            inset = extent * 0.16
            self.image("TagIcon", art, (x + inset, origin[1] + inset),
                       (extent - inset * 2, extent - inset * 2), 11)
            self.text("TagText", label, 20, (x - 10.0, origin[1] + extent + 4.0),
                      (extent + 20.0, 32.0), 12, self.theme["sub"])

    def panel(self, name, pos, box, z=6):
        """빈 CanvasPanel. 런타임이 자식을 만들어 넣는 자리를 미리 잡아 둘 때 쓴다."""
        name = self.uid(name)
        canvas = unreal.new_object(unreal.CanvasPanel, outer=self.tree, name=name)
        canvas.set_visibility(unreal.SlateVisibility.SELF_HIT_TEST_INVISIBLE)
        self._place(canvas, pos, box, z)
        self.record("CanvasPanel", name, pos, box, z)
        return canvas

    def badge(self, label, art, pos, height=54.0):
        """등급 뱃지 + 글자. 등급은 색만으로는 안 읽혀서 뱃지를 같이 둔다."""
        self.image("GradeBadge", art, pos, (height, height), 12)
        self.text("GradeText", label, 26, (pos[0] + height + 10.0, pos[1] + height * 0.16),
                  (220.0, height * 0.7), 13, self.theme["accent"], "left")

    def hp_bar(self, origin, width, label, percent=1.0, height=44.0):
        self.well("BarTrack", origin, (width, height), 10)
        self.bar("Bar", (origin[0] + 4.0, origin[1] + 4.0), (width - 8.0, height - 8.0),
                 percent, rgba(0.78, 0.20, 0.18))
        self.text("BarText", label, 26, (origin[0], origin[1] + height * 0.16),
                  (width, height * 0.68), 13)

    def hero(self, content, pos, box, frame="portrait"):
        """초상화/아이콘 한 장. 테두리가 화면마다 달라 무엇을 보는지 구분된다."""
        if frame == "portrait":
            frame = PORTRAIT_FRAME
        elif frame == "skill":
            frame = SKILL_CARD
        if frame:
            self.image("HeroFrame", frame, pos, box, 10)
            inset = box[0] * 0.08
            self.image("HeroArt", content.get("art"), (pos[0] + inset, pos[1] + inset),
                       (box[0] - inset * 2, box[1] - inset * 2), 11)
        else:
            self.image("HeroArt", content.get("art"), pos, box, 11)

    def kv(self, pairs, origin, width, gap=50.0, size=26):
        for index, (key, value) in enumerate(pairs):
            y = origin[1] + gap * index
            self.text("KvKey", key, size, (origin[0], y), (width * 0.42, 40.0), 12,
                      self.theme["sub"], "left")
            self.text("KvValue", value, size + 2, (origin[0] + width * 0.45, y),
                      (width * 0.55, 40.0), 12, align="left")

    def lines(self, items, origin, width, gap=50.0, size=26):
        for index, line in enumerate(items):
            self.text("Line", line, size, (origin[0], origin[1] + gap * index),
                      (width, gap - 6.0), 12, align="left")

    def list_panel(self, items, origin, box, selected=0):
        """왼쪽 명단. 용병탭/몬스터탭처럼 고르는 화면에 쓴다."""
        row_height = min(96.0, (box[1] - 24.0) / max(1, len(items)) - 10.0)
        for index, item in enumerate(items):
            y = origin[1] + 12.0 + (row_height + 10.0) * index
            accent = self.theme["accent"]
            base = self.theme["well"]
            color = rgba(accent.r * 0.5, accent.g * 0.45, accent.b * 0.3, 0.75) \
                if index == selected else rgba(base.r, base.g, base.b, 0.55)
            self.well("ListRow", (origin[0] + 12.0, y), (box[0] - 24.0, row_height), 8, color)
            slot = row_height - 16.0
            self.image("ListIcon", ACTION_SLOT, (origin[0] + 24.0, y + 8.0), (slot, slot), 10)
            art = item[2] if len(item) > 2 else None
            if art:
                inset = slot * 0.14
                self.image("ListArt", art, (origin[0] + 24.0 + inset, y + 8.0 + inset),
                           (slot - inset * 2, slot - inset * 2), 11)
            self.text("ListName", item[0], 30, (origin[0] + 40.0 + row_height, y + row_height * 0.14),
                      (box[0] - 80.0 - row_height, row_height * 0.44), 12, align="left")
            if len(item) > 1:
                self.text("ListNote", item[1], 22,
                          (origin[0] + 40.0 + row_height, y + row_height * 0.56),
                          (box[0] - 80.0 - row_height, row_height * 0.36), 12,
                          self.theme["sub"], "left")
            self.button("ListButton", (origin[0] + 12.0, y), (box[0] - 24.0, row_height), 30)


# ------------------------------------------------------------- 패턴 헬퍼
def cross_pattern(count, reach):
    center = count // 2
    result = {(center, center): "caster"}
    for step in range(1, reach + 1):
        for dy, dx in ((-1, 0), (1, 0), (0, -1), (0, 1)):
            result[(center + dy * step, center + dx * step)] = "range"
    return result


def ring_pattern(count, move, attack):
    center = count // 2
    result = {(center, center): "caster"}
    for row in range(count):
        for column in range(count):
            distance = abs(row - center) + abs(column - center)
            if distance == 0:
                continue
            if distance <= attack:
                result[(row, column)] = "area"
            elif distance <= move:
                result[(row, column)] = "range"
    return result
