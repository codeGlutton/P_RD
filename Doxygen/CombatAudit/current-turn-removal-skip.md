# 현재 턴 유닛 제거 후 다음 턴 스킵 가능성 조사

## 배경

턴 소유자 제거 요청은 pending 큐에 보관되었다가 `AdvanceTurn`에서 처리된다. 현재 턴 소유자의 노드를 제거하는 경로는 `mCurTurnContextOrder`를 제거된 노드의 다음 노드로 옮기며, 이후 `AdvanceTurn`이 같은 포인터를 다시 다음 노드로 전진시킨다. 두 단계가 연속 실행될 때 원래 다음 차례였던 턴을 건너뛸 가능성이 있어 확인이 필요하다.

이 메모는 포인터 이동 순서를 바탕으로 한 조사 가설이다. 전투 종료 조건이나 다른 제거 경로가 개입할 수 있으므로 실제 턴 스킵이 발생한다고 확정하지 않는다.

## 예상 시나리오

1. 순환 턴 목록이 현재 턴 `A`, 다음 턴 `B`, 그다음 턴 `C` 순서인 상태에서 `A` 소유자 제거 요청이 pending 큐에 들어간다.
2. `AdvanceTurn(false)`가 `FlushPendingTurnRequests`를 호출한다.
3. `UnregisterTurnsImmediately`가 현재 노드 `A`를 제거하고 `mCurTurnContextOrder`를 `B`로 변경한다.
4. `AdvanceTurn`의 턴 변경 구문이 `mCurTurnContextOrder->GetNextNode()`를 다시 적용해 `C`를 선택할 가능성이 있다.
5. 이 경우 `B`의 턴 시작 처리가 한 번 누락된 것처럼 보일 수 있다.

현재 소유자 제거 이후 전투가 즉시 종료되지 않고 유효한 턴 노드가 둘 이상 남는 조건에서 재현 여부를 확인해야 한다.

## 근거

- `Source/P_RD/Singleton/WorldSubsystem/SRPGCombatModel.cpp:348-399`
  - `UnregisterTurnsImmediately`는 제거 대상이 현재 턴이면 `mCurTurnContextOrder`를 `RemoveNode`가 반환한 다음 노드로 변경한다.
  - pending 소유자 제거 요청은 `FlushPendingTurnRequests`에서 이 경로로 처리된다.
- `Source/P_RD/Singleton/WorldSubsystem/SRPGCombatModel.cpp:579-591`
  - `AdvanceTurn(false)`는 pending 요청을 flush한 뒤 `mCurTurnContextOrder`를 다시 다음 노드로 변경한다.

## 확인 항목

- `A → B → C` 순서에서 현재 소유자 `A`를 pending 제거하고 다음에 시작되는 TurnId가 `B`인지 기록한다.
- 자기 턴 중 사망, 지속 피해, 자폭 등 현재 소유자 제거 요청이 생성될 수 있는 실제 호출 경로를 확인한다.
- 단일 TurnContext 제거와 소유자의 모든 TurnContext 제거가 같은 결과를 내는지 구분해 시험한다.
- 제거 후 남은 노드가 0개, 1개, 2개 이상인 경우와 전투 종료가 동시에 결정되는 경우를 나눠 확인한다.
- 현재 노드 제거와 정상 턴 전진의 책임을 어느 함수가 가져야 하는지 정리하고 회귀 테스트 필요성을 검토한다.

## 범위

조사를 위한 메모만 추가한다. 턴 목록, 제거 요청, 전투 종료 코드는 수정하지 않는다.
