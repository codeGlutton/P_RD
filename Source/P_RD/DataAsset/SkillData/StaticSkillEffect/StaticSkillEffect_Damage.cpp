#include "DataAsset/SkillData/StaticSkillEffect/StaticSkillEffect_Damage.h"
#include "TAS/Effect/Stat/TacticalEffectContext_Stat.h"
#include "Component/AttributeComponent/AttributeSetComponentModel.h"
#include "Actor/BoardActor/BoardActorModel.h"
#include "TAS/Effect/Stat/TacticalEffect_Stat_Damage.h"

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

	// 시전자의 정보를 가져온다.
	TWeakObjectPtr<UAttributeSetComponentModel> SourceASC = SourceActor->FindComponentModelByClass<UAttributeSetComponentModel>();
	UE_LOG(LogTemp, Warning, TEXT("SourceID : %d"), SourceASC->GetUniqueID());

	// 시전자의 ASC는 없어도 된다.

	// 피격자의 정보를 가져온다.
	TWeakObjectPtr<UAttributeSetComponentModel> TargetASC = TargetActor->FindComponentModelByClass<UAttributeSetComponentModel>();
	UE_LOG(LogTemp, Warning, TEXT("TargetID : %d"), TargetASC->GetUniqueID());

	// 피격자의 ASC는 반드시 필요하다.
	if (!TargetASC.IsValid())
		return;

	// Effect를 적용한다.
	UTacticalEffectContext* EffectContext = TargetASC->MakeEffectContext();
	EffectContext->SetInstigator(SourceActor.Get());
	EffectContext->SetAttributeSetComponentModel(TargetASC.Get());

	TSharedPtr<FTacticalEffectSpec> EffectSpec = TargetASC->MakeOutgoingSpec(UTacticalEffect_Stat_Damage::StaticClass(), EffectContext);
	EffectSpec->mDynamicMagnitude = mEffectDefaultValue + mEffectRatioValue * SkillPoint;

	UE_LOG(LogTemp, Warning, TEXT("PreHP : %f"), TargetASC->GetAttributeCurrentValue(UUnitAttributeSet::GetHPAttribute()));

	TargetASC->ApplyTacticalEffectSpecToSelf(*EffectSpec);

	UE_LOG(LogTemp, Warning, TEXT("CurHP : %f"), TargetASC->GetAttributeCurrentValue(UUnitAttributeSet::GetHPAttribute()));
}
