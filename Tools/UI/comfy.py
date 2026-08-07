"""ComfyUI 로 UI 그림을 만든다. 5090(32GB) 세팅.

어느 모델을 언제 쓰나
---------------------
    parts   FLUX.2 Klein 9B + 게임 에셋 LoRA
            틀 · 판 · 단추 · 칸 같은 **UI 부품**. 한 개만 그리고, 테두리가
            얇고, 평평한 정면에, 배경이 단색이라 알파를 따기 쉽다. 30초.

    art     FLUX.2 dev 32B (GGUF Q6) + Turbo LoRA
            초상화 · 배경 · 타이틀 원화처럼 **화질이 곧 값어치**인 것.
            훨씬 무겁고 느리다.

**큰 모델이 늘 낫지는 않다.** 게임 에셋 LoRA 는 Klein 9B 용으로 학습된 것이라
dev 로 못 옮긴다(dev 용은 아직 없다). 부품처럼 좁고 반복적인 일은 전용 LoRA
붙은 작은 모델이 범용 큰 모델을 이긴다 -- SDXL · Qwen · dev 와 견줘 본 결과다.

32GB 에 어떻게 올리나
---------------------
dev 는 FP8 이 33GB 라 카드에 안 들어간다. GGUF Q6_K(25.5GB)로 내리고, 텍스트
인코더(Mistral 16.8GB)는 ComfyUI 가 번갈아 올린다 -- 둘 다 따로는 들어간다.

    python Tools/UI/comfy.py parts "낡은 양피지 판" 1024 768
"""

import json
import sys
import time
import urllib.parse
import urllib.request
from pathlib import Path

HOST = "http://127.0.0.1:8288"
OUT = Path("D:/UnrealProjects/P_RD_develop_20260803/Saved/UIGen")

# 화풍은 한 곳에만 적는다. 화면마다 다르면 한 게임처럼 안 보인다.
STYLE = ("hand-painted fantasy mobile game UI art, warm parchment and carved "
         "wood, soft gold trim, flat even lighting, plain background, no text")


def queue(workflow):
    data = json.dumps({"prompt": workflow}).encode("utf-8")
    request = urllib.request.Request(f"{HOST}/prompt", data=data,
                                     headers={"Content-Type": "application/json"})
    with urllib.request.urlopen(request, timeout=30) as response:
        return json.loads(response.read())["prompt_id"]


def wait(prompt_id, timeout=1800):
    deadline = time.time() + timeout
    while time.time() < deadline:
        with urllib.request.urlopen(f"{HOST}/history/{prompt_id}", timeout=20) as response:
            history = json.loads(response.read())
        if prompt_id in history:
            status = history[prompt_id].get("status", {})
            if status.get("status_str") == "error":
                raise RuntimeError(json.dumps(status, ensure_ascii=False)[:500])
            return history[prompt_id]
        time.sleep(3)
    raise TimeoutError(f"{prompt_id} 가 {timeout}초 안에 안 끝남")


def fetch(image, target):
    query = urllib.parse.urlencode({
        "filename": image["filename"], "subfolder": image.get("subfolder", ""),
        "type": image.get("type", "output")})
    with urllib.request.urlopen(f"{HOST}/view?{query}", timeout=120) as response:
        target.write_bytes(response.read())


def parts_workflow(prompt, width, height, seed, strength=0.9):
    """UI 부품. Klein 9B + 게임 에셋 LoRA."""
    return {
        "1": {"class_type": "UNETLoader",
              "inputs": {"unet_name": "flux-2-klein-9b-fp8.safetensors",
                         "weight_dtype": "default"}},
        "2": {"class_type": "CLIPLoader",
              "inputs": {"clip_name": "qwen_3_8b_fp8mixed.safetensors", "type": "flux2"}},
        "3": {"class_type": "VAELoader", "inputs": {"vae_name": "flux2-vae.safetensors"}},
        "4": {"class_type": "CLIPTextEncode",
              "inputs": {"text": f"{prompt}, {STYLE}", "clip": ["2", 0]}},
        "5": {"class_type": "EmptyFlux2LatentImage",
              "inputs": {"width": width, "height": height, "batch_size": 1}},
        "6": {"class_type": "LoraLoaderModelOnly",
              "inputs": {"lora_name": "flux-2-klein-9b-game-asset-tiles-lora.safetensors",
                         "strength_model": strength, "model": ["1", 0]}},
        "7": {"class_type": "KSampler",
              "inputs": {"seed": seed, "steps": 28, "cfg": 4.0,
                         "sampler_name": "euler", "scheduler": "simple", "denoise": 1.0,
                         "model": ["6", 0], "positive": ["4", 0], "negative": ["4", 0],
                         "latent_image": ["5", 0]}},
        "8": {"class_type": "VAEDecode", "inputs": {"samples": ["7", 0], "vae": ["3", 0]}},
        "9": {"class_type": "SaveImage",
              "inputs": {"filename_prefix": "rd_parts", "images": ["8", 0]}},
    }


