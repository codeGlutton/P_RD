#include "TAS/Effect/TacticalEffect.h"

FTacticalEffectSpec::FTacticalEffectSpec(const UTacticalEffect* Class, UTacticalEffectContext* EffectContext) :
	FTacticalEffectSpec()
{
	Initialize(Class, EffectContext);
}

FTacticalEffectSpec::FTacticalEffectSpec(const FTacticalEffectSpec& Other, UTacticalEffectContext* EffectContext)
{
	*this = Other;
	mEffectContext = EffectContext;
}

void FTacticalEffectSpec::Initialize(const UTacticalEffect* Class, UTacticalEffectContext* EffectContext)
{
	mEffectClass = Class;
	check(mEffectClass != nullptr);

	// 지속 정책이 Infinite 이면 영구 이펙트로 표시(스택/주기 처리 분기에서 사용).
	mIsInfinite = mEffectClass->mDurationPolicy == ETacticalEffectDurationType::Infinite;

	SetContext(EffectContext);
	// 모디파이어 개수만큼 평가 결과 값 버퍼를 미리 확보(인덱스가 mModifiers 와 1:1 대응).
	mModifierValues.SetNum(mEffectClass->mModifiers.Num());
}

void FTacticalEffectSpec::SetStackCount(int32 NewStackCount)
{
	mStackCount = NewStackCount;
}

void FTacticalEffectSpec::SetContext(UTacticalEffectContext* NewEffectContext)
{
	mEffectContext = NewEffectContext;
}

void FTacticalEffectSpec::SetInstigatorSnapshotData(UBoardCombatTargetSnapshotData* SnapshotData)
{
	mInstigatorSnapshotData = SnapshotData;
}

void FTacticalEffectSpec::SetTargetSnapshotData(UBoardCombatTargetSnapshotData* SnapshotData)
{
	mTargetSnapshotData = SnapshotData;
}

int32 FTacticalEffectSpec::GetStackCount() const
{
	return mStackCount;
}

UTacticalEffectContext* FTacticalEffectSpec::GetContext() const
{
	return mEffectContext;
}

UBoardCombatTargetSnapshotData* FTacticalEffectSpec::GetInstigatorSnapshotData() const
{
	return mInstigatorSnapshotData;
}
UBoardCombatTargetSnapshotData* FTacticalEffectSpec::GetTargetSnapshotData() const
{
	return mTargetSnapshotData;
}

void FTacticalEffectSpec::CalculateModifierMagnitudes()
{
	for (int32 ModIdx = 0; ModIdx < mModifierValues.Num(); ++ModIdx)
	{
		const FTacticalModifierInfo& ModDef = mEffectClass->mModifiers[ModIdx];
		float& ModifierValue = mModifierValues[ModIdx];

		// 정적 크기 x 동적 배율 = 스택 1개 기준 단일 모디파이어 크기.
		ModifierValue = ModDef.mModifierMagnitude * mDynamicMagnitude;
	}
}

/**
 * @brief 지정 모디파이어의 최종 크기를 반환한다. 이펙트가 스택 반영을 켰다면 스택 수를 합성한다.
 * @param ModifierIdx 조회할 모디파이어 인덱스(mModifiers / mModifierValues 와 1:1 대응).
 * @return 스택까지 반영된 최종 모디파이어 크기.
 */
float FTacticalEffectSpec::GetModifierMagnitude(int32 ModifierIdx) const
{
	check(mModifierValues.IsValidIndex(ModifierIdx) && mEffectClass != nullptr && mEffectClass->mModifiers.IsValidIndex(ModifierIdx));

	// CalculateModifierMagnitudes 가 캐시한 스택 1개 기준 단일 크기.
	const float SingleEvaluatedMagnitude = mModifierValues[ModifierIdx];
	float ModMagnitude = SingleEvaluatedMagnitude;
	if (mEffectClass->mFactorInStackCount == true)
	{
		/* 스택 반영 */

		// 스택 수에 따른 크기 합성. 연산 종류(mModifierOp: ETacticalModOp)에 따라 합성 방식이 다르다.
		// 합산 계열은 단일크기 x 스택수, MultiplyCompound 는 단일크기^스택수(거듭제곱) 식으로 처리된다.
		// (mModifierOp 는 구 EGameplayModOp 를 ETacticalModOp 로 치환한 PR #191 마이그레이션 지점)
		ModMagnitude = TacticalEffectUtilities::ComputeStackedModifierMagnitude(SingleEvaluatedMagnitude, GetStackCount(), mEffectClass->mModifiers[ModifierIdx].mModifierOp);
	}
	return ModMagnitude;
}

/**
 * @brief 두 모디파이어 정의가 동일한지 비교한다. 대상 어트리뷰트와 연산 종류가 모두 같아야 동일.
 *        (크기 값은 비교 대상이 아님 - "같은 종류의 모디파이어"인지를 판별)
 * @param Other 비교 대상 모디파이어 정의.
 * @return 어트리뷰트와 연산 종류가 모두 같으면 true.
 */
