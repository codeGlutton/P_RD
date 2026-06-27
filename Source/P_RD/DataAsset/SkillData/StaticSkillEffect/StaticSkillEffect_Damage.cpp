#include "DataAsset/SkillData/StaticSkillEffect/StaticSkillEffect_Damage.h"
#include "Component/AttributeComponent/AttributeSetComponentModel.h"
#include "Actor/BoardActor/BoardActorModel.h"
#include "TAS/Effect/Stat/TacticalEffect_Stat_Damage.h"
#include "TAS/Effect/TacticalEffectContext.h"

#include "Actor/BoardActor/BoardCombatTarget.h"

#include "AttributeSet/UnitAttributeSet.h"

void UStaticSkillEffect_Damage::ApplySkillEffect(
	float SkillPoint,
	TWeakObjectPtr<UBoardActorModel> SourceActor,
	TWeakObjectPtr<UBoardActorModel> TargetActor,
	FBoardCombatTargetSnapshotData* SourceSnapShot,
	FBoardCombatTargetSnapshotData* TargetSnapShot)
{
	// 유효한 타겟인지 검사합니다.
	checkf(SourceActor.IsValid(), TEXT("시전자 유효하지 않음"));
	checkf(TargetActor.IsValid(), TEXT("타겟 유효하지 않음"));

	IBoardCombatTarget* SourceActorTarget = Cast<IBoardCombatTarget>(SourceActor);
	if (SourceActorTarget == nullptr)
	{
		// 전투 가능한 대상이 아님
		return;
	}
	IBoardCombatTarget* TargetActorTarget = Cast<IBoardCombatTarget>(TargetActor);
	if (TargetActorTarget == nullptr)
	{
		// 전투 가능한 대상이 아님
		return;
	}

	// 시전자의 정보를 가져온다.
	TWeakObjectPtr<UAttributeSetComponentModel> SourceASC = SourceActorTarget->GetAttributeComponentModel();
	UE_LOG(LogTemp, Log, TEXT("SourceID : %d"), SourceASC->GetUniqueID());

	if (!SourceASC.IsValid())
		return;

	// 피격자의 정보를 가져온다.
	TWeakObjectPtr<UAttributeSetComponentModel> TargetASC = TargetActorTarget->GetAttributeComponentModel();
	UE_LOG(LogTemp, Log, TEXT("TargetID : %d"), TargetASC->GetUniqueID());

	if (!TargetASC.IsValid())
		return;

	// Effect를 적용한다.
	UTacticalEffectContext* EffectContext = TargetASC->MakeEffectContext();
	EffectContext->SetInstigator(SourceActor.Get());
	EffectContext->SetAttributeSetComponentModel(TargetASC.Get());

	TSharedPtr<FTacticalEffectSpec> EffectSpec = TargetASC->MakeOutgoingSpec(UTacticalEffect_Stat_Damage::StaticClass(), EffectContext);
	EffectSpec->mDynamicMagnitude = mEffectDefaultValue + mEffectRatioValue * SkillPoint;

	UE_LOG(LogTemp, Warning, TEXT("PreHP : %f"), TargetASC->GetAttributeCurrentValue(UUnitAttributeSet::GetHPAttribute()));

	SourceASC->ApplyTacticalEffectSpecToTarget(*EffectSpec, TargetASC.Get());

	UE_LOG(LogTemp, Warning, TEXT("CurHP : %f"), TargetASC->GetAttributeCurrentValue(UUnitAttributeSet::GetHPAttribute()));
}
