#!/usr/bin/env python3
"""Generate a short MARCHBOUND title-screen motion test with local ComfyUI."""

from __future__ import annotations

import json
import shutil
import sys
import time
import urllib.error
import urllib.parse
import urllib.request
from pathlib import Path


COMFY_URL = "http://127.0.0.1:8288"
COMFY_ROOT = Path(r"D:\ComfyUI_windows_portable\ComfyUI_windows_portable\ComfyUI")
SOURCE_IMAGE = Path(
    r"D:\UnrealProjects\P_RD_develop_20260803\SourceArt\UI\Marchbound\VideoConcepts"
    r"\MARCHBOUND_TitleVideo_LoopKeyframe_20260803_v01.png"
)
INPUT_NAME = "codex_MARCHBOUND_title_i2v_20260803_v01.png"
PROJECT_OUTPUT = Path(
    r"D:\UnrealProjects\P_RD_develop_20260803\SourceArt\UI\Marchbound\VideoConcepts"
    r"\MARCHBOUND_TitleVideo_Wan22_832x480_20260803_v01.mp4"
)
ARCHIVE_OUTPUT = Path(
    r"F:\코덱스이미지생성폴더\MARCHBOUND_타이틀영상_Wan22_832x480_20260803_v01.mp4"
)


def api_json(path: str, payload: dict | None = None) -> dict:
    data = None if payload is None else json.dumps(payload).encode("utf-8")
    request = urllib.request.Request(
        COMFY_URL + path,
        data=data,
        headers={"Content-Type": "application/json"},
        method="GET" if data is None else "POST",
    )
    with urllib.request.urlopen(request, timeout=30) as response:
        return json.loads(response.read().decode("utf-8"))


def build_prompt(seed: int) -> dict:
    positive = (
        "Locked-off wide cinematic fantasy game title screen. Preserve the exact source composition, "
        "the exact six heroes, their costumes, faces, weapons, scale, and positions. Keep the dark empty "
        "title-safe area on the left. Very subtle natural looping motion only: the knight's red cape and "
        "the ranger's cloth gently flutter, the mage's orange fire and blue frost slowly pulse, rogue smoke "
        "curls softly, druid leaves drift in a tiny circle, barbarian fur shifts slightly, sparse embers float, "
        "and distant fog drifts slowly. Subtle breathing only. No camera movement. No character changes."
    )
    negative = (
        "camera movement, zoom, dolly, pan, tilt, shake, reframing, crop change, characters walking, running, "
        "attacking, large body motion, pose change, morphing, face change, costume change, weapon change, "
        "extra character, missing character, duplicate character, extra limbs, deformed hands, distorted face, "
        "text, letters, logo, UI, subtitles, flicker, jitter, flashing, abrupt motion, overexposure, blur"
    )

    return {
        "1": {"class_type": "LoadImage", "inputs": {"image": INPUT_NAME}},
        "2": {
            "class_type": "CLIPLoader",
            "inputs": {
                "clip_name": "umt5_xxl_fp8_e4m3fn_scaled.safetensors",
                "type": "wan",
            },
        },
        "3": {"class_type": "CLIPTextEncode", "inputs": {"text": positive, "clip": ["2", 0]}},
        "4": {"class_type": "CLIPTextEncode", "inputs": {"text": negative, "clip": ["2", 0]}},
        "5": {"class_type": "VAELoader", "inputs": {"vae_name": "wan_2.1_vae.safetensors"}},
        "6": {
            "class_type": "WanImageToVideo",
            "inputs": {
                "positive": ["3", 0],
                "negative": ["4", 0],
                "vae": ["5", 0],
                "width": 832,
                "height": 480,
                "length": 81,
                "batch_size": 1,
                "start_image": ["1", 0],
            },
        },
        "7": {
            "class_type": "UNETLoader",
            "inputs": {
                "unet_name": "wan2.2_i2v_high_noise_14B_fp8_scaled.safetensors",
                "weight_dtype": "default",
            },
        },
        "8": {
            "class_type": "LoraLoaderModelOnly",
            "inputs": {
                "model": ["7", 0],
                "lora_name": "wan2.2_i2v_lightx2v_4steps_lora_v1_high_noise.safetensors",
                "strength_model": 1.0,
            },
        },
        "9": {"class_type": "ModelSamplingSD3", "inputs": {"model": ["8", 0], "shift": 5.0}},
        "10": {
            "class_type": "KSamplerAdvanced",
            "inputs": {
                "model": ["9", 0],
                "add_noise": "enable",
                "noise_seed": seed,
                "steps": 4,
                "cfg": 1.0,
                "sampler_name": "euler",
                "scheduler": "simple",
                "positive": ["6", 0],
                "negative": ["6", 1],
                "latent_image": ["6", 2],
                "start_at_step": 0,
                "end_at_step": 2,
                "return_with_leftover_noise": "enable",
            },
        },
        "11": {
            "class_type": "UNETLoader",
            "inputs": {
                "unet_name": "wan2.2_i2v_low_noise_14B_fp8_scaled.safetensors",
                "weight_dtype": "default",
            },
        },
        "12": {
            "class_type": "LoraLoaderModelOnly",
            "inputs": {
                "model": ["11", 0],
                "lora_name": "wan2.2_i2v_lightx2v_4steps_lora_v1_low_noise.safetensors",
                "strength_model": 1.0,
            },
        },
        "13": {"class_type": "ModelSamplingSD3", "inputs": {"model": ["12", 0], "shift": 5.0}},
        "14": {
            "class_type": "KSamplerAdvanced",
            "inputs": {
                "model": ["13", 0],
                "add_noise": "disable",
                "noise_seed": seed,
                "steps": 4,
                "cfg": 1.0,
                "sampler_name": "euler",
                "scheduler": "simple",
                "positive": ["6", 0],
                "negative": ["6", 1],
                "latent_image": ["10", 0],
                "start_at_step": 2,
                "end_at_step": 4,
                "return_with_leftover_noise": "disable",
            },
        },
        "15": {"class_type": "VAEDecode", "inputs": {"samples": ["14", 0], "vae": ["5", 0]}},
        "16": {"class_type": "CreateVideo", "inputs": {"images": ["15", 0], "fps": 16.0}},
        "17": {
            "class_type": "SaveVideo",
            "inputs": {
                "video": ["16", 0],
                "filename_prefix": "MARCHBOUND/title/title_loop_wan22_832x480_20260803_v01",
                "format": "mp4",
                "codec": "h264",
            },
        },
    }


