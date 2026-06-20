// Fill out your copyright notice in the Description page of Project Settings.

#include "TAS/Effect/Stat/TacticalEffect_Stat_Damage.h"
#include "TAS/Effect/TacticalEffectContext_Stat.h"
#include "Actor/BoardActor/BoardActorModel.h"
#include "Component/AttributeComponent/AttributeSetComponentModel.h"
#include "Singleton/WorldSubsystem/SRPGCombatSubsystem.h"
#include "SRPGFramework/SRPGFrameworkType.h"

void UTacticalEffect_Stat_Damage::ActivateEffect(const UBoardActorModel& Caster, const FTileIndex& TargetTile, TArray<class UTacticalEffectContext*>& EffectContexts)
{
	// 캐스터의 ASC를 가져온다.
	TWeakObjectPtr<UAttributeSetComponentModel> AttributeSet = Caster.FindComponentModelByClass<UAttributeSetComponentModel>();

	// 타일에서 Unit을 뽑아온다.
	if (UWorld* World = GetWorld())
	{
		if (USRPGCombatSubsystem* CombatSubsytem = World->GetSubsystem<USRPGCombatSubsystem>())
		{
			//UObjectModel* CombatObjectModel= CombatSubsytem->GetModel_Internal();
		}
	}

	// 효과를 적용한다.
}
