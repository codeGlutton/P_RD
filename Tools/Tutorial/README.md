# 튜토리얼 첫 방 DA

## 편집 대상

`Content/BP/DataAsset/Room/Turtorial/DA_Tutorial_FirstBattle.uasset`

- 클래스: `StaticTutorialRoomSpawnData` (일반 MonsterRoom을 상속)
- `StageLevel`: `None`. 일반 스테이지의 기존 레벨 필터에서 제외된다.
- `BackgroundMap`, `DefaultSpawnSettingName`: 같은 배경의 고정 스폰 설정을 사용한다.
- `PlayerTransform`: 0번 파티원이 조작 실습 대상이다. 세 파티원의 위치를 편집한다.
- `EnemyUnitPlacementDatas`: 적 종류와 위치를 편집한다. 턴 종료 안내 전 조기 승리를 막기 위해 두 마리 이상 유지한다.
- `ObstaclePlacementDatas`: 장애물을 편집한다. 초기 튜토리얼은 장애물 없는 연습 보드다.
- `MoveDestination`: 이동 실습의 목적지. 0번 파티원이 이동할 수 있는 빈칸이어야 한다.
- `TargetEnemyIndex`: 공격 실습 대상의 배열 인덱스. 해당 적의 실제 위치를 안내와 입력 제한이 함께 참조한다.

일반 첫 방 DA는 수정하지 않았다. 튜토리얼 DA는 별도의 배경 맵 파일을 복제하지 않고 기존 배경을 참조한다.
생성 스크립트는 최초 DA 제작용이며 게임 실행 중에는 사용하지 않는다. 이미 존재하는 DA를 덮어쓰지 않는다.

## 선택 및 저장 흐름

신규 런의 Stage1 생성 시 튜토리얼 미완료·미건너뛰기 프로필에만 `SetFirstRoomOverride`를 전달한다.
Stage Builder는 정상 생성 후 시작 방의 ID만 교체한다. 이후 방과 보상의 랜덤 스트림 소비는 유지한다.
선택한 ID를 런에 저장하므로 이어하기도 같은 DA의 배경과 배치를 사용한다.

조작 안내 마지막 단계인 턴 종료를 마치면 기존 `CoreComplete` 저장값으로 다음 런의 고정을 해제한다.
전투를 이기기 전에 런을 포기해도 완료 기록은 유지된다. 진행 중인 방을 완료 순간에 재배치하지 않는다.
이전 버전의 일반 방 ID로 저장된 진행 중 런은 그대로 유지하며, 런타임에서 배치를 덮어쓰지 않는다.

`Stage.FixedMonsterRoom`은 모든 일반 몬스터 방에 적용되는 기존 디버깅 기능이다. 튜토리얼 선택에 사용하지 않는다.

## 검증

에셋의 Validate Assets에서 `None`, 고정 스폰 설정, 파티/적 배치, 이동 목적지와 장애물의 회전된 추가 점유 칸을 검사한다.
안내 이동 경로와 스킬 사거리는 배치를 바꾼 뒤 실제 플레이로도 확인한다.

자동화 테스트:

`P_RD.Tutorial.RoomData` / `P_RD.Tutorial.Guided.FixedScenario` / `P_RD.Tutorial.Guided.EnemyColdSkills`

확인할 수동 시나리오:

1. 새 프로필에서 새로하기 → 전용 방의 독수리 두 마리와 장애물 없는 보드 확인.
2. 정보 안내 → 이동 → 스킬 확인 → 공격 → 턴 종료까지 진행.
3. 전투 승리 전에 런 포기 → 새로하기 → 일반 DA의 배치 확인.
4. 새로 생성된 일반 런 저장 후 이어하기 → 동일한 방 유지 확인.

일반 랜덤 추첨은 같은 방이 연속으로 나올 수 있다. 동일 배경만으로 고정 여부를 판단하지 않는다.

## 로컬 Fold5 검증본

테스트 패키지는 별도 빌드 사본에서 `com.aurelight.mercenaryguildoftheruinedkingdom.tutorialtest`로 생성한다.
표시 이름은 `폐허왕국 튜토리얼 테스트`, Development APK이며 기존 Play 앱과 별도의 저장 공간을 사용한다.
테스트 패키지명과 빌드 설정은 배포 프로젝트 설정에 반영하지 않는다. Play 업로드·출시는 수행하지 않는다.
