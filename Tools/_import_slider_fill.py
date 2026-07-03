# 13_Settings/02_Slider/slider_fill.png 을 T_set_slider_fill 텍스처로 재임포트(교체).
# 기존 브러시 참조(Set_slider_fill_* ProgressBar)는 에셋 경로가 같아 자동 유지된다.
import unreal, json

SRC = "D:/UnrealProjects/P_RD_CombatUI_Assets_nobg/13_Settings/02_Slider/slider_fill.png"
DEST = "/Game/SVN/OutSideAsset/AICreation/UI/Settings"
NAME = "T_set_slider_fill"

task = unreal.AssetImportTask()
task.filename = SRC
task.destination_path = DEST
task.destination_name = NAME
task.replace_existing = True
task.automated = True
task.save = True
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

ok = unreal.EditorAssetLibrary.does_asset_exist(DEST + "/" + NAME + "." + NAME)
print("FILLIMPORT|" + json.dumps({"imported": ok, "src": SRC}))
