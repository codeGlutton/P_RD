"""Render the 50 screen variants into one self-contained HTML contact sheet.

같은 텍스처를 위젯마다 다시 묻으면 파일이 수십 MB 가 된다. 텍스처는 CSS 클래스로
한 번만 심고 참조만 반복한다.

Run with plain python (no Unreal needed):
    python Tools/UI/make_variants_preview.py
"""

from __future__ import annotations

import base64
import html
import io
import json
from pathlib import Path

PROJECT = Path("D:/UnrealProjects/P_RD_develop_20260803")
EDITOR = Path("D:/UnrealProjects_WBP_Editor")
SPEC_PATH = PROJECT / "Saved/LegacyAudit/variants_render_spec.json"
WORKSPACE_PATH = EDITOR / "data/workspace.json"
OUT_PATH = PROJECT / "Saved/LegacyAudit/variants_preview.html"

SCALE = 0.30           # 1920 -> 576px. 배치를 견주기에 충분한 크기.
MAX_TEXTURE_EDGE = 520  # 미리보기에서 실제로 쓰이는 최대 폭

SCREEN_LABELS = {
    "Title": "타이틀", "Settings": "설정", "SkillDetail": "스킬 상세",
    "EnemyDetail": "적 상세", "ArtifactDetail": "아티팩트 상세",
    "EnemySummary": "적 요약", "MercenarySummary": "용병 요약",
    "MercenaryTab": "용병탭", "MonsterTab": "몬스터탭", "Map": "지도",
}
VARIANT_LABELS = {"v01": "안전안", "v02": "재배치", "v03": "야간",
                  "v04": "미니멀", "v05": "실험"}


def texture_sources() -> dict[str, Path]:
    """WBP 편집기가 뽑아 둔 PNG 를 텍스처 에셋 경로로 찾을 수 있게 만든다."""
    if not WORKSPACE_PATH.is_file():
        return {}
    workspace = json.loads(WORKSPACE_PATH.read_text(encoding="utf-8"))
    mapping: dict[str, Path] = {}
    for document in workspace.get("documents", []):
        for widget in document.get("widgets", []):
            resource, image = widget.get("resourcePath"), widget.get("image")
            if resource and image:
                mapping[resource] = EDITOR / image
    return mapping


def build_texture_classes(spec: dict, sources: dict[str, Path]) -> tuple[dict[str, str], str, list[str]]:
    """쓰이는 텍스처마다 CSS 클래스를 하나씩 만든다. 같은 그림을 여러 번 묻지 않는다."""
    from PIL import Image

    wanted = {w["texture"] for widgets in spec.values() for w in widgets if w.get("texture")}
    class_of, rules, missing = {}, [], []
    for index, asset in enumerate(sorted(wanted)):
        path = sources.get(asset)
        if path is None or not path.is_file():
            missing.append(asset)
            continue
        with Image.open(path) as source:
            image = source.convert("RGBA")
            if max(image.size) > MAX_TEXTURE_EDGE:
                image.thumbnail((MAX_TEXTURE_EDGE, MAX_TEXTURE_EDGE), Image.LANCZOS)
            buffer = io.BytesIO()
            image.save(buffer, format="PNG", optimize=True)
        uri = "data:image/png;base64," + base64.b64encode(buffer.getvalue()).decode("ascii")
        name = f"t{index}"
        class_of[asset] = name
        rules.append(f".{name}{{background-image:url({uri})}}")
    return class_of, "\n".join(rules), missing


def render(widgets: list[dict], class_of: dict[str, str]) -> str:
    parts = []
    for widget in sorted(widgets, key=lambda item: item["z"]):
        left, top, width, height = widget["rect"]
        style = (f"left:{left:.0f}px;top:{top:.0f}px;"
                 f"width:{width:.0f}px;height:{height:.0f}px;z-index:{widget['z']};")
        kind = widget["class"]
        if kind == "Image":
            css_class = class_of.get(widget.get("texture") or "")
            if css_class:
                parts.append(f'<div class="w tex {css_class}" style="{style}"></div>')
            else:
                parts.append(f'<div class="w slot" style="{style}"></div>')
        elif kind == "Border":
            parts.append(f'<div class="w" style="{style}background:{widget.get("color", "#0008")};"></div>')
        elif kind == "ProgressBar":
            percent = widget.get("percent", 1.0) * 100.0
            parts.append(f'<div class="w bar" style="{style}">'
                         f'<i style="width:{percent:.0f}%;background:{widget.get("color", "#c33")};"></i></div>')
        elif kind == "TextBlock":
            align = {"left": "flex-start", "right": "flex-end"}.get(widget.get("justify"), "center")
            text = html.escape(widget.get("text", "")).replace("\n", "<br>")
            parts.append(f'<div class="w t" style="{style}color:{widget.get("color", "#fff")};'
                         f'font-size:{widget.get("fontSize", 24)}px;justify-content:{align};">{text}</div>')
        elif kind == "Button":
            parts.append(f'<div class="w btn" style="{style}"></div>')
    return "".join(parts)


