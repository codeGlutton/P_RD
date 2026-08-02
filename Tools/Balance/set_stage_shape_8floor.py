"""스테이지 형태를 8층 기획안으로 맞춘다(알고리즘 수정 없이 설정값만).

1층 시작 / 2~6층 일반 / 7층 상점 / 8층 보스 = RowCount 8.
층 역할은 코드가 끝에서부터 상대적으로 정하므로 RowCount만 바꾸면 배치가 맞는다.
열 수는 7 -> 5로 줄인다. 한 층에 방이 3~4개뿐인데 7칸 폭을 잡으면 폰 화면에서
열 간격이 아이콘보다 좁아진다.
"""

import json

import unreal

TABLE_PATH = "/Game/BP/DataTable/DT_StageBuildSetting"

# (JSON 키 후보(소문자 비교), 새 값)
OVERRIDES = {
    "rowcount": 8,
    "columncount": 5,
    "minstartpointcount": 3,
}


def apply_overrides(row: dict) -> list:
    changed = []
    for key in list(row.keys()):
        normalized = key.lstrip("m").lower()
        if normalized in OVERRIDES:
            old = row[key]
            new = OVERRIDES[normalized]
            if old != new:
                row[key] = new
                changed.append(f"{key} {old}->{new}")
    return changed


def main() -> None:
    table = unreal.load_asset(TABLE_PATH)
    if table is None:
        raise RuntimeError(f"DataTable 없음: {TABLE_PATH}")

    rows = json.loads(table.export_to_json_string())
    unreal.log(f"RD_STAGE_SHAPE 행 수={len(rows)}")

    for row in rows:
        name = row.get("Name", "?")
        changed = apply_overrides(row)
        unreal.log(f"RD_STAGE_SHAPE {name}: {', '.join(changed) if changed else '변경 없음'}")

    if not table.fill_from_json_string(json.dumps(rows)):
        raise RuntimeError("DataTable JSON 반영 실패")

    if not unreal.EditorAssetLibrary.save_loaded_asset(table, only_if_is_dirty=False):
        raise RuntimeError("DataTable 저장 실패")

    # 저장 후 재확인
    verify = json.loads(table.export_to_json_string())
    for row in verify:
        unreal.log(
            f"RD_STAGE_SHAPE 확인 {row.get('Name')}: "
            + " ".join(
                f"{k}={v}"
                for k, v in row.items()
                if k.lstrip("m").lower() in OVERRIDES
                or k.lstrip("m").lower() in ("maxpathcount", "maxstartpointcount")
            )
        )
    unreal.log("RD_STAGE_SHAPE done")


main()
