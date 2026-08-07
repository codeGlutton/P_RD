"""만드는 중인 것을 브라우저에서 계속 볼 수 있게 한다.

왜
--
사람이 "만들어지는지 확인이 안 되니 갑갑하다" 고 했다. 맞는 말이다 -- 진행이
내 쪽 로그에만 있었고, 물어봐야만 알 수 있었다.

이 스크립트는 몇 초마다
    1. 받는 중인 모델 파일 크기를 재고
    2. ComfyUI 큐에 뭐가 걸려 있는지 묻고
    3. 새로 나온 그림을 웹에서 보이는 자리로 옮기고
    4. 그 셋을 ``genstatus.json`` 에 적는다.

``gen.html`` 이 그 파일을 계속 읽어 그린다. 그래서 브라우저만 띄워 두면 된다.

    python Tools/UI/gen_status.py          # 계속 돌린다
"""

import json
import shutil
import time
import urllib.request
from pathlib import Path

ROOT = Path("D:/UnrealProjects/P_RD_develop_20260803")
GEN = ROOT / "Saved/UIGen"
WEB = ROOT / "Tools/UI/mockups"
SHOW = WEB / "gen"
STATUS = WEB / "genstatus.json"
HOST = "http://127.0.0.1:8288"

MODELS = Path("D:/ComfyUI_windows_portable/ComfyUI_windows_portable/ComfyUI/models")
# 받는 중인 것 -> 다 받으면 몇 바이트인지. 크기를 알아야 몇 %인지 말할 수 있다.
DOWNLOADS = [
    ("flux2-dev-Q6_K.gguf", MODELS / "unet/flux2-dev-Q6_K.gguf", 27_380_000_000),
    ("mistral 텍스트 인코더",
     MODELS / "text_encoders/mistral_3_small_flux2_fp8.safetensors", 18_040_000_000),
    ("Turbo LoRA", MODELS / "loras/Flux_2-Turbo-LoRA_comfyui.safetensors", 2_760_000_000),
]


def ask(path, timeout=5):
    try:
        with urllib.request.urlopen(f"{HOST}{path}", timeout=timeout) as response:
            return json.loads(response.read())
    except Exception:  # noqa: BLE001
        return None


def downloads():
    rows = []
    for label, path, total in DOWNLOADS:
        have = path.stat().st_size if path.is_file() else 0
        rows.append({"name": label, "have": have, "total": total,
                     "done": have >= total * 0.995})
    return rows


def queue_state():
    data = ask("/queue")
    if data is None:
        return {"up": False, "running": 0, "pending": 0}
    return {"up": True,
            "running": len(data.get("queue_running", [])),
            "pending": len(data.get("queue_pending", []))}


# 이름만으로는 어느 원본에서 나왔는지 모르는 것들.
GUESS = {
    "Cell_Selected_gold": "T_KitA_Cell_Normal", "Cell_Disabled_grey": "T_KitA_Cell_Normal",
    "Button_Disabled": "T_KitA_Button_Small_Normal",
    "Chip_Ring_Gold": "T_KitA_StatChip_Ring", "Chip_Ring_Red": "T_KitA_StatChip_Ring",
    "Ring_Blue": "T_KitA_StatChip_Ring", "Ring_Green": "T_KitA_StatChip_Ring",
    "Ring_Steel": "T_KitA_StatChip_Ring",
    "HPFill_Blue": "T_KitA_HPBar_Fill_Red", "HPFill_Yellow": "T_KitA_HPBar_Fill_Red",
    "Portrait_Round": "T_KitA_Portrait_Frame",
    "Thin_Portrait": "T_KitA_Portrait_Frame", "Thin_ButtonSmall": "T_KitA_Button_Small_Normal",
    "Thin_Cell": "T_KitA_Cell_Normal", "Thin_Ring": "T_KitA_StatChip_Ring",
    "Thin_FrameOuter": "T_KitA_Frame_Outer", "Thin_HireRow": "T_MB_HireRowNormal",
}


def publish():
    """새로 나온 그림을 웹에서 볼 수 있는 자리로 옮긴다."""
    SHOW.mkdir(parents=True, exist_ok=True)
    shown = []
    for path in sorted(GEN.rglob("*.png"), key=lambda p: -p.stat().st_mtime):
        if path.name.startswith("_") or "rejected" in path.parts:
            continue
        # 폴더가 다르면 이름도 달라야 한다. parts/Panel.png 와 models/Panel.png.
        flat = f"{path.parent.name}__{path.name}" if path.parent != GEN else path.name
        target = SHOW / flat
        if not target.is_file() or target.stat().st_mtime < path.stat().st_mtime:
            shutil.copy2(path, target)
        # 어느 원본에서 고친 것인지 알려 준다. 나란히 놓고 봐야 판단이 된다.
        stem = path.stem
        source = None
        if stem.startswith("Thin__"):
            source = stem.split("__")[1]
        else:
            source = GUESS.get(stem)
        shown.append({"file": f"gen/{flat}", "name": stem,
                      "group": path.parent.name if path.parent != GEN else "초기",
                      "src": f"assets/{source}.png" if source else "",
                      "when": int(path.stat().st_mtime)})
    return shown


def main():
    """혼자 돌릴 때. 보통은 mockups_server 가 곁다리로 돌린다."""
    WEB.mkdir(parents=True, exist_ok=True)
    while True:
        state = {"at": int(time.time()), "downloads": downloads(),
                 "queue": queue_state(), "images": publish()}
        STATUS.write_text(json.dumps(state, ensure_ascii=False), encoding="utf-8")
        time.sleep(5)


if __name__ == "__main__":
    main()
