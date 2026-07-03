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
	// 들어온 타이밍이 발동 시점이면: 적용 여부를 묻고 참이면 이펙트 적용
	if (TimingTag == mActivateTimingTag)
	{
		// 수량 조건(quantifier) 게이트: 자격 타겟 수가 기준 미달이면 발동하지 않음
		if (PassesTargetQuantifier(Ctx) == false)
		{
			return;
		}

		// 발동 시 대상에게 가할 기여값을 담을 내부 중간 버퍼
		FBoardCombatTargetSnapshotData Contribution;

		// 내부상태로 적용 여부를 판단하고 다음 상태를 PassiveState에 기록
		// 여기서는 아직 내부상태가 변경되진 않음 (CommitPassive 해야 변경됨)
		if (EvaluateActivate(Ctx, PassiveState, Contribution))
		{
			// 계산된 수치를 이펙트로 적용
			NotifyPassive(Ctx, Contribution);
		}
	}
	// 해제 시점이면: 적용 중인 이펙트를 제거 (핸들 없으면 내부에서 no-op)
	else if (TimingTag == mDeactivateTimingTag)
	{
		DeactivatePassive();
	}
	// 둘 다 아니면 무시
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

	// 이펙트 종류와 발동/해제 시점 태그를 데이터에서 베이스 멤버로 복사
	mEffectClass = mStaticData->mEffectClass.LoadSynchronous();
	mActivateTimingTag = mStaticData->mActivateTimingTag;
	mDeactivateTimingTag = mStaticData->mDeactivateTimingTag;
}

void UTacticalPassive::NotifyPassive(
	const FPassiveActivateContext& Ctx,
	FBoardCombatTargetSnapshotData& TargetDelta)
{
	// 적용할 이펙트가 없으면(계산 전용/무상태 등) 적용하지 않음
	if (mEffectClass == nullptr)
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

	// 대상 속성·연산은 이펙트가 정의. 첫 modifier 속성의 변화량을 크기로 사용(단일 modifier 가정).
	const UTacticalEffect* EffectCDO = mEffectClass.GetDefaultObject();
	if (EffectCDO == nullptr || EffectCDO->mModifiers.Num() == 0)
	{
		return;
	}
	const float* Magnitude = TargetDelta.mAttributes.Find(EffectCDO->mModifiers[0].mAttribute);
	if (Magnitude == nullptr || FMath::IsNearlyZero(*Magnitude))
	{
		// 기여가 없으면(예: 임계 미달 N번째) 이펙트 적용 안 함
		return;
	}

	// 적용할 대상이 없으면 종료
	if (Ctx.mTargets.Num() == 0)
	{
		return;
	}

	// 이전 핸들이 남아있으면 먼저 제거
	if (mActiveHandles.Num() > 0)
	{
		DeactivatePassive();
	}

	// 대상마다 이펙트 적용하면서 핸들을 배열에 저장
	for (const TWeakObjectPtr<UBoardActorModel>& TargetPtr : Ctx.mTargets)
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
		EffectContext->SetInstigator(Ctx.mOwner.Get());
		EffectContext->SetAttributeSetComponentModel(OwnerComp);
		EffectContext->SetAbility(this);

		TSharedPtr<FTacticalEffectSpec> Spec = OwnerComp->MakeOutgoingSpec(mEffectClass, EffectContext);
		Spec->mDynamicMagnitude = *Magnitude;

		mActiveHandles.Add(OwnerComp->ApplyTacticalEffectSpecToTarget(*Spec, TargetComp));
	}

	// TODO: TargetDelta.mTags(발동 여부형 변화) 적용 경로는 미정. 정해지면 추가.
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

bool UTacticalPassive::PassesTargetQuantifier(const FPassiveActivateContext& Ctx) const
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
		// 스냅샷이 짝으로 있으면 넘기고, 없으면 스냅샷과 실제객체가 다르므로 nullptr
		const FBoardCombatTargetSnapshotData* Snapshot =
			Ctx.mTargetSnapshots.IsValidIndex(Index) ? Ctx.mTargetSnapshots[Index] : nullptr;

		if (IsTargetQualified(Snapshot))
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