bool FTacticalModifierInfo::operator==(const FTacticalModifierInfo& Other) const
{
	if (mAttribute != Other.mAttribute)
	{
		return false;
	}

	if (mModifierOp != Other.mModifierOp)
	{
		return false;
	}

	return true;
}

/**
 * @brief operator== 의 부정. 두 모디파이어 정의가 다르면 true.
 * @param Other 비교 대상 모디파이어 정의.
 * @return 다르면 true.
 */
bool FTacticalModifierInfo::operator!=(const FTacticalModifierInfo& Other) const
{
	return !(*this == Other);
}

/**
 * @brief 이 이펙트를 대상 컨테이너에 적용 가능한지 판정한다.
 *        구 GAS 에서는 GameplayEffectComponent 들이 각자 적용 가능 여부를 검사했으나,
 *        GAS 폐기(PR #191 라인)로 해당 컴포넌트 루프는 주석 처리되었고 현재는 항상 허용한다.
 * @param ActiveTEContainer 대상이 보유한 활성 이펙트 컨테이너.
 * @param TESpec 적용을 시도하는 이펙트 스펙.
 * @return 적용 가능하면 true(현재 구현은 항상 true).
 */
bool UTacticalEffect::CanApply(const FActiveTacticalEffectsContainer& ActiveTEContainer, const FTacticalEffectSpec& TESpec) const
{
	// GameplayEffectComponent 기반 적용 검사 
	// - GEComponent도 구현해둘까 고민중 by Mohojae
	/*for (const UGameplayEffectComponent* GEComponent : GEComponents)
	{
		if (GEComponent && !GEComponent->CanGameplayEffectApply(ActiveTEContainer, TESpec))
		{
			return false;
		}
	}*/

	return true;
}

/**
 * @brief 이펙트가 활성 컨테이너에 추가될 때 호출되는 콜백. 추가된 이펙트의 활성 유지 여부를 반환한다.
 *        구 GAS 의 GameplayEffectComponent 활성화 훅을 대체하나, GAS 폐기로 컴포넌트 루프는 비활성.
 * @param ActiveTEContainer 이펙트가 추가되는 활성 이펙트 컨테이너.
 * @param ActiveTE 새로 추가된 활성 이펙트 인스턴스.
 * @return 이펙트가 활성으로 유지되어야 하면 true(현재 구현은 항상 true).
 */
bool UTacticalEffect::OnAddedToActiveContainer(FActiveTacticalEffectsContainer& ActiveTEContainer, FActiveTacticalEffect& ActiveTE) const
{
	bool ShouldBeActive = true;
	// GameplayEffectComponent 추가 콜백
	// - GEComponent도 구현해둘까 고민중 by Mohojae
	/*for (const UGameplayEffectComponent* GEComponent : GEComponents)
	{
		if (GEComponent)
		{
			bShouldBeActive = GEComponent->OnActiveGameplayEffectAdded(ActiveTEContainer, ActiveTE) && bShouldBeActive;
		}
	}*/

	return ShouldBeActive;
}

/**
 * @brief 이펙트가 즉시 실행(Instant/주기 실행)될 때 호출되는 콜백.
 *        구 GAS 의 GameplayEffectComponent 실행 훅 자리이나, GAS 폐기로 컴포넌트 루프는 비활성.
 * @param ActiveTEContainer 실행이 일어나는 활성 이펙트 컨테이너.
 * @param TESpec 실행 중인 이펙트 스펙.
 */
void UTacticalEffect::OnExecuted(FActiveTacticalEffectsContainer& ActiveTEContainer, FTacticalEffectSpec& TESpec) const
{
	// GameplayEffectComponent 실행 콜백
	// - GEComponent도 구현해둘까 고민중 by Mohojae
	/*for (const UGameplayEffectComponent* GEComponent : GEComponents)
	{
		if (GEComponent)
		{
			GEComponent->OnGameplayEffectExecuted(ActiveTEContainer, TESpec);
		}
	}*/
}

/**
 * @brief 이펙트가 대상에 적용 완료될 때 호출되는 콜백.
 *        구 GAS 의 GameplayEffectComponent 적용 훅 자리이나, GAS 폐기로 컴포넌트 루프는 비활성.
 * @param ActiveTEContainer 적용이 일어나는 활성 이펙트 컨테이너.
 * @param TESpec 적용된 이펙트 스펙.
 */
void UTacticalEffect::OnApplied(FActiveTacticalEffectsContainer& ActiveTEContainer, FTacticalEffectSpec& TESpec) const
{
	// GameplayEffectComponent 적용 콜백
	// - GEComponent도 구현해둘까 고민중 by Mohojae
	/*for (const UGameplayEffectComponent* GEComponent : GEComponents)
	{
		if (GEComponent)
		{
			GEComponent->OnGameplayEffectApplied(ActiveTEContainer, TESpec);
		}
	}*/
}
