#include "TAS/Effect/Stat/TacticalEffect_Stat_Damage.h"
#include "TAS/Effect/Stat/TacticalEffectContext_Stat.h"
#include "Actor/BoardActor/BoardActorModel.h"
#include "Component/AttributeComponent/AttributeSetComponentModel.h"

#include "AttributeSet/UnitAttributeSet.h"

//void UTacticalEffect_Stat_Damage::ActivateEffect(const UBoardActorModel& Caster, const FTileIndex& TargetTile, const class UTacticalEffectContext* EffectContexts)
//{
//	Super::ActivateEffect(Caster, TargetTile, EffectContexts);
//
//	// 타겟 액터를 가져옵니다.
//	TWeakObjectPtr<UBoardActorModel> TargetActorModel = ExtractTarget(TargetTile, EffectContexts);
//	if (!TargetActorModel.IsValid())
//		return;
//
//	// 타겟의 ASC를 가져온다.
//	TWeakObjectPtr<UAttributeSetComponentModel> TargetASC = TargetActorModel->FindComponentModelByClass<UAttributeSetComponentModel>();
//	//checkf(TargetASC.IsValid(), TEXT("타겟의 ASC가 존재하지 않습니다.."));
//	if (!TargetASC.IsValid())
//		return;
//
//	// 컨텍스트를 가져온다.
//	TWeakObjectPtr<const UTacticalEffectContext_Stat> EffectContext_Stat = Cast<UTacticalEffectContext_Stat>(EffectContexts);
//	checkf(EffectContext_Stat.IsValid(), TEXT("컨텍스트가 이상합니다."));
//
//	// 타겟의 체력을 깍는다.
//	float HP = TargetASC->GetAttributeCurrentValue(UUnitAttributeSet::GetHPAttribute());
//	//TargetASC->SetAttributeValue(UUnitAttributeSet::GetHPAttribute(), HP - EffectContext_Stat.Get()->GetFinal());
//
//	UE_LOG(LogTemp, Warning, TEXT("Damage : %f"), HP);
//}
