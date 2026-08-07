"""게임 원본을 고쳐 **같은 화풍의 다른 부품**을 만든다.

왜 이 방법인가
--------------
글 프롬프트로만 시키면 "깔끔한 판타지 UI" 가 나오지, **우리 게임 것**이 안
나온다. 게임에는 이미 351장이 있는데 그걸 한 번도 안 보여 주고 있었다.

Qwen-Image-Edit 2509 는 원본을 참조로 받아 고친다. 그래서 나무결·금세공·
양피지 얼룩 같은, 글로는 못 적는 결이 그대로 남는다.

배선에서 빠뜨리기 쉬운 것
-------------------------
처음에 KSampler 만 놓고 돌렸다가 **단색 사각**이 나왔다. 공식 템플릿과
견줘 보니 셋이 없었다.

    ModelSamplingAuraFlow(shift=3)  없으면 잠재값이 엉뚱한 배율로 돈다
    CFGNorm(1)                      Lightning LoRA 와 짝
    FluxKontextImageScale           참조를 모델이 아는 크기로 맞춘다

그리고 스텝은 **4** 다. Lightning 4steps LoRA 이므로 8을 주면 망가진다.

할 수 있는 것 / 없는 것
-----------------------
넓게 · 높게 · 장식 얹기 · 상태 바꾸기(고름/못 누름)는 된다.
**없던 물건은 못 만든다** -- 그건 새로 그려야 한다(comfy.py 의 parts).

    python Tools/UI/comfy_edit.py
"""

import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from comfy import OUT, fetch, queue, wait  # noqa: E402

REF_DIR = Path("D:/ComfyUI_windows_portable/ComfyUI_windows_portable/ComfyUI/input")
# 참조는 **원본 해상도**여야 한다.
#
# 목록 페이지의 그림(mockups/assets)은 320px 로 줄인 미리보기다. 그걸 참조로
# 넣고 있었더니 결과가 벡터처럼 딱딱하고 종이 질감이 없었다. 원본은
# export_full.py 가 Saved/UIGen/source 에 뽑아 둔다.
ASSETS = Path("D:/UnrealProjects/P_RD_develop_20260803/Saved/UIGen/source")

# 화풍을 지키라는 말은 매번 같아야 한다. 문장이 흔들리면 결과도 흔들린다.
KEEP = ("Keep the exact same painting style, colours, materials and texture "
        "as the original. ")


# 빠른 길과 정품 길.
#
# Lightning 4-step LoRA 는 4스텝 · cfg 1.0 으로 6초에 끝난다. 공식 템플릿에
# 스위치가 있는데(20스텝 · cfg 4.0) 나는 빠른 쪽만 쓰고 있었다. 8배 빠른
# 대신 테두리가 각지고 질감이 죽는다. 시안을 훑을 때만 빠른 쪽을 쓴다.
FAST = False


def workflow(prompt, reference, seed, fast=None):
    fast = FAST if fast is None else fast
    base = {
        "1": {"class_type": "UNETLoader",
              "inputs": {"unet_name": "qwen_image_edit_2509_fp8_e4m3fn.safetensors",
                         "weight_dtype": "default"}},
        "2": {"class_type": "CLIPLoader",
              "inputs": {"clip_name": "qwen_2.5_vl_7b_fp8_scaled.safetensors",
                         "type": "qwen_image"}},
        "3": {"class_type": "VAELoader",
              "inputs": {"vae_name": "qwen_image_vae.safetensors"}},
        "11": {"class_type": "LoadImage", "inputs": {"image": reference}},
        "14": {"class_type": "FluxKontextImageScale", "inputs": {"image": ["11", 0]}},
        "15": {"class_type": "ModelSamplingAuraFlow",
               "inputs": {"shift": 3.0, "model": ["12", 0] if fast else ["1", 0]}},
        "16": {"class_type": "CFGNorm", "inputs": {"strength": 1.0, "model": ["15", 0]}},
        "4": {"class_type": "TextEncodeQwenImageEditPlus",
              "inputs": {"prompt": KEEP + prompt, "clip": ["2", 0], "vae": ["3", 0],
                         "image1": ["14", 0]}},
        "10": {"class_type": "TextEncodeQwenImageEditPlus",
               "inputs": {"prompt": "blurry, low detail, extra decoration, flat vector",
                          "clip": ["2", 0], "vae": ["3", 0], "image1": ["14", 0]}},
        "13": {"class_type": "VAEEncode",
               "inputs": {"pixels": ["14", 0], "vae": ["3", 0]}},
        "7": {"class_type": "KSampler",
              # 샘플러는 **정품 길에서 바꿔야 한다.**
              #
              # euler/simple 은 Lightning 4스텝 템플릿의 기본값이다. 스텝만
              # 20으로 올리고 그대로 뒀더니, 40으로 더 올려도 세부량이
              # 36 -> 42 밖에 안 늘었다. 4스텝용 조합이 20스텝을 못 쓰는 것이다.
              # dpmpp_2m/karras 로 바꾸니 **같은 66초에 세부량 97** 이 나왔다.
              "inputs": {"seed": seed, "steps": 4 if fast else 30,
                         "cfg": 1.0 if fast else 4.0,
                         "sampler_name": "euler" if fast else "dpmpp_2m",
                         "scheduler": "simple" if fast else "karras",
                         "denoise": 1.0, "model": ["16", 0],
                         "positive": ["4", 0], "negative": ["10", 0],
                         "latent_image": ["13", 0]}},
        "8": {"class_type": "VAEDecode",
              "inputs": {"samples": ["7", 0], "vae": ["3", 0]}},
        "9": {"class_type": "SaveImage",
              "inputs": {"filename_prefix": "rd_edit", "images": ["8", 0]}},
    }
    if fast:
        workflow_lora = {"class_type": "LoraLoaderModelOnly",
                         "inputs": {"lora_name":
                                    "Qwen-Image-Edit-2509-Lightning-4steps-V1.0-bf16.safetensors",
                                    "strength_model": 1.0, "model": ["1", 0]}}
        base["12"] = workflow_lora
    return base


