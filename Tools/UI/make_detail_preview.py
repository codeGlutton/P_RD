"""Turn the detail render spec into a self-contained HTML preview.

Textures come from the WBP editor's PNG export (assets/current-develop), matched
by the texture asset path recorded in workspace.json. Everything is inlined as
data URIs so the file can be opened or shared on its own.

Run with plain python (no Unreal needed):
    python Tools/UI/make_detail_preview.py
"""

from __future__ import annotations

import base64
import html
import json
from pathlib import Path

PROJECT = Path("D:/UnrealProjects/P_RD_develop_20260803")
EDITOR = Path("D:/UnrealProjects_WBP_Editor")
SPEC_PATH = PROJECT / "Saved/LegacyAudit/detail_render_spec.json"
WORKSPACE_PATH = EDITOR / "data/workspace.json"
OUT_PATH = PROJECT / "Saved/LegacyAudit/detail_preview.html"

PANELS = [
    ("WBP_SkillDetail_Marchbound", "스킬 상세", "SkillDetailBaseFrame"),
    ("WBP_EnemyDetail_Marchbound", "적 상세", "EnemyDetailBaseFrame"),
]

# 1920 폭 두 장을 그대로 놓으면 화면 밖으로 나간다. 비율은 지키고 크기만 줄인다.
PREVIEW_SCALE = 0.78


def texture_png_map() -> dict[str, Path]:
    """WBP 편집기가 뽑아 둔 PNG 를 텍스처 에셋 경로로 찾을 수 있게 만든다."""
    workspace = json.loads(WORKSPACE_PATH.read_text(encoding="utf-8"))
    mapping: dict[str, Path] = {}
    for document in workspace.get("documents", []):
        for widget in document.get("widgets", []):
            resource = widget.get("resourcePath")
            image = widget.get("image")
            if resource and image:
                mapping[resource] = EDITOR / image
    return mapping


_URI_CACHE: dict[Path, str | None] = {}


def data_uri(path: Path, box: tuple[float, float]) -> str | None:
    """텍스처를 실제 표시 크기로 줄여 data URI 로 만든다.

    원본은 1254px 짜리가 흔한데 패널에서는 100~900px 로 쓰인다. 원본 그대로
    묻으면 미리보기 한 장이 35MB 가 되어 열기도 버겁다. 같은 텍스처는 한 번만
    인코딩해 재사용한다.
    """
    if path in _URI_CACHE:
        return _URI_CACHE[path]
    if not path.is_file():
        _URI_CACHE[path] = None
        return None

    from PIL import Image

    with Image.open(path) as source:
        image = source.convert("RGBA")
        target = (max(8, int(box[0] * 1.5)), max(8, int(box[1] * 1.5)))
        if image.width > target[0] or image.height > target[1]:
            image.thumbnail(target, Image.LANCZOS)
        buffer = __import__("io").BytesIO()
        image.save(buffer, format="PNG", optimize=True)

    uri = "data:image/png;base64," + base64.b64encode(buffer.getvalue()).decode("ascii")
    _URI_CACHE[path] = uri
    return uri


def render_panel(widgets: list[dict], frame_name: str, textures: dict[str, Path]) -> tuple[str, float, float]:
    frame = next(w for w in widgets if w["name"] == frame_name)
    origin_x, origin_y, width, height = frame["rect"]

    parts = []
    for widget in widgets:
        left, top, box_width, box_height = widget["rect"]
        style = (
            f"left:{left - origin_x:.1f}px;top:{top - origin_y:.1f}px;"
            f"width:{box_width:.1f}px;height:{box_height:.1f}px;z-index:{widget['z']};"
        )
        kind = widget["class"]
        title = html.escape(widget["name"])

        if kind == "Image":
            asset = widget.get("texture")
            uri = (data_uri(textures[asset], (box_width, box_height))
                   if asset and asset in textures else None)
            if uri:
                parts.append(
                    f'<img class="w" style="{style}" src="{uri}" alt="{title}" title="{title}">')
            else:
                # 런타임에 채우는 아이콘 칸. 비어 있는 것이 정상이라 자리만 표시한다.
                parts.append(
                    f'<div class="w slot" style="{style}" title="{title}">'
                    f'<span>{title}</span></div>')
        elif kind == "Border":
            color = widget.get("color") or "rgba(0,0,0,0.5)"
            parts.append(f'<div class="w" style="{style}background:{color};" title="{title}"></div>')
        elif kind == "ProgressBar":
            color = widget.get("color") or "rgba(200,60,50,1)"
            percent = widget.get("percent", 1.0) * 100.0
            parts.append(
                f'<div class="w bar" style="{style}" title="{title}">'
                f'<i style="width:{percent:.0f}%;background:{color};"></i></div>')
        elif kind == "TextBlock":
            color = widget.get("color") or "#fff"
            justify = {"left": "flex-start", "right": "flex-end"}.get(
                widget.get("justify", "center"), "center")
            text = html.escape(widget.get("text", "")).replace("\n", "<br>")
            parts.append(
                f'<div class="w t" style="{style}color:{color};'
                f'font-size:{widget.get("fontSize", 24)}px;justify-content:{justify};" '
                f'title="{title}">{text}</div>')
        elif kind == "Button":
            parts.append(f'<div class="w btn" style="{style}" title="{title}"></div>')

    return "".join(parts), width, height


