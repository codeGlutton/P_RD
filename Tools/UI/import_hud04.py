# -*- coding: utf-8 -*-
"""시안4 전투 HUD 의 판을 텍스처로 넣는다.

## 왜 따로 만드나

import_kaykit_ui.py 가 이미 있지만 그것은 여섯 갈래의 아트를 한 번에 훑고,
지금은 그 중 몇 갈래의 원본 폴더가 없어 도중에 죽는다. 남의 큰 스크립트를
고쳐 가며 쓰는 것보다 이 판만 넣는 작은 것을 두는 편이 낫다 -- 죽으면 어디서
죽었는지 바로 보인다.

## 설정

UI 판이라 밉맵을 끈다. 판을 원래 크기 그대로 놓는 방식이라 축소해서 그릴 일이
없고, 밉맵이 있으면 첫 프레임에 흐린 단계가 잠깐 뜬다.

    python (RunEditorPython) import_hud04.py
"""
import os

import unreal

SOURCE = r"D:/UnrealProjects/P_RD_develop_20260726/Tools/UI/KayKitUIKit/HUD04"
PACKAGE = "/Game/SVN/OutSideAsset/UI/KayKit/HUD04"

names = sorted(f for f in os.listdir(SOURCE) if f.lower().endswith(".png"))
if not names:
    raise RuntimeError("판이 없다: " + SOURCE)

# 한 장씩 넣는다. 열넷을 한 번에 넘기면 첫 장만 들어가고 죽는다 -- 왜인지는
# 인터체인지 안쪽이라 안 보이고, 한 장씩이면 멀쩡히 다 들어간다.
for name in names:
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", os.path.join(SOURCE, name))
    task.set_editor_property("destination_path", PACKAGE)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", True)
    # 팩토리를 못 박는다. 안 박으면 인터체인지가 맡는데, 커맨드릿에는 Slate 가
    # 없어서 그쪽이 대화상자를 띄우려다 죽는다 -- 첫 장만 들어가고 멈췄다.
    task.set_editor_property("factory", unreal.TextureFactory())
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

# 디스크에서 다시 읽는다. 조용히 아무것도 안 한 임포트는 위젯이 흰 네모를
# 그릴 때까지 성공한 것처럼 보인다.
missing = []
for name in names:
    stem = os.path.splitext(name)[0]
    texture = unreal.EditorAssetLibrary.load_asset("{}/{}".format(PACKAGE, stem))
    if texture is None:
        missing.append(stem)
        continue
    texture.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
    texture.set_editor_property("compression_settings",
                                unreal.TextureCompressionSettings.TC_EDITOR_ICON)
    texture.set_editor_property("mip_gen_settings",
                                unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
    texture.set_editor_property("srgb", True)
    unreal.EditorAssetLibrary.save_loaded_asset(texture, False)

if missing:
    raise RuntimeError("못 넣은 판: %s" % missing)
unreal.log("[HUD04] 판 %d장 넣음" % len(names))
