# 동일 EffectLayer 중복 구성 시 수치 과다 적용 가능성 조사

## 배경

스킬 모션 시작 시 모든 EffectLayer가 먼저 시전자 공용 Point Attribute에 값을 적용하고, 실제 트리거에서는 각 EffectLayer가 차례로 `CommitEffect`를 호출한다.

## 예상 시나리오

- 하나의 MotionLayer에 Attack EffectLayer 두 개를 구성한다.
- 두 레이어의 값이 공용 AttackPoint에 합산된다.
- 각 레이어의 Commit이 합산된 전체 AttackPoint를 각각 사용해 의도보다 큰 피해가 적용될 가능성이 있다.
- Heal, Defense, Movement Point 기반 레이어도 유사한 구조인지 확인이 필요하다.

## 근거

- `Source/P_RD/Component/SkillComponent/SkillComponentModel.cpp:266-271`
- `Source/P_RD/Component/SkillComponent/SkillComponentModel.cpp:421-426`
- `Source/P_RD/DataAsset/SkillData/SkillEffectLayer/SkillEffectLayer_Attack.cpp:52-101`

## 확인 항목

- 단일 Attack 레이어와 동일 수치의 Attack 레이어 두 개를 각각 실행해 최종 피해를 비교한다.
- 레이어별 Point를 독립 계산할지, 동일 타입 중복 구성을 데이터 검증에서 금지할지 결정한다.
- Heal, Defense, Movement 레이어에서도 동일 패턴을 확인한다.

## 범위

이 문서는 중복 구성에서 발생할 수 있는 수치 문제를 조사하기 위한 초안이다. 스킬 데이터나 계산 코드는 변경하지 않는다.
