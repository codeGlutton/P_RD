# SkillCast 명령 최종 유효성 검증 부족 가능성 조사

## 배경

`USRPGSkillAction::HandleCommand`는 SkillCast 명령에서 전달된 스킬 인덱스, 타겟 타일과 주사위 합을 `ActivateSkill`로 넘긴다. 명령 실행 경계에서 조준 가능 범위, 주사위 비용, 타겟 유효성을 다시 확인하는 단계는 명확하지 않다.

## 예상 시나리오

- UI 상태가 변경된 직후 이전 프리뷰에서 만든 SkillCast 명령이 도착한다.
- Blueprint 또는 AI가 잘못된 인덱스, 범위 밖 타겟, 비정상 주사위 합을 전달한다.
- 빌드 단계의 사전 검증에만 의존해 assert가 발생하거나 현재 규칙과 다른 시전이 실행될 가능성이 있다.

## 근거

- `Source/P_RD/SRPGFramework/SRPGSkillAction.cpp:41-76`
- `Source/P_RD/Component/SkillComponent/SkillComponentModel.cpp:138-163`
- `ActivateSkill`은 인덱스와 빈 슬롯을 `checkf`로 검증하지만 조준/비용 규칙을 다시 계산하지 않는다.

## 확인 항목

- 잘못된 스킬 인덱스, 빈 슬롯, 범위 밖 타겟, 음수 및 과도한 DiceSum 명령을 각각 실행한다.
- UI, AI와 무관한 모델 계층의 authoritative validation 위치를 정한다.
- 실패 시 assert 대신 명령 거부 결과를 반환해야 하는지 확인한다.

## 범위

이 문서는 명령 경계의 방어 검증 필요성을 확인하기 위한 조사 초안이다. SkillAction 및 SkillComponent 코드는 변경하지 않는다.
