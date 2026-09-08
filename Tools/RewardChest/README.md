# 상자와 빛 분리

원본 33프레임의 색상은 그대로 사용한다. 별도 흑백 마스크가 상자 밖의 잘린 빛과 검은 배경을 제거하며, 후광과 빛줄기는 `chest_light.ush`에서 실시간으로 그린다. 원본 SVN 텍스처는 수정하지 않는다.

- `chest_cutout.ush`: 원본 색상 + 물체 투명도. 상자에는 새 후광을 합성하지 않는다.
- `chest_light.ush`: 이미지 샘플링 없는 가산 합성. 영역 끝에 닿기 전에 밝기가 0이 된다.
- `outline_guides.json`: 실제 나무·금속 외곽을 따라 지정한 기준점.
- `exclusion_guides.json`: 자동 분리가 놓친 뚜껑 옆·상자 밑 빛의 수동 제외 영역.
- `build_silhouette.py`: 기준점, OpenCV optical flow/GrabCut, 동전 분리로 4092×2730 마스크를 만든다. 원본 픽셀 해시가 다르면 중단한다.
- `create_separated_materials.py`: 마스크를 G8 UI 텍스처로 가져오고 두 UI 재질을 생성·갱신한다.

재생성에는 Python 3, NumPy, Pillow, opencv-python 4.11이 필요하다. 원본 아틀라스를 Unreal TextureExporterTGA로 내보낸 다음:

```text
python Tools/RewardChest/build_silhouette.py <원본-atlas.tga> SourceArt/UI/RewardChest/chest_silhouette_v1.png --preview <검수-폴더>
UnrealEditor-Cmd.exe P_RD.uproject -run=pythonscript -script=<절대경로>/Tools/RewardChest/create_separated_materials.py -unattended -nop4 -nullrhi
```

최종 마스크는 AI가 다시 그린 외곽을 사용하지 않는다. 원본의 표면 조명·동전 움직임은 유지하지만, 주변의 빛과 구분하기 어려운 초반의 일부 외부 동전은 제외한다. 기준점과 제외 영역 변경 후에는 33프레임을 모두 검수한다.

검증: `P_RD.UI.RewardChestSeparated.TimelineAndRender`는 두 Frameless 변형에서 실제 개봉 시간 전체의 사각 플래시, 레이어 가장자리, 상자 내부의 기존 직선 잘림 위치, 골드 전환과 초기화를 검사하고 캡처를 남긴다. 화면 색상은 실제 게임 캡처로도 확인한다.
