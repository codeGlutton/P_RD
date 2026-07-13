# 유한 수명 턴 제거 시 삭제 노드 접근 가능성 조사

## 배경

`UnregisterTurnImmediately`는 턴 순환 리스트에서 대상 노드를 제거한 다음 턴 맵에서도 같은 ID를 제거한다. 순환 리스트의 기본 `RemoveNode`는 전달받은 노드를 즉시 `delete`한다.

## 예상 시나리오

- `LifeCount`가 설정된 유한 수명 턴이 만료된다.
- `UnregisterTurnImmediately`가 해당 순환 리스트 노드를 제거한다.
- 제거 직후 삭제된 `CurNode`에서 `GetValue()`를 다시 읽으면서 비정상 메모리 접근이 발생할 가능성이 있다.

## 근거

- `Source/P_RD/Singleton/WorldSubsystem/SRPGCombatModel.cpp:322-345`
- `Source/P_RD/Tool/CircularList.h:267-306`
- `RemoveNode(CurNode)` 호출 이후 `CurNode->GetValue()`를 사용한다.

## 확인 항목

- 수명 1인 턴을 등록하고 종료했을 때 제거 경로를 자동화 테스트로 실행한다.
- 제거 전에 TurnId를 값으로 보관하는 방식으로 변경했을 때 동작을 비교한다.
- 단일 노드와 복수 노드 순환 리스트에서 각각 확인한다.

## 범위

이 문서는 발생 가능성을 기록하기 위한 조사 초안이다. 메모리 문제의 확정이나 게임플레이 코드 수정은 포함하지 않는다.
