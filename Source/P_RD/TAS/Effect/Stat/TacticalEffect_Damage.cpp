/**
 * @file   TacticalEffect_Damage.cpp
 * @brief  HP 어트리뷰트에 고정 피해(-1)를 즉시 가하는 택티컬 이펙트의 구현부.
 *         GAS 폐기 마이그레이션(PR #191)의 일부로, 모디파이어 "연산 종류"를
 *         GAS의 EGameplayModOp가 아닌 자체 ETacticalModOp로 지정한다.
 * @author 박용수
 * @date   2026-06-26
 */

#include "TAS/Effect/Stat/TacticalEffect_Damage.h"
#include "AttributeSet/UnitAttributeSet.h"

/**
 * @brief  기본 생성자. 즉시 적용(Instant)·비스택(None) 정책으로 HP -1 모디파이어 1개를 구성한다.
 * @note   [마이그레이션 핵심] 본래 이 모디파이어의 연산 종류는 GAS의 EGameplayModOp::Additive였으나,
 *         GAS 폐기에 따라 자체 enum ETacticalModOp::Additive로 치환되었다.
 *         ETacticalModOp의 정수값은 구 EGameplayModOp와 동일하게 유지되므로
 *         (1) 기존 에셋/세이브에 박힌 정수값이 그대로 직렬화 호환되고,
 *         (2) Aggregator가 op 값으로 mMods 배열을 인덱싱하는 동작이 깨지지 않으며,
 *         (3) DefaultEngine.ini의 CoreRedirect가 구 enum 이름을 새 enum으로 매핑할 수 있다.
 *         ETacticalModOp::Additive(=0)는 AddBase(=0)의 구 GAS 이름 별칭으로, "합산" 연산을 의미한다.
 */
UTacticalEffect_Damage::UTacticalEffect_Damage()
{
	// 즉시(Instant) 적용: 듀레이션 없이 1회성으로 어트리뷰트 BaseValue에 직접 반영된다.
	mDurationPolicy = ETacticalEffectDurationType::Instant;
	// 비스택(None): 동일 이펙트가 중첩되어도 스택 수에 따른 크기 보정을 적용하지 않는다.
	mStackingType = ETacticalEffectStackingType::None;

	FTacticalModifierInfo Info;
	// 대상 어트리뷰트: 유닛의 HP. (정적 접근자 GetHPAttribute로 FTacticalAttribute 핸들을 획득)
	Info.mAttribute = UUnitAttributeSet::GetHPAttribute();
	// 연산 종류: Additive(=AddBase=0, 합산). 구 EGameplayModOp::Additive에서 치환된 지점.
	//            합산 연산이므로 모디파이어 바이어스(항등값)는 0이고, 크기가 그대로 더해진다.
	Info.mModifierOp = ETacticalModOp::Additive;
	// 크기: -1.f → HP에 -1을 합산하여 1만큼의 고정 피해를 가한다.
	Info.mModifierMagnitude = -1.f;

	// 구성한 모디파이어를 이펙트의 모디파이어 목록에 등록한다(이 이펙트는 HP -1 모디파이어 1개만 가진다).
	mModifiers.Add(Info);
}
