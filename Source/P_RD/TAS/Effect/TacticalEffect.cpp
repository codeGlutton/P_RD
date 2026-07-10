#include "TAS/Effect/TacticalEffect.h"

/**
 * @file TacticalEffect.cpp
 * @brief 전술 전투(TAS) 이펙트 시스템의 런타임 구현. 이펙트 스펙(FTacticalEffectSpec)의
 *        모디파이어 크기 계산과 이펙트 정의(UTacticalEffect)의 적용 콜백을 담당한다.
 *
 *        [PR #191 마이그레이션 맥락]
 *        본 시스템은 GAS(GameplayAbilitySystem) 폐기 작업의 일부로, 모디파이어 "연산 종류"를
 *        구 EGameplayModOp 에서 자체 ETacticalModOp(TacticalEffectType.h)로 치환한 결과물이다.
 *        ETacticalModOp 의 정수값은 의도적으로 구 EGameplayModOp 와 동일하게 유지된다.
 *        그 이유는 다음 3가지이며, 이 파일의 연산 인덱싱/직렬화 가정도 모두 이 전제에 기댄다:
 *          (1) 직렬화 호환 - 기존 에셋/세이브 파일에 박힌 정수값을 그대로 보존하기 위함.
 *          (2) 배열 인덱싱 - Aggregator 가 mMods[ETacticalModOp::Max] 처럼 op 값을 배열 인덱스로 사용.
 *          (3) CoreRedirect - DefaultEngine.ini 가 구 enum 이름을 새 enum 이름으로 매핑.
 *        연산 종류: AddBase(0)=합산, MultiplyAdditive(1)=배율 가산, DivideAdditive(2)=나눗셈 가산,
 *        Override(3)=덮어쓰기, MultiplyCompound(4)=거듭제곱 곱, AddFinal(5)=최종 합산, Max(6)=무효/개수.
 *        구 GAS 이름(Additive/Multiplicitive/Division/Override)은 동일 정수값의 하위호환 별칭으로 남아 있다.
 */

/**
 * @brief 이펙트 클래스와 컨텍스트로부터 스펙을 생성하는 생성자.
 *        기본 생성자에 위임(delegating)하여 멤버를 초기화한 뒤 Initialize 로 본격 설정한다.
 * @param Class 이 스펙이 표현할 이펙트 정의(UTacticalEffect).
 * @param EffectContext 이펙트 적용 맥락(시전자/타겟 등)을 담은 컨텍스트.
 */
FTacticalEffectSpec::FTacticalEffectSpec(const UTacticalEffect* Class, UTacticalEffectContext* EffectContext) :
	FTacticalEffectSpec()
{
	Initialize(Class, EffectContext);
}

/**
 * @brief 기존 스펙을 복제하되 컨텍스트만 새 것으로 교체하는 복사 생성자.
 *        값 복사 후 컨텍스트 포인터를 덮어쓴다(원본의 컨텍스트는 공유하지 않음).
 * @param Other 복사 원본이 되는 스펙.
 * @param EffectContext 복사본에 새로 부여할 컨텍스트.
 */
FTacticalEffectSpec::FTacticalEffectSpec(const FTacticalEffectSpec& Other, UTacticalEffectContext* EffectContext)
{
	*this = Other;
	mEffectContext = EffectContext;
}

/**
 * @brief 스펙을 이펙트 정의에 맞게 초기화한다. 지속 정책/컨텍스트/모디파이어 값 버퍼를 세팅.
 * @param Class 이 스펙이 표현할 이펙트 정의(UTacticalEffect). null 이면 안 됨(check).
 * @param EffectContext 이펙트 적용 맥락 컨텍스트.
 */
void FTacticalEffectSpec::Initialize(const UTacticalEffect* Class, UTacticalEffectContext* EffectContext)
{
	mEffectClass = Class;
	SetContext(EffectContext);
	if (IsValid(mEffectClass.Get()) == false)
	{
		mIsInfinite = false;
		mModifierValues.Reset();
		return;
	}

	// 지속 정책이 Infinite 이면 영구 이펙트로 표시(스택/주기 처리 분기에서 사용).
	mIsInfinite = mEffectClass->mDurationPolicy == ETacticalEffectDurationType::Infinite;

	// 모디파이어 개수만큼 평가 결과 값 버퍼를 미리 확보(인덱스가 mModifiers 와 1:1 대응).
	mModifierValues.SetNum(mEffectClass->mModifiers.Num());
}

/**
 * @brief 이 스펙의 현재 스택 수를 설정한다.
 * @param NewStackCount 새로 적용할 스택 수.
 */
void FTacticalEffectSpec::SetStackCount(int32 NewStackCount)
{
	mStackCount = NewStackCount;
}

/**
 * @brief 이펙트 적용 컨텍스트를 교체한다.
 * @param NewEffectContext 새 컨텍스트.
 */
void FTacticalEffectSpec::SetContext(UTacticalEffectContext* NewEffectContext)
{
	mEffectContext = NewEffectContext;
}

/**
 * @brief 현재 스택 수를 반환한다.
 * @return 스택 수.
 */
int32 FTacticalEffectSpec::GetStackCount() const
{
	return mStackCount;
}

/**
 * @brief 현재 이펙트 적용 컨텍스트를 반환한다.
 * @return 컨텍스트 포인터.
 */
UTacticalEffectContext* FTacticalEffectSpec::GetContext() const
{
	return mEffectContext;
}

/**
 * @brief 모든 모디파이어의 단일(스택 미반영) 크기를 미리 계산하여 mModifierValues 에 캐시한다.
 *        이펙트 정의의 정적 크기(mModifierMagnitude)에 런타임 동적 배율(mDynamicMagnitude)을 곱한 값.
 *        스택 반영은 여기서 하지 않고 조회 시점(GetModifierMagnitude)으로 미룬다.
 */
void FTacticalEffectSpec::CalculateModifierMagnitudes()
{
	if (IsValid(mEffectClass.Get()) == false)
	{
		mModifierValues.Reset();
		return;
	}

	if (mModifierValues.Num() != mEffectClass->mModifiers.Num())
	{
		mModifierValues.SetNum(mEffectClass->mModifiers.Num());
	}

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
	if (mModifierValues.IsValidIndex(ModifierIdx) == false || IsValid(mEffectClass.Get()) == false || mEffectClass->mModifiers.IsValidIndex(ModifierIdx) == false)
	{
		UE_LOG(LogAttributeSetComp, Warning, TEXT("Effect ModifierMagnitude 조회 실패: 유효하지 않은 인덱스(%d)"), ModifierIdx);
		return 0.f;
	}

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