def art_workflow(prompt, width, height, seed, turbo=True):
    """원화. dev 32B(GGUF Q6) + Turbo LoRA.

    Turbo LoRA 를 붙이면 8 스텝으로 끝난다. 안 붙이면 28 스텝이라 2K 한 장에
    2분 넘게 걸린다 -- 시안을 여러 장 뽑는 일에는 못 쓴다.
    """
    workflow = {
        "1": {"class_type": "UnetLoaderGGUF",
              "inputs": {"unet_name": "flux2-dev-Q6_K.gguf"}},
        "2": {"class_type": "CLIPLoader",
              "inputs": {"clip_name": "mistral_3_small_flux2_fp8.safetensors",
                         "type": "flux2"}},
        "3": {"class_type": "VAELoader", "inputs": {"vae_name": "flux2-vae.safetensors"}},
        "4": {"class_type": "CLIPTextEncode",
              "inputs": {"text": f"{prompt}, {STYLE}", "clip": ["2", 0]}},
        "5": {"class_type": "EmptyFlux2LatentImage",
              "inputs": {"width": width, "height": height, "batch_size": 1}},
        "7": {"class_type": "KSampler",
              "inputs": {"seed": seed, "steps": 8 if turbo else 28,
                         "cfg": 1.0 if turbo else 4.0,
                         "sampler_name": "euler", "scheduler": "simple", "denoise": 1.0,
                         "model": ["6", 0] if turbo else ["1", 0],
                         "positive": ["4", 0], "negative": ["4", 0],
                         "latent_image": ["5", 0]}},
        "8": {"class_type": "VAEDecode", "inputs": {"samples": ["7", 0], "vae": ["3", 0]}},
        "9": {"class_type": "SaveImage",
              "inputs": {"filename_prefix": "rd_art", "images": ["8", 0]}},
    }
    if turbo:
        workflow["6"] = {"class_type": "LoraLoaderModelOnly",
                         "inputs": {"lora_name": "Flux_2-Turbo-LoRA_comfyui.safetensors",
                                    "strength_model": 1.0, "model": ["1", 0]}}
    return workflow


ENGINES = {"parts": parts_workflow, "art": art_workflow}


def make(engine, name, prompt, width=1024, height=768, seed=1):
    """한 장 만들어 Saved/UIGen 아래에 놓는다. @return 만든 파일 목록."""
    began = time.time()
    result = wait(queue(ENGINES[engine](prompt, width, height, seed)))
    folder = OUT / engine
    folder.mkdir(parents=True, exist_ok=True)
    saved = []
    for node in result.get("outputs", {}).values():
        for index, image in enumerate(node.get("images", [])):
            target = folder / (f"{name}.png" if index == 0 else f"{name}_{index}.png")
            fetch(image, target)
            saved.append(target)
    print(f"  {engine}/{name}: {time.time() - began:.0f}초")
    return saved


if __name__ == "__main__":
    engine = sys.argv[1] if len(sys.argv) > 1 else "parts"
    prompt = sys.argv[2] if len(sys.argv) > 2 else "an empty parchment panel"
    width = int(sys.argv[3]) if len(sys.argv) > 3 else 1024
    height = int(sys.argv[4]) if len(sys.argv) > 4 else 768
    for path in make(engine, "cli", prompt, width, height, 1):
        print(f"    -> {path}")
