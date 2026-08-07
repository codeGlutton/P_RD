"""Report the imported size of every world-map texture candidate.

세로 스크롤 지도는 그림 비율이 배치를 결정한다. 몸통 텍스처를 갈아 끼우기 전에
후보들의 실제 크기부터 잰다 -- 가로로 넓은 그림을 세로 스크롤에 깔면 늘어난다.
"""

from pathlib import Path

import unreal

OUT = Path("D:/UnrealProjects/P_RD_develop_20260803/Saved/LegacyAudit/map_textures.txt")
ROOT = "/Game/SVN/OutSideAsset/AICreation/UI"
CANDIDATES = [
    f"{ROOT}/RunFlow/T_StageMap_Scroll_Flat",
    f"{ROOT}/RunFlow/T_StageMap_PopupBackground",
    f"{ROOT}/RunFlow/T_StageMap_Legend_V2",
    f"{ROOT}/RunFlow/T_MapUI_ButtonPlate_V1",
    f"{ROOT}/Map/T_StageMap_Background_Parchment",
    f"{ROOT}/WorldMap/T_wm_T_DP_Map_ScrollVertical",
    f"{ROOT}/WorldMap/T_wm_panel_scrim",
    f"{ROOT}/WorldMap/T_wm_legend_full_20260704_004607",
    f"{ROOT}/WorldMap/T_wm_action_button_frame_normal",
]

LINES = []
for path in CANDIDATES:
    leaf = path.rsplit("/", 1)[-1]
    texture = unreal.load_object(None, f"{path}.{leaf}")
    if texture is None:
        LINES.append(f"{leaf:44s} 없음")
        continue
    width = int(texture.blueprint_get_size_x())
    height = int(texture.blueprint_get_size_y())
    ratio = width / height if height else 0.0
    LINES.append(f"{leaf:44s} {width:5d}x{height:5d}  ratio={ratio:.2f}")

OUT.write_text("\n".join(LINES), encoding="utf-8")
