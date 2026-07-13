# Owner 기반 TurnContext 조회 실패 가능성 조사

## 배경

`USRPGCombatModel::GetTurnContext`는 특정 유닛이 소유한 턴 컨텍스트를 찾기 위한 API다. 현재 구현은 순회용 `HeadNode`를 이동시키지만, 실제 비교와 반환에서는 `mCurTurnContextOrder`를 계속 사용한다.

## 예상 시나리오

- 현재 턴 소유자가 아닌 유닛으로 `GetTurnContext`를 호출한다.
- 해당 유닛의 턴이 등록돼 있어도 현재 턴만 반복 검사한다.
- 호출자는 등록된 턴이 없다고 판단하거나 `nullptr` 경로를 탈 가능성이 있다.

## 근거

- `Source/P_RD/Singleton/WorldSubsystem/SRPGCombatModel.cpp:666-677`
- 루프 안에서 `HeadNode`는 갱신되지만 조회 키에는 사용되지 않는다.

## 확인 항목

- 현재 턴, 다음 턴, 복수 턴을 가진 유닛 각각에 대한 반환값을 자동화 테스트로 확인한다.
- 순회 노드의 값을 사용하도록 변경했을 때 호출부 동작을 확인한다.
- 등록되지 않은 유닛은 계속 `nullptr`을 반환하는지 확인한다.

## 범위

이 문서는 발생 가능성을 기록하기 위한 조사 초안이다. 게임플레이 코드 수정은 포함하지 않는다.
