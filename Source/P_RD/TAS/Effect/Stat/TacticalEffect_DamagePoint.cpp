// Fill out your copyright notice in the Description page of Project Settings.


#include "TAS/Effect/Stat/TacticalEffect_DamagePoint.h"
#include "AttributeSet/UnitAttributeSet.h"


UTacticalEffect_DamagePoint::UTacticalEffect_DamagePoint()
{
	// 지속: 영구(핸들로 적용/해제) · 스택: 없음
	mDurationPolicy = ETacticalEffectDurationType::Infinite;
	mStackingType = ETacticalEffectStackingType::None;

	// 대상 속성/연산만 정의 (크기 1, 실제 배율은 적용 측이 mDynamicMagnitude로 주입)
	FTacticalModifierInfo Info;
	// 모디파이어가 건드릴 속성: 공격력(AttackPoint). UnitAttributeSet의 정적 접근자로 캡처한다.
	Info.mAttribute = UUnitAttributeSet::GetDamagePointAttribute();
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
