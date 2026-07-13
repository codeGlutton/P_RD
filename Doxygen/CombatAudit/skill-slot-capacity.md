# 6개 초과 스킬 로딩 시 슬롯 범위 오류 가능성 조사

## 배경

`USkillComponentModel`은 생성 시 여섯 개 슬롯을 미리 만들지만, `SetSkillFrom`과 적 초기화 코드는 입력 스킬 개수만큼 계속 `SetSkill`을 호출한다. 코드 주석은 몬스터가 여섯 개 이상의 스킬을 가질 수 있다고 설명한다.

## 예상 시나리오

- 유닛 스폰 데이터에 일곱 개 이상의 스킬을 설정한다.
- 초기화가 일곱 번째 스킬을 인덱스 6에 배치하려 한다.
- 배열 크기 확장 없이 `SetSkill`의 인덱스 검증을 통과하지 못해 assert가 발생할 가능성이 있다.

## 근거

- `Source/P_RD/Component/SkillComponent/SkillComponentModel.cpp:86-114`
- `Source/P_RD/Component/SkillComponent/SkillComponentModel.cpp:128-130`
- `Source/P_RD/Pawn/Enemy/EnemyUnitModel.cpp:38-48`

## 확인 항목

- 플레이어와 적 각각에 6개, 7개 스킬을 설정해 초기화 결과를 비교한다.
- 고정 6슬롯이 기획 제약인지, 동적 확장이 필요한지 확인한다.
- 고정 제약이라면 에셋 검증 단계에서 초과 입력을 차단할지 결정한다.

## 범위

이 문서는 스킬 슬롯 용량 계약을 확인하기 위한 조사 초안이다. 슬롯 수나 초기화 코드는 변경하지 않는다.
