# -*- coding: utf-8 -*-
"""처음 고르는 여섯 명의 스탯 표를 만든다.

## 왜 필요한가

유닛의 스탯은 이름으로 찾는다. UStaticObstacleSpawnData::GetKeyName() 이
표시 이름에서 띄어쓰기만 빼고 그대로 쓰고, 그것이 커브 표의 행 앞머리가
된다.

    Knight.PlayerUnitAttributeSet.MaxHP

그래서 표시 이름은 표시용이 아니다. 이름을 한글로 바꾸면 그 순간 스탯을
못 찾고, 캐릭터 선택 화면이 열리다가 죽는다. 실제로 그렇게 깨뜨렸다.

이름을 되돌리는 대신 행을 만들었다. 여섯이 전부 '기사' 로 나오면 고르는
의미가 없기 때문이다.

## 값에 대하여

체력만 다르다. 나머지는 기사 것을 그대로 쓴다. 지어낸 수치를 여기저기
심어 놓으면 나중에 진짜 기획 값이 들어올 때 무엇이 임시였는지 못 가린다.
체력 여섯 개는 시안에 이미 적혀 있던 숫자다.

레벨 1 과 3 에 같은 값을 넣는다. 난이도별로 다르게 하는 것은 기획이 정할
일이고, 지금 필요한 것은 여섯이 서로 다르다는 것뿐이다.

    python (RunEditorPython) make_starting_crew_stats.py
"""
import os

import unreal

TABLE_DIR = "/Game/BP/Curve/Player"
TABLE_NAME = "CT_StartingCrewAttribute"
COMPOSITE = "/Game/BP/Curve/CCT_Attribute"

#: 표시 이름 -> 체력. 표시 이름이 곧 행 앞머리다.
CREW_HP = (
    ("기사", 100),
    ("야만전사", 120),
    ("궁수", 80),
    ("도적", 75),
    ("마법사", 70),
    ("성직자", 85),
)

#: 기사 표에서 그대로 가져온 값. (레벨1, 레벨3)
SHARED = (
    ("MaxExp", 250.0, 350.0),
    ("Money", 100.0, 50.0),
    ("RechargeMovement", 10.0, 8.0),
)

SET = "PlayerUnitAttributeSet"


def write_csv(path):
    lines = ["---,1.000000,3.000000"]
    for name, hp in CREW_HP:
        # 체력은 두 칸 다 같은 값이다. 사이 값을 물어도 같은 숫자가 나온다.
        lines.append("%s.%s.HP,%f,%f" % (name, SET, hp, hp))
        lines.append("%s.%s.MaxHP,%f,%f" % (name, SET, hp, hp))
        for attr, low, high in SHARED:
            lines.append("%s.%s.%s,%f,%f" % (name, SET, attr, low, high))
    with open(path, "w", encoding="utf-8-sig", newline="\n") as out:
        out.write("\n".join(lines) + "\n")
    return path


def import_table(csv_path):
    task = unreal.AssetImportTask()
    task.filename = csv_path
    task.destination_path = TABLE_DIR
    task.destination_name = TABLE_NAME
    task.automated = True
    task.replace_existing = True
    task.save = True

    factory = unreal.CSVImportFactory()
    settings = unreal.CSVImportSettings()
    settings.set_editor_property("import_type",
                                 unreal.CSVImportType.ECSV_CURVE_TABLE)
    settings.set_editor_property("import_curve_interp_mode",
                                 unreal.RichCurveInterpMode.RCIM_LINEAR)
    factory.set_editor_property("automated_import_settings", settings)
    task.factory = factory

    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    made = unreal.load_asset("{}/{}".format(TABLE_DIR, TABLE_NAME))
    if made is None:
        raise RuntimeError("커브 표를 못 만듦")
    return made


def attach(table):
    """합쳐진 표에 새 표를 건다. 안 걸면 아무도 못 읽는다."""
    composite = unreal.load_asset(COMPOSITE)
    parents = list(composite.get_editor_property("parent_tables"))
    if any(p.get_name() == TABLE_NAME for p in parents):
        unreal.log("[스탯] 이미 걸려 있음")
        return
    parents.append(table)
    composite.set_editor_property("parent_tables", parents)
    unreal.EditorAssetLibrary.save_loaded_asset(composite, False)
    unreal.log("[스탯] 합친 표에 {} 걸음".format(TABLE_NAME))


csv_path = os.path.join(unreal.Paths.project_saved_dir(), "CurveTables",
                        TABLE_NAME + ".csv")
os.makedirs(os.path.dirname(csv_path), exist_ok=True)
write_csv(csv_path)
attach(import_table(csv_path))
unreal.log("[스탯] {}명 행 만듦".format(len(CREW_HP)))
