# Cancel 및 LongPressUnit 명령 미처리 가능성 조사

## 배경

`UCombatUIModel`은 `Cancel`과 `LongPressUnit` 입력을 `OnCombatCommand`로 전달한다. 실제 게임플레이 연결점인 `ACombatGameMode::HandleCombatCommand`의 switch에는 두 입력에 대한 분기가 보이지 않는다.

## 예상 시나리오

- UI가 현재 스킬 또는 이동 빌드를 취소하기 위해 `RequestCancel`을 호출한다.
- 또는 UnitId 기반 상세 정보를 요청하기 위해 `RequestLongPressUnit`을 호출한다.
- 명령은 방송되지만 GameMode에서 처리되지 않아 선택 상태가 남거나 상세 패널이 갱신되지 않을 가능성이 있다.

## 근거

- `Source/P_RD/UI/Combat/CombatUIModel.cpp:35-56`
- `Source/P_RD/GameMode/CombatGameMode.cpp:365-392`
- 월드 롱프레스는 별도 `OnCombatWorldTouch` 경로에서 처리된다.

## 확인 항목

- 실제 WBP가 두 Request API를 호출하는지 확인한다.
- 취소 책임을 WorldTouch의 무효 타일 처리로 일원화할지 명령 분기를 추가할지 결정한다.
- UnitId 기반 상세 요청과 월드 롱프레스 기반 상세 요청 중 유지할 계약을 정한다.

## 범위

이 문서는 UI와 게임플레이 명령 계약의 불일치 가능성을 기록하는 조사 초안이다. UI 및 GameMode 코드는 변경하지 않는다.
