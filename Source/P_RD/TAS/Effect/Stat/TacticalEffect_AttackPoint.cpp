/*****************************************************************//**
 * @file   TacticalEffect_AttackPoint.cpp
 * @brief  공격력(AttackPoint) 가산 이펙트 구현
 * @author 이문환
 * @date   2026-06-26
 *********************************************************************/

#include "TAS/Effect/Stat/TacticalEffect_AttackPoint.h"

// AttackPoint 속성 캡처용. GetAttackPointAttribute()로 모디파이어 대상 속성을 지정한다.
#include "AttributeSet/UnitAttributeSet.h"

/**
 * @brief 공격력(AttackPoint) 가산 이펙트의 기본 형태를 정의하는 생성자
 *
 * @details
 * 이 이펙트는 "공격력에 어떤 연산을 어떤 속성으로 가하는지"의 골격(템플릿)만 정의한다.
 * 실제 더해질 수치(배율)는 생성 시점에 고정하지 않고, 적용하는 측이
 * mDynamicMagnitude를 통해 런타임에 주입한다. 따라서 여기서는 크기를 1.f로 두고
 * 대상 속성과 연산 종류만 박아 둔다.
 *
 * @par [PR #191] 연산 종류 enum 마이그레이션 (EGameplayModOp -> ETacticalModOp)
 * 본 PR은 GAS 폐기 작업의 일부로, 모디파이어 "연산 종류"를 GAS의 EGameplayModOp에서
 * 자체 정의한 ETacticalModOp(TacticalEffectType.h)로 치환한다.
 * ETacticalModOp의 정수값은 구 EGameplayModOp와 동일하게 유지되며, 그 이유는 다음 3가지다.
 *   (1) 직렬화 호환: 기존 에셋/세이브 데이터에 박힌 정수값을 그대로 보존하기 위함.
 *   (2) 배열 인덱싱: Aggregator가 mMods[ETacticalModOp::Max]처럼 op 값 자체를
 *       배열 인덱스로 사용하므로, 값이 연속/고정이어야 한다.
 *   (3) CoreRedirect: DefaultEngine.ini의 CoreRedirect가 구 enum 이름을
 *       새 enum 이름으로 매핑하여 로드 시 자동 치환되도록 한다.
 *
 * @note 연산 종류별 의미(정수값):
 *       AddBase(0)=합산, MultiplyAdditive(1)=배율 가산, DivideAdditive(2)=나눗셈 가산,
 *       Override(3)=덮어쓰기, MultiplyCompound(4)=거듭제곱 곱, AddFinal(5)=최종 합산,
 *       Max(6)=무효/개수. 하위호환 별칭(구 GAS 이름): Additive=0 / Multiplicitive=1 /
 *       Division=2 / Override=3.
 */

UTacticalEffect_AttackPoint::UTacticalEffect_AttackPoint()
{
	// 지속: 영구(핸들로 적용/해제) · 스택: 없음
	mDurationPolicy = ETacticalEffectDurationType::Infinite;
	mStackingType = ETacticalEffectStackingType::None;

	// 대상 속성/연산만 정의 (크기 1, 실제 배율은 적용 측이 mDynamicMagnitude로 주입)
	FTacticalModifierInfo Info;
	// 모디파이어가 건드릴 속성: 공격력(AttackPoint). UnitAttributeSet의 정적 접근자로 캡처한다.
	Info.mAttribute = UUnitAttributeSet::GetAttackPointAttribute();
	// 연산 종류: ETacticalModOp::Additive(==AddBase, 정수값 0) — 기준값에 합산.
	// [PR #191] 구 EGameplayModOp::Additive를 대체. Additive는 AddBase(0)의 하위호환 별칭으로,
	// 정수값이 동일하므로 직렬화/CoreRedirect 호환이 유지된다. 즉 "공격력 += magnitude" 연산이다.
	Info.mModifierOp = ETacticalModOp::Additive;
	// 크기는 1.f로 고정(템플릿값). 실제 더해질 수치는 적용 측이 mDynamicMagnitude로 덮어쓰므로
	// 여기 값 자체는 의미가 없고, "유효한 모디파이어 1개"를 만들기 위한 자리표시자에 가깝다.
	Info.mModifierMagnitude = 1.f;

	// 정의한 모디파이어를 이펙트의 모디파이어 목록에 등록.
	mModifiers.Add(Info);
}
