"""Redirect every reference away from the doomed textures, then let them go.

왜 위젯을 안 훑나
-----------------
파이썬에서는 WidgetTree 의 자식을 훑을 길이 없다(get_children_count 가 0 을 준다).
이름을 다 알아야 찍을 수 있는데, 지울 그림을 어느 위젯이 쓰는지는 모른다.

대신 엔진이 주는 **참조 합치기**(consolidate)를 쓴다. "A 를 쓰는 모든 곳을 B 로
바꾸고 A 를 지운다" 를 엔진이 해 준다. WBP 든 머티리얼이든 가리지 않는다.

무엇으로 바꾸나
---------------
* KitA 에 같은 뜻의 부품이 있으면 그것으로 (슬라이더 홈·단추 판·바깥 틀)
* 없으면 **4x4 투명 그림** 으로. 위젯은 남고 자리도 유지되지만 아무것도 안 그린다.
  브러시를 그냥 비우면 흰 사각형이 남는다 -- 몬스터탭에서 실제로 그랬다.

새 그림이 오면 그 자리에 넣으면 된다.

Run headless:
    UnrealEditor-Cmd.exe <project> -run=pythonscript
        -script="Tools/UI/clear_asset_refs.py" -unattended -nop4 -nosplash -nullrhi
"""

import json
from pathlib import Path

import unreal

ROOT = Path("D:/UnrealProjects/P_RD_develop_20260803")
REFS = ROOT / "Saved/LegacyAudit/asset_refs.json"
OUT = ROOT / "Saved/LegacyAudit/asset_clear.txt"
KIT = "/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/KitA"
# 투명 그림은 지웠다. 이제 대체가 없으면 PurgeWidgetTextures 로
# 아무것도 안 그리게(NoDrawType) 만든다 -- 자산을 하나 더 두지 않는다.
EMPTY = None

# 지울 그림 -> 대신 쓸 KitA 부품. 없으면 투명 그림으로 간다.
REPLACE = {
    "T_set_slider_track": "T_KitA_Slider_Track",
    "T_set_slider_knob": "T_KitA_Slider_Knob",
    "T_wm_action_button_frame_normal": "T_KitA_Button_Wide_Normal",
    "T_wm_action_button_frame_pressed": "T_KitA_Button_Wide_Pressed",
    "T_MT_BaseFrame": "T_KitA_Frame_Outer",
    "T_CombatHUD_UnitHpBar_Backplate": "T_KitA_Row_Plate",
    "T_MercenaryRoster_Shell": "T_KitA_Frame_Outer",
    "T_menu_button_frame_normal": "T_KitA_Button_Wide_Normal",
    "T_CombatHUD_SkillSlotFrame": "T_KitA_Cell_Normal",
}

LINES = []


def main():
    data = json.loads(REFS.read_text(encoding="utf-8"))
    empty = unreal.EditorAssetLibrary.load_asset(EMPTY)
    if empty is None:
        LINES.append(f"빈 그림이 없다: {EMPTY}")
        return

    groups = {}
    for entry in data["blocked"]:
        swap = REPLACE.get(entry["name"])
        target = f"{KIT}/{swap}" if swap else EMPTY
        groups.setdefault(target, []).append(entry["asset"])

    for target, sources in sorted(groups.items()):
        into = unreal.EditorAssetLibrary.load_asset(target)
        if into is None:
            LINES.append(f"대상 없음 {target} -- 건너뜀: {len(sources)}개")
            continue
        assets = [unreal.EditorAssetLibrary.load_asset(path) for path in sources]
        assets = [asset for asset in assets if asset is not None and asset != into]
        if not assets:
            continue
        try:
            unreal.EditorAssetLibrary.consolidate_assets(into, assets)
            LINES.append(f"== {target} 로 합침 {len(assets)}개 ==")
            LINES.extend(f"  {path}" for path in sources)
        except Exception as error:  # noqa: BLE001
            LINES.append(f"합치기 실패 {target}: {error}")
        LINES.append("")


try:
    main()
except Exception as error:  # noqa: BLE001
    import traceback
    LINES.append("FAILED: " + traceback.format_exc())
finally:
    OUT.write_text("\n".join(LINES) + "\n", encoding="utf-8")