def stage(name):
    """게임 원본을 ComfyUI 가 읽는 자리로. 이미 있으면 그대로 쓴다."""
    source = ASSETS / f"{name}.png"
    if not source.is_file():
        return None
    target = REF_DIR / f"ref_{name}.png"
    if not target.is_file():
        REF_DIR.mkdir(parents=True, exist_ok=True)
        from PIL import Image
        Image.open(source).convert("RGB").save(target)
    return target.name


def make(label, asset, prompt, seed=7):
    reference = stage(asset)
    if reference is None:
        print(f"  {label}: 원본 {asset} 없음")
        return None
    folder = OUT / "edit"
    folder.mkdir(parents=True, exist_ok=True)
    began = time.time()
    try:
        result = wait(queue(workflow(prompt, reference, seed)))
    except Exception as exc:  # noqa: BLE001
        print(f"  {label} 실패: {str(exc)[:180]}")
        return None
    made = None
    for node in result.get("outputs", {}).values():
        for image in node.get("images", []):
            made = folder / f"{label}.png"
            fetch(image, made)
    print(f"  {label}: {time.time() - began:.0f}초")
    return made


# 지금 판에 **없어서 아쉬운 것**들. 상태 그림이 없어 코드가 색만 바꾸고 있다.
JOBS = [
    ("Cell_Selected_gold", "T_KitA_Cell_Normal",
     "Make this cell look selected: add a bright gold glowing rim around the border."),
    ("Cell_Disabled_grey", "T_KitA_Cell_Normal",
     "Make this cell look disabled: desaturate to grey and dim it."),
    ("Button_Pressed", "T_KitA_Button_Small_Normal",
     "Make this button look pressed down: darker and slightly recessed."),
    ("Button_Disabled", "T_KitA_Button_Small_Normal",
     "Make this button look disabled: desaturated grey wood and dull metal."),
    ("Panel_Tall", "T_MB_HireRowNormal",
     "Make this panel much taller, keeping the border thin and the centre empty."),
    ("Panel_Square", "T_MB_HireRowNormal",
     "Make this panel square, keeping the border thin and the centre empty."),
    ("Plate_Wide", "T_MB_HireNamePlate",
     "Make this nameplate wider and empty, keeping the cut corners and rivets."),
    ("Chip_Ring_Gold", "T_KitA_StatChip_Ring",
     "Make this ring gold instead, keeping the same shape and thickness."),
    ("Chip_Ring_Red", "T_KitA_StatChip_Ring",
     "Make this ring deep red instead, keeping the same shape and thickness."),
    ("Frame_Two_Column", "T_KitA_Frame_Outer",
     "Add one vertical divider pillar down the middle, in the same wood and gold."),
    ("Portrait_Round", "T_KitA_Portrait_Frame",
     "Make this frame circular instead of square, same wood and gold work."),
    ("Slider_Fill_Gold", "T_KitA_Slider_Track",
     "Fill this slider track with a warm gold bar, same style."),
]


def main():
    made = 0
    for label, asset, prompt in JOBS:
        if make(label, asset, prompt) is not None:
            made += 1
    print(f"\n{made}/{len(JOBS)}장")


if __name__ == "__main__":
    main()
