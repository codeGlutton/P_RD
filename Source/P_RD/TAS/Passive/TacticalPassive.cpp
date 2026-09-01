/*****************************************************************//**
 * @file   TacticalPassive.cpp
 * @brief  패시브 베이스 클래스 구현
 * @author 이문환
 * @date   2026-06-25
 *********************************************************************/

#include "TAS/Passive/TacticalPassive.h"

#include "TAS/Passive/PassiveActivateContext.h"
#include "Actor/BoardActor/BoardActorModel.h"
#include "Actor/BoardActor/BoardCombatTarget.h"
#include "Component/AttributeComponent/AttributeSetComponentModel.h"
#include "TAS/Effect/TacticalEffect.h"
#include "TAS/Effect/TacticalEffectContext.h"
#include "DataAsset/PassiveData/StaticPassiveData.h"

void UTacticalPassive::ActivatePassive(
	const FGameplayTag& TimingTag,
	const FPassiveActivateContext& Ctx,
	TInstancedStruct<FDynamicPassiveData>& PassiveState)
{
	// 상태 작업본 준비: 비어 있으면 커밋본 복사, 커밋본도 없으면 최초 생성 (무상태 패시브는 빈 상태 유지)
	if (!PassiveState.IsValid())
	{
		if (mState.IsValid())
		{
			PassiveState = mState;
		}
		else
		{
			InitializeState(PassiveState);
		}
	}

	// 같은 태그가 여러 시점에 걸릴 수 있으므로 else-if 없이 순서대로 전부 처리 (리셋 → 캡처 → 발동 → 해제)

	// 카운터 리셋 시점: 내부 카운터 초기화
	if (TimingTag == mCounterResetTimingTag)
	{
		OnCounterReset(PassiveState);
	}

	// 캡처 시점: 비교용 값 저장
	if (TimingTag == mCaptureTimingTag)
	{
		OnCapture(Ctx, PassiveState);
	}

	// 발동 시점: 발동 경로 처리 (게이트 → 계산 → 적용)
	if (TimingTag == mActivateTimingTag)
	{
		OnActivate(Ctx, PassiveState);
	}

	// 해제 시점: 적용 중인 이펙트를 제거 (핸들 없으면 내부에서 no-op)
	if (TimingTag == mDeactivateTimingTag)
	{
		DeactivatePassive();
	}
}

void UTacticalPassive::OnActivate(
	const FPassiveActivateContext& Ctx,
	TInstancedStruct<FDynamicPassiveData>& PassiveState)
{
	// 수량 조건(quantifier) 게이트: 자격 타겟 수가 기준 미달이면 발동하지 않음
	if (PassesTargetQuantifier(Ctx, PassiveState) == false)
	{
		return;
	}

	// 발동 시 대상에게 가할 기여값(크기)
	float ContributionMagnitude = 0.f;

	// 내부상태로 적용 여부를 판단하고 다음 상태를 PassiveState에 기록
	// 여기서는 아직 내부상태가 변경되진 않음 (CommitPassive 해야 변경됨)
	if (EvaluateActivate(Ctx, PassiveState, ContributionMagnitude))
	{
		// 계산된 수치를 이펙트로 적용
		NotifyPassive(Ctx, ContributionMagnitude);
	}
}

void UTacticalPassive::SetStaticData(UStaticPassiveData* InStaticData)
{
	// 정적 데이터 주입 후, 공통 필드(이펙트/타이밍)를 데이터에서 채움
	mStaticData = InStaticData;
	InitializeFromData();
}

void UTacticalPassive::InitializeFromData()
{
	// 데이터가 없으면(클래스 구동 패시브) 아무 것도 하지 않음
	if (mStaticData == nullptr)
	{
		return;
	}

	// 이펙트 종류와 시점 태그 4종(발동/해제/리셋/캡처)을 데이터에서 베이스 멤버로 복사
	mEffectClass = mStaticData->mEffectClass.LoadSynchronous();
	mActivateTimingTag = mStaticData->mActivateTimingTag;
	mDeactivateTimingTag = mStaticData->mDeactivateTimingTag;
	mCounterResetTimingTag = mStaticData->mCounterResetTimingTag;
	mCaptureTimingTag = mStaticData->mCaptureTimingTag;
}

void UTacticalPassive::NotifyPassive(
	const FPassiveActivateContext& Ctx,
	const float Magnitude)
{
	// 구 경로 호환: 베이스 멤버 이펙트를 타겟들에게 적용
	NotifyPassive(Ctx, mEffectClass, Magnitude, EPassiveEffectTarget::Targets);
}

