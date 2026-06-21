// Fill out your copyright notice in the Description page of Project Settings.

#include "TAS/Effect/Stat/TacticalEffect_Stat_Damage.h"
#include "TAS/Effect/Stat/TacticalEffectContext_Stat.h"
#include "Actor/BoardActor/BoardActorModel.h"
#include "Component/AttributeComponent/AttributeSetComponentModel.h"
#include "Actor/TileMap/TileMapModel.h"
#include "SRPGFramework/SRPGFrameworkType.h"

void UTacticalEffect_Stat_Damage::ActivateEffect(const UBoardActorModel& Caster, const FTileIndex& TargetTile, const class UTacticalEffectContext* EffectContexts)
{
	Super::ActivateEffect(Caster, TargetTile, EffectContexts);

	// 캐스터의 ASC를 가져온다.
	TWeakObjectPtr<UAttributeSetComponentModel> AttributeSet = Caster.FindComponentModelByClass<UAttributeSetComponentModel>();
	checkf(AttributeSet.IsValid(), TEXT("시전자의 ASC가 없습니다."));

	// 컨텍스트를 가져온다.
	TWeakObjectPtr<const UTacticalEffectContext_Stat> EffectContext_Stat = Cast<UTacticalEffectContext_Stat>(EffectContexts);
	checkf(EffectContext_Stat.IsValid(), TEXT("컨텍스트가 이상합니다."));

	// 서브 시스템에 접근하여 타일맵 모델을 가져온다.
	TWeakObjectPtr<UTileMapModel> TMModel;
	checkf(TMModel.IsValid(), TEXT("타일맵 모델이 존재하지 않습니다.."));
	// ===============================================================

	// 해당 타일에 유닛 모델을 가져온다.
	TWeakObjectPtr<UBoardActorModel> TargetBoardModel = TMModel->GetActorOnTile<UBoardActorModel>(TargetTile, ETileLayerFlag::Unit);
	
	// 유닛이 없다면 반환한다.
	if (!TargetBoardModel.IsValid())
		return;

	// 타겟의 ASC를 가져온다.
	TWeakObjectPtr<UAttributeSetComponentModel> TargetASC = TargetBoardModel->FindComponentModelByClass<UAttributeSetComponentModel>();
	checkf(TargetASC.IsValid(), TEXT("타겟의 ASC가 존재하지 않습니다.."));

	// 타겟의 체력을 깍는다.
	float HP = TargetASC->GetAttributeValue(UUnitAttributeSet::GetHPAttribute());
	TargetASC->SetAttributeValue(UUnitAttributeSet::GetHPAttribute(), HP - EffectContext_Stat.Get()->GetFinal());

}
