# 적 턴 중 플레이어 명령 유입 가능성 조사

## 배경

월드 터치 입력은 현재 턴 소유자가 플레이어인지 확인한 뒤 처리하지만, 버튼에서 전달되는 전투 명령은 같은 조건을 확인하지 않고 명령 라우터로 전달되는 것으로 보인다. UI 비활성화가 정상 동작하더라도 턴 전환 프레임의 잔여 입력, 지연된 이벤트, 다른 호출 경로가 존재하면 적 턴 컨텍스트에 플레이어용 Move·Skill·EndTurn 명령이 도달할 가능성이 있어 경계 계층의 확인이 필요하다.

이 메모는 정적 코드 검토에서 도출한 조사 가설이며, 실제 입력 유입이나 비정상 종료가 재현되었다고 단정하지 않는다.

## 예상 시나리오

1. 적 턴 시작 직전 또는 진행 중에 플레이어 버튼 입력 이벤트가 전달된다.
2. `HandleCombatCommand`가 현재 턴 소유자를 확인하지 않고 Move, Skill 또는 EndTurn 명령을 생성한다.
3. 명령 라우터가 현재 적 유닛을 Instigator로 액션을 구성할 경우, Move 빌드에서 `UPlayerUnitModel` 캐스팅 결과가 null이 되어 `checkf` 조건에 도달할 수 있다.
4. Skill 또는 EndTurn은 라우터 및 액션 처리 상태에 따라 적 턴을 예상보다 일찍 종료하거나 플레이어 전용 빌드 흐름을 시작할 가능성이 있다.

실제 결과는 UI 입력 차단, 명령 라우터의 추가 검증, 빌드 구성에 따라 달라질 수 있으므로 턴 전환 구간을 포함한 재현이 필요하다.

## 근거

- `Source/P_RD/GameMode/CombatGameMode.cpp:365-425`
  - `HandleCombatCommand`는 버튼 명령을 바로 각 처리 함수로 분기한다.
  - 같은 파일의 `HandleCombatWorldTouch`에는 현재 턴 소유자가 플레이어인지 확인하는 조건이 별도로 존재한다.
- `Source/P_RD/SRPGFramework/SRPGMoveBuildAction.cpp:40-66`
  - MoveSelect 처리에서 Instigator를 `UPlayerUnitModel`로 캐스팅하고 null이면 `checkf` 조건에 도달한다.

## 확인 항목

- 적 턴과 턴 전환 연출 중 Move, Skill, EndTurn 이벤트를 직접 주입해 라우터 도달 여부를 확인한다.
- UI 비활성화 외에 GameMode, CommandRouter 또는 TurnContext에 플레이어 턴 권한 검증이 존재하는지 전체 호출 경로를 확인한다.
- 각 명령이 적 턴에서 처리될 때 액션 Instigator와 턴 종료 상태가 어떻게 변하는지 로그로 기록한다.
- 입력 권한 검증의 책임 계층과 거부 시 반환 결과·UI 복구 정책을 정리한다.
- 개발 및 Shipping 구성에서 `checkf` 경로의 결과가 어떻게 다른지 확인한다.

## 범위

조사를 위한 메모만 추가한다. 입력 처리, 명령 라우팅, 액션 코드는 수정하지 않는다.
