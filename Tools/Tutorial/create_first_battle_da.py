"""One-time authoring of the tutorial DA. Runtime never runs this script.

Run with UnrealEditor-Cmd -run=pythonscript -script=<this file>.
Refuses to overwrite a teammate's existing authored asset.
"""
import unreal

DEST = "/Game/BP/DataAsset/Room/Turtorial/DA_Tutorial_FirstBattle"
SOURCE = "/Game/BP/DataAsset/Room/Monster/Stage1/DA_Monster_Stage1_00"


def tile(x, y):
    result = unreal.TileIndex()
    result.set_editor_property("m_x", x)
    result.set_editor_property("m_y", y)
    return result


def transform(x, y, direction):
    result = unreal.TileTransform()
    result.set_editor_property("m_index", tile(x, y))
    result.set_editor_property("m_direction", direction)
    return result


def main():
    if unreal.EditorAssetLibrary.does_asset_exist(DEST):
        raise RuntimeError("Tutorial DA already exists; edit it in the editor instead of regenerating it.")
    source = unreal.load_asset(SOURCE)
    if not source:
        raise RuntimeError("Source room is missing")
    factory = unreal.DataAssetFactory()
    factory.set_editor_property("data_asset_class", unreal.StaticTutorialRoomSpawnData)
    room = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        "DA_Tutorial_FirstBattle", DEST.rsplit("/", 1)[0], unreal.StaticTutorialRoomSpawnData, factory)
    for name in ("m_background_map", "m_default_spawn_setting_name", "m_override_bgm"):
        room.set_editor_property(name, source.get_editor_property(name))
    # Native constructor sets hidden StageLevel None; verified by C++ asset validation.
    room.set_editor_property("m_use_random_spawn_setting", False)
    room.set_editor_property("m_player_transforms", [
        transform(4, 2, unreal.TileActorDirection.RIGHT),
        transform(3, 2, unreal.TileActorDirection.RIGHT),
        transform(5, 2, unreal.TileActorDirection.RIGHT)])
    eagle = unreal.load_asset("/Game/BP/DataAsset/Unit/EnemyUnit/Stage1/DA_EagleUnit")
    enemies = []
    for x, y in ((4, 4), (6, 6)):
        enemy = unreal.EnemyUnitPlacementData()
        enemy.set_editor_property("m_spawn_data", eagle)
        enemy.set_editor_property("m_difficulty", 1)
        enemy.set_editor_property("m_default_speed_point", 0)
        enemy.set_editor_property("m_transform", transform(x, y, unreal.TileActorDirection.LEFT))
        enemies.append(enemy)
    room.set_editor_property("m_enemy_unit_placement_datas", enemies)
    # Clear practice board. The normal source room retains all its rocks, barrels and spiders.
    room.set_editor_property("m_obstacle_placement_datas", [])
    room.set_editor_property("m_round_start_events", [])
    room.set_editor_property("m_round_end_events", [])
    room.set_editor_property("move_destination", tile(4, 3))
    room.set_editor_property("target_enemy_index", 0)
    if not unreal.EditorAssetLibrary.save_loaded_asset(room, only_if_is_dirty=False):
        raise RuntimeError("Could not save tutorial DA")
    unreal.log("TUTORIAL_DA_CREATED " + room.get_path_name())


main()
