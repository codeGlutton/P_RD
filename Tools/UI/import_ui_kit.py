"""Import the sliced Concept A UI parts into Unreal as UI textures.

어디에 넣나
-----------
아트 원본은 SVN 이 정본이다(`Content/SVN` 은 D:/UnrealProjects/SVN 정션).
git 폴더에는 두지 않는다. 그래서 PNG 를 SVN 쪽으로 먼저 복사한 뒤 그 자리에서
임포트한다.

    Content/SVN/OutSideAsset/AICreation/UI/Marchbound/KitA/

UI 텍스처 설정
--------------
셋 다 안 맞추면 폰에서 티가 난다.

    LODGroup   TEXTUREGROUP_UI     UI 전용 압축·필터
    MipGen     NoMipmaps           UI 는 축소 밉이 필요 없고, 있으면 흐려진다
    NeverStream                    스트리밍으로 늦게 뜨면 한 프레임 빈 칸이 보인다

9-slice 여백은 텍스처가 아니라 **브러시**의 값이라 여기서 못 넣는다.
`kit_manifest_a.py` 에 적어 두고 배치 빌더가 읽어 쓴다.

Run headless -- ``-nullrhi`` 를 빼야 한다:
    UnrealEditor-Cmd.exe <project> -run=pythonscript
        -script="Tools/UI/import_ui_kit.py" -unattended -nop4 -nosplash

``-nullrhi`` 를 붙이면 텍스처 임포트 팩토리가 Slate 를 건드리는 순간
"Assertion failed: CurrentApplication.IsValid()" 로 죽는다. 실제로 그렇게 죽었다.
"""

import shutil
import sys
from pathlib import Path

import unreal

sys.path.insert(0, str(Path(__file__).resolve().parent))

from kit_manifest_a import FRAME_PARTS, PARTS  # noqa: E402

PROJECT = Path("D:/UnrealProjects/P_RD_develop_20260803")
SOURCE = PROJECT / "Saved/UIKit/ConceptA"
FRAME_SOURCE = PROJECT / "Saved/UIKit/FrameA"
ART_DIR = PROJECT / "Content/SVN/OutSideAsset/AICreation/UI/Marchbound/KitA"
GAME_DIR = "/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/KitA"
RESULT = PROJECT / "Saved/LegacyAudit/kit_import.txt"
LINES = []


def run():
    ART_DIR.mkdir(parents=True, exist_ok=True)

    # 부품 시트에서 뜬 것과 전면 프레임에서 나눈 것. 원본 위치가 다르다.
    wanted = [(SOURCE / f"part_{index:02d}.png", name)
              for index, name, _draw, _margin, _use in PARTS]
    wanted += [(FRAME_SOURCE / f"{name}.png", name)
               for name, _draw, _margin, _pad, _use in FRAME_PARTS]

    tasks = []
    for source, name in wanted:
        if not source.exists():
            LINES.append(f"{name}: 원본 없음 {source.name}")
            continue
        target = ART_DIR / f"{name}.png"
        shutil.copyfile(source, target)

        task = unreal.AssetImportTask()
        task.set_editor_property("filename", str(target))
        task.set_editor_property("destination_path", GAME_DIR)
        task.set_editor_property("destination_name", name)
        task.set_editor_property("automated", True)
        task.set_editor_property("replace_existing", True)
        # save 는 켜지 않는다. 임포트 팩토리가 저장 대화상자를 띄우려 하고,
        # 커맨드릿에는 Slate 가 없어 그 자리에서 죽는다. 아래에서 직접 저장한다.
        task.set_editor_property("save", False)
        tasks.append((name, task))

    if not tasks:
        LINES.append("임포트할 것이 없다")
        return

    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(
        [task for _name, task in tasks])

    for name, _task in tasks:
        path = f"{GAME_DIR}/{name}"
        texture = unreal.load_object(None, f"{path}.{name}")
        if texture is None:
            LINES.append(f"{name}: 임포트 실패")
            continue
        texture.modify()
        texture.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
        texture.set_editor_property("mip_gen_settings",
                                    unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
        texture.set_editor_property("never_stream", True)
        # 알파를 살린다. UI 부품은 투명 여백이 곧 모양이다.
        texture.set_editor_property("compression_settings",
                                    unreal.TextureCompressionSettings.TC_EDITOR_ICON)
        unreal.EditorAssetLibrary.save_loaded_asset(texture, False)
        width = int(texture.blueprint_get_size_x())
        height = int(texture.blueprint_get_size_y())
        LINES.append(f"{name}: {width}x{height} ok")


try:
    run()
except Exception as error:  # noqa: BLE001
    import traceback
    LINES.append("FAILED: %s" % error)
    LINES.append(traceback.format_exc())
finally:
    RESULT.write_text("\n".join(LINES), encoding="utf-8")
