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

void UTacticalPassive::ActivatePassive(
	const FPassiveActivateContext& Ctx,
	TInstancedStruct<FTacticalPassiveState>& PassiveState)
{
	// 계산 결과를 담을 내부 중간 버퍼 (외부로 반환하지 않음)
	FBoardCombatTargetSnapshotData Contribution;

	// 계산: 기여값 산출 + 러닝 상태 갱신 (실제 내부 상태는 여기서 안 바꿈)
	EvaluatePassive(Ctx, Contribution, PassiveState);
	// 적용: 계산된 기여값을 대상 이펙트로 적용
	NotifyPassive(Ctx, Contribution);
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

	// 소유자(시전 주체)와 대상의 속성 컴포넌트 획득. 자기버프면 둘이 동일.
	IBoardCombatTarget* OwnerTarget = Cast<IBoardCombatTarget>(Ctx.mOwner.Get());
	IBoardCombatTarget* HitTarget = Cast<IBoardCombatTarget>(Ctx.mTarget.Get());
	if (OwnerTarget == nullptr || HitTarget == nullptr)
	{
		return;
	}
	UAttributeSetComponentModel* OwnerComp = OwnerTarget->GetAttributeComponentModel();
	UAttributeSetComponentModel* TargetComp = HitTarget->GetAttributeComponentModel();
	if (OwnerComp == nullptr || TargetComp == nullptr)
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

	// 단일 핸들 전제: 이전 적용분이 남아있으면 먼저 제거(직렬 보장)
	if (mActiveHandle.IsValid())
	{
		DeactivatePassive();
	}

	// 소유자를 시전자로 spec 생성 -> 계산된 크기를 배율로 주입 -> 대상에 적용 -> 핸들 저장
	UTacticalEffectContext* EffectContext = OwnerComp->MakeEffectContext();
	EffectContext->SetInstigator(Ctx.mOwner.Get());
	EffectContext->SetAttributeSetComponentModel(OwnerComp);

	TSharedPtr<FTacticalEffectSpec> Spec = OwnerComp->MakeOutgoingSpec(mEffectClass, EffectContext);
	Spec->mDynamicMagnitude = *Magnitude;

	mActiveHandle = OwnerComp->ApplyTacticalEffectSpecToTarget(*Spec, TargetComp);

	// TODO: TargetDelta.mTags(발동 여부형 변화) 적용 경로는 미정. 정해지면 추가.
}

void UTacticalPassive::DeactivatePassive()
{
	if (mActiveHandle.IsValid() == false)
	{
		return;
	}

	// 핸들로 이펙트를 활성 집합에서 제거 -> TAS가 base에서 재계산(기여분을 산술로 되돌리지 않음)
	UAttributeSetComponentModel* OwningComp = mActiveHandle.GetOwningAttributeSetComponentModel();
	if (OwningComp != nullptr)
	{
		OwningComp->RemoveActiveTacticalEffect(mActiveHandle);
	}
	mActiveHandle.Reset();
}
