# -*- coding: utf-8 -*-
"""지금 만들어져 있는 데이터 자산의 값을 그대로 뽑는다.

클래스에 무슨 칸이 있느냐가 아니라 **그 칸에 무엇이 들어 있느냐**를 본다.
상세창에 띄울 것을 정하려면 이쪽이 필요하다 -- 칸이 있어도 비어 있으면
화면에 그릴 것이 없다.

    UnrealEditor-Cmd.exe <uproject> -run=pythonscript -script=dump_gamedata.py
"""
import io
import json
import os

import unreal

OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "gamedata_dump.json")

REGISTRY = unreal.AssetRegistryHelpers.get_asset_registry()


def props(obj):
    """m_ 로 시작하는 칸을 이름과 값으로 모은다."""
    found = {}
    for name in dir(obj):
        if not name.startswith("m_"):
            continue
        try:
            value = obj.get_editor_property(name)
        except Exception:
            continue
        found[name] = describe(value)
    return found


def describe(value):
    """읽을 수 있는 꼴로 바꾼다. 자산 참조는 이름만 남긴다."""
    if value is None:
        return None
    if isinstance(value, (bool, int, float, str)):
        return value
    if isinstance(value, unreal.Text):
        return str(value)
    if isinstance(value, unreal.Name):
        return str(value)
    if isinstance(value, unreal.Array):
        return [describe(v) for v in value]
    if isinstance(value, unreal.SoftObjectPath):
        return str(value)
    if isinstance(value, unreal.GameplayTag):
        # 태그 이름을 꺼내는 칸 이름이 판마다 다르다. 문자열 표현을 그냥 쓴다.
        return str(value)
    if isinstance(value, unreal.GameplayTagContainer):
        return str(value)
    if isinstance(value, unreal.Object):
        return value.get_name()
    if isinstance(value, unreal.StructBase):
        # 구조체는 한 겹 더 들어간다. 효과 레이어가 여기 있다.
        inner = {"__struct": type(value).__name__}
        inner.update(props(value))
        return inner
    if isinstance(value, unreal.EnumBase):
        return str(value)
    return str(value)


def load(path):
    return unreal.EditorAssetLibrary.load_asset(path)


def sweep(folder):
    """폴더 아래 자산을 전부 훑는다."""
    result = {}
    for path in unreal.EditorAssetLibrary.list_assets(folder, recursive=True):
        clean = path.split(".")[0]
        asset = load(clean)
        if asset is None:
            continue
        result[clean] = {"__class": type(asset).__name__}
        result[clean].update(props(asset))
    return result


dump = {}
for name, folder in (
        ("스킬", "/Game/BP/DataAsset/Skill"),
        ("유닛", "/Game/BP/DataAsset/Unit"),
        ("장비", "/Game/BP/DataAsset/Equipment"),
        ("패시브", "/Game/BP/DataAsset/Passive"),
        ("방", "/Game/BP/DataAsset/Room"),
):
    dump[name] = sweep(folder)
    unreal.log("%s: %d개" % (name, len(dump[name])))

io.open(OUT, "w", encoding="utf-8", newline="\n").write(
    json.dumps(dump, ensure_ascii=False, indent=1))
unreal.log("적었다: %s" % OUT)