def main() -> None:
    spec = json.loads(SPEC_PATH.read_text(encoding="utf-8"))
    class_of, css_rules, missing = build_texture_classes(spec, texture_sources())

    screens: dict[str, list[tuple[str, str]]] = {}
    for asset_name in spec:
        _, screen, variant = asset_name.split("_", 2)
        screens.setdefault(screen, []).append((variant, asset_name))

    sections = []
    for screen, variants in screens.items():
        cards = []
        for variant, asset_name in sorted(variants):
            body = render(spec[asset_name], class_of)
            cards.append(f"""
        <figure>
          <div class="frame" style="width:{1920 * SCALE:.0f}px;height:{1080 * SCALE:.0f}px;">
            <div class="stage" style="transform:scale({SCALE});">{body}</div>
          </div>
          <figcaption><b>{variant}</b> {html.escape(VARIANT_LABELS.get(variant, ''))}
            <span>{html.escape(asset_name)}</span></figcaption>
        </figure>""")
        sections.append(f"""
    <section>
      <h2>{html.escape(SCREEN_LABELS.get(screen, screen))}
        <code>/Game/UI/Concepts/{html.escape(screen)}/</code></h2>
      <div class="row">{''.join(cards)}</div>
    </section>""")

    gap_note = ""
    if missing:
        items = "".join(f"<li>{html.escape(path)}</li>" for path in sorted(missing))
        gap_note = ("<p class='note'>아래 텍스처는 PNG 추출본이 없어 미리보기에서만 "
                    f"빈 칸으로 보입니다(WBP 에는 정상 적용됨).</p><ul class='gap'>{items}</ul>")

    document = f"""<!doctype html>
<html lang="ko"><head><meta charset="utf-8">
<title>화면 시안 50종</title>
<style>
  body {{ margin:0; padding:26px; background:#12151c; color:#dfe4ee;
         font-family:'Malgun Gothic','Segoe UI',sans-serif; }}
  h1 {{ font-size:21px; margin:0 0 6px; }}
  h2 {{ font-size:16px; margin:30px 0 10px; }}
  h2 code {{ font-size:12px; color:#8fa0bd; font-weight:400; margin-left:8px; }}
  .note {{ font-size:13px; color:#9fb0cc; max-width:1000px; line-height:1.6; }}
  .gap {{ font-size:12px; color:#c9a06a; }}
  .row {{ display:flex; flex-wrap:wrap; gap:16px; }}
  figure {{ margin:0; }}
  figcaption {{ font-size:12px; color:#9fb0cc; padding-top:5px; }}
  figcaption b {{ color:#e8eefc; }}
  figcaption span {{ display:block; color:#6d7d99; font-size:11px; }}
  .frame {{ overflow:hidden; border-radius:5px; background:#0b0e14;
            box-shadow:0 4px 16px rgba(0,0,0,.45); }}
  .stage {{ position:relative; width:1920px; height:1080px; transform-origin:top left; }}
  .w {{ position:absolute; }}
  .tex {{ background-size:100% 100%; background-repeat:no-repeat; }}
  .t {{ display:flex; align-items:center; line-height:1.2; overflow:hidden;
        text-shadow:0 0 3px rgba(0,0,0,.95), 1.5px 1.5px 0 rgba(0,0,0,.8); }}
  .slot {{ border:1px dashed rgba(150,170,200,.40); border-radius:3px; }}
  .btn {{ border:1px dashed rgba(255,190,120,.30); border-radius:3px; }}
  .bar {{ background:rgba(0,0,0,.35); border-radius:3px; overflow:hidden; }}
  .bar i {{ display:block; height:100%; }}
{css_rules}
</style></head><body>
<h1>화면 시안 50종 — 10화면 × 5안</h1>
<p class="note">v01 안전안 · v02 재배치 · v03 야간 · v04 미니멀 · v05 실험.
모두 1920×1080 전면이며 기존 에셋만 사용했습니다. 점선은 런타임에 채우는 자리입니다.</p>
{gap_note}
{''.join(sections)}
</body></html>
"""
    OUT_PATH.write_text(document, encoding="utf-8")
    print(f"wrote {OUT_PATH} ({OUT_PATH.stat().st_size // 1024} KB), "
          f"textures={len(class_of)} missing={len(missing)}")


if __name__ == "__main__":
    main()