def find_video_metadata(value):
    if isinstance(value, dict):
        filename = value.get("filename")
        if isinstance(filename, str) and filename.lower().endswith(".mp4"):
            return value
        for child in value.values():
            found = find_video_metadata(child)
            if found:
                return found
    elif isinstance(value, list):
        for child in value:
            found = find_video_metadata(child)
            if found:
                return found
    return None


def main() -> int:
    if not SOURCE_IMAGE.is_file():
        raise FileNotFoundError(SOURCE_IMAGE)
    for destination in (PROJECT_OUTPUT, ARCHIVE_OUTPUT):
        if destination.exists():
            raise FileExistsError(f"Refusing to overwrite existing output: {destination}")

    input_path = COMFY_ROOT / "input" / INPUT_NAME
    input_path.parent.mkdir(parents=True, exist_ok=True)
    if not input_path.exists():
        shutil.copy2(SOURCE_IMAGE, input_path)
    elif input_path.read_bytes() != SOURCE_IMAGE.read_bytes():
        raise FileExistsError(f"ComfyUI input name is already used by a different image: {input_path}")

    api_json("/system_stats")
    seed = 6080301
    result = api_json("/prompt", {"prompt": build_prompt(seed)})
    prompt_id = result["prompt_id"]
    print(f"SUBMITTED prompt_id={prompt_id} seed={seed}", flush=True)

    started = time.monotonic()
    while True:
        history = api_json(f"/history/{prompt_id}")
        record = history.get(prompt_id)
        if record:
            status = record.get("status", {})
            if status.get("status_str") == "error" or status.get("completed") is False:
                messages = status.get("messages", [])
                raise RuntimeError(f"ComfyUI execution failed: {messages}")
            metadata = find_video_metadata(record.get("outputs", {}))
            if metadata:
                break

        elapsed = int(time.monotonic() - started)
        queue = api_json("/queue")
        running = len(queue.get("queue_running", []))
        pending = len(queue.get("queue_pending", []))
        print(f"WAIT elapsed={elapsed}s running={running} pending={pending}", flush=True)
        time.sleep(10)

    filename = metadata["filename"]
    subfolder = metadata.get("subfolder", "")
    file_type = metadata.get("type", "output")
    query = urllib.parse.urlencode({"filename": filename, "subfolder": subfolder, "type": file_type})
    with urllib.request.urlopen(COMFY_URL + "/view?" + query, timeout=120) as response:
        video_bytes = response.read()

    for destination in (PROJECT_OUTPUT, ARCHIVE_OUTPUT):
        destination.parent.mkdir(parents=True, exist_ok=True)
        destination.write_bytes(video_bytes)
        print(f"SAVED {destination} ({len(video_bytes)} bytes)", flush=True)

    print(f"DONE elapsed={int(time.monotonic() - started)}s", flush=True)
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, KeyError, RuntimeError, urllib.error.URLError) as exc:
        print(f"ERROR {type(exc).__name__}: {exc}", file=sys.stderr, flush=True)
        raise