def main() -> None:
    spec = json.loads(SPEC_PATH.read_text(encoding="utf-8"))
    textures = texture_png_map()

    sections = []
    for asset_name, label, frame_name in PANELS:
        widgets = spec.get(asset_name)
        if not widgets:
            continue
        body, width, height = render_panel(widgets, frame_name, textures)
        sections.append(f"""
    <section>
      <h2>{html.escape(label)} <code>{html.escape(asset_name)}</code></h2>
      <p class="meta">패널 {width:.0f} × {height:.0f} · 위젯 {len(widgets)}개 · 설계 1920×1080 전면</p>
      <div class="frame" style="width:{width * PREVIEW_SCALE:.0f}px;height:{height * PREVIEW_SCALE:.0f}px;">
        <div class="stage" style="width:{width:.0f}px;height:{height:.0f}px;
             transform:scale({PREVIEW_SCALE});">{body}</div>
      </div>
    </section>""")

    document = f"""<!doctype html>
<html lang="ko"><head><meta charset="utf-8">
<title>전투 상세 패널 미리보기</title>
<style>
  body {{ margin:0; padding:28px; background:#141821; color:#dfe4ee;
         font-family:'Malgun Gothic','Segoe UI',sans-serif; }}
  h1 {{ font-size:20px; margin:0 0 6px; }}
  h2 {{ font-size:16px; margin:34px 0 4px; font-weight:600; }}
  code {{ font-size:12px; color:#8fa0bd; font-weight:400; }}
  .meta {{ margin:0 0 12px; font-size:12px; color:#8fa0bd; }}
  .note {{ font-size:13px; color:#9fb0cc; max-width:960px; line-height:1.6; }}
  .frame {{ overflow:hidden; border-radius:6px; box-shadow:0 6px 26px rgba(0,0,0,.5); }}
  .stage {{ position:relative; overflow:hidden; transform-origin:top left;
            background:#0b0e14; }}
  .w {{ position:absolute; }}
  img.w {{ object-fit:fill; }}
  .t {{ display:flex; align-items:center; line-height:1.25;
        text-shadow:0 0 3px rgba(0,0,0,.95), 1.5px 1.5px 0 rgba(0,0,0,.8); }}
  .slot {{ border:1px dashed rgba(150,170,200,.45); border-radius:4px;
           display:flex; align-items:center; justify-content:center; }}
  .slot span {{ font-size:10px; color:rgba(170,190,220,.6); }}
  .btn {{ border:1px dashed rgba(255,190,120,.35); border-radius:4px; }}
  .bar {{ background:rgba(0,0,0,.35); border-radius:3px; overflow:hidden; }}
  .bar i {{ display:block; height:100%; }}
</style></head><body>
<h1>전투 상세 패널 미리보기</h1>
<p class="note">WBP 에서 뽑은 좌표·폰트·색과 추출된 텍스처 PNG 로 그대로 그린 것입니다.
점선 칸은 런타임에 채우는 아이콘/버튼 자리라 비어 있는 것이 정상입니다.</p>
{''.join(sections)}
</body></html>
"""
    OUT_PATH.write_text(document, encoding="utf-8")
    print(f"wrote {OUT_PATH} ({OUT_PATH.stat().st_size // 1024} KB)")


if __name__ == "__main__":
    main()