void UTacticalPassive::NotifyPassive(
	const FPassiveActivateContext& Ctx,
	TSubclassOf<UTacticalEffect> EffectClass,
	const float Magnitude,
	const EPassiveEffectTarget EffectTarget,
	const bool bRefreshPrevious)
{
	// 적용할 이펙트가 없으면(계산 전용/무상태 등) 적용하지 않음
	if (EffectClass == nullptr)
	{
		return;
	}

	// 기여가 없으면(변화량 0) 굳이 이펙트 적용할 필요가 없으므로 바로 리턴
	// @note 이펙트 적용을 안하니까 이펙트 핸들을 갖지 않으므로 딱히 부작용은 없음
	if (FMath::IsNearlyZero(Magnitude))
	{
		return;
	}

	// 소유자(시전 주체)의 속성 컴포넌트 획득. 대상과 무관하게 1회만 조회.
	IBoardCombatTarget* OwnerTarget = Cast<IBoardCombatTarget>(Ctx.mOwner.Get());
	if (OwnerTarget == nullptr)
	{
		return;
	}
	UAttributeSetComponentModel* OwnerComp = OwnerTarget->GetAttributeComponentModel();
	if (OwnerComp == nullptr)
	{
		return;
	}

	// 적용 대상 목록: Self면 소유자 1명, Targets면 Ctx.mTargets 전부
	TArray<TWeakObjectPtr<UBoardActorModel>> SelfOnly;
	if (EffectTarget == EPassiveEffectTarget::Self)
	{
		SelfOnly.Add(Ctx.mOwner);
	}
	const TArray<TWeakObjectPtr<UBoardActorModel>>& ApplyTargets =
		(EffectTarget == EPassiveEffectTarget::Self) ? SelfOnly : Ctx.mTargets;

	// 적용할 대상이 없으면 종료
	if (ApplyTargets.Num() == 0)
	{
		return;
	}

	// 이전 핸들이 남아있으면 먼저 제거 (효과 배열 적용 중에는 호출자가 false로 끔)
	if (bRefreshPrevious && mActiveHandles.Num() > 0)
	{
		DeactivatePassive();
	}

	// 대상마다 이펙트 적용하면서 핸들을 배열에 저장
	for (const TWeakObjectPtr<UBoardActorModel>& TargetPtr : ApplyTargets)
	{
		// 대상의 속성 컴포넌트 획득.
		// 유효하지 않은 대상은 건너뜀.
		IBoardCombatTarget* HitTarget = Cast<IBoardCombatTarget>(TargetPtr.Get());
		if (HitTarget == nullptr)
		{
			continue;
		}
		UAttributeSetComponentModel* TargetComp = HitTarget->GetAttributeComponentModel();
		if (TargetComp == nullptr)
		{
			continue;
		}

		// 소유자를 시전자로 spec 생성 -> 계산된 크기를 배율로 주입 -> 대상에 적용 -> 핸들 저장
		UTacticalEffectContext* EffectContext = OwnerComp->MakeEffectContext();
		EffectContext->SetAbility(this);

		TSharedPtr<FTacticalEffectSpec> Spec = OwnerComp->MakeOutgoingSpec(EffectClass, EffectContext);
		Spec->mDynamicMagnitude = Magnitude;

		mActiveHandles.Add(OwnerComp->ApplyTacticalEffectSpecToTarget(*Spec, TargetComp));
	}
}

void UTacticalPassive::DeactivatePassive()
{
	if (mActiveHandles.Num() == 0)
	{
		return;
	}

	// 핸들마다 이펙트를 활성 집합에서 제거 -> TAS가 base에서 재계산(기여분을 산술로 되돌리지 않음)
	for (FActiveTacticalEffectHandle& Handle : mActiveHandles)
	{
		if (Handle.IsValid() == false)
		{
			continue;
		}
		UAttributeSetComponentModel* OwningComp = Handle.GetOwningAttributeSetComponentModel();
		if (OwningComp != nullptr)
		{
			OwningComp->RemoveActiveTacticalEffect(Handle);
		}
	}

	// 배치 핸들 전체 비움
	mActiveHandles.Reset();
}

bool UTacticalPassive::PassesTargetQuantifier(
	const FPassiveActivateContext& Ctx,
	const TInstancedStruct<FDynamicPassiveData>& State) const
{
	// 판정 대상이 없으면(자기 대상 등) 게이트 통과
	const int32 TargetNum = Ctx.mTargets.Num();
	if (TargetNum == 0)
	{
		return true;
	}

	// 타겟별 자격 조건(predicate)으로 자격 타겟 수 집계
	int32 QualifiedNum = 0;
	for (int32 Index = 0; Index < TargetNum; ++Index)
	{
		if (IsTargetQualified(Ctx, Index, State))
		{
			++QualifiedNum;
		}
	}

	// 수량 조건 값: 기본값은 Any
	const EPassiveTargetQuantifier Quantifier = (mStaticData != nullptr)
		? mStaticData->mTargetQuantifier
		: EPassiveTargetQuantifier::Any;

	switch (Quantifier)
	{
	case EPassiveTargetQuantifier::All:
		return QualifiedNum == TargetNum;
	case EPassiveTargetQuantifier::Any:
	default:
		return QualifiedNum >= 1;
	}
}
