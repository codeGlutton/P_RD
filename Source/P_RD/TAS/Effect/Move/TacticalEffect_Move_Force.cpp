// Fill out your copyright notice in the Description page of Project Settings.


#include "TAS/Effect/Move/TacticalEffect_Move_Force.h"
#include "Actor/BoardActor/BoardActorModel.h"
#include "Component/AttributeComponent/AttributeSetComponentModel.h"
#include "Singleton/WorldSubsystem/SRPGCombatSubsystem.h"
#include "Singleton/WorldSubsystem/SRPGCombatModel.h"
#include "Actor/TileMap/TileMap.h"
#include "SRPGFramework/SRPGFrameworkType.h"

void UTacticalEffect_Move_Force::ActivateEffect(const UBoardActorModel& Caster, const FTileIndex& TargetTile, TArray<class UTacticalEffectContext*>& EffectContexts)
{
	Super::ActivateEffect(Caster, TargetTile, EffectContexts);
	// 캐스터의 ASC를 가져온다.
	TWeakObjectPtr<UAttributeSetComponentModel> AttributeSet = Caster.FindComponentModelByClass<UAttributeSetComponentModel>();

	// 타일에서 Unit을 뽑아온다.
	if (UWorld* World = GetWorld())
	{
		// 서브 시스템에 접근하여 타일맵 모델을 가져온다.
		if (USRPGCombatSubsystem* CombatSubsytem = World->GetSubsystem<USRPGCombatSubsystem>())
		{

			//UObjectModel* ObjectModel= CombatSubsytem->GetModel();
			//check(ObjectModel);

			//USRPGCombatModel* CombatModel = Cast<USRPGCombatModel>(ObjectModel);
			//check(CombatModel);

			// 타일맵 모델을 가져온다.
			//ATileMap* TileMap = CombatModel->GetTileMap();
			//checkf(CombatModel, TEXT("CombatModel이 없습니다. CombatModel이 생성되었는지 확인해주세요."));

			// 모델을 가져와야 하는 것 같은데?

			// 유닛을 가져온다.
			//TileMap->GetActorsOnTile(TargetTile, ETileLayerFlag::Unit);

			// 유닛의 체력을 깍는다.

		}
	}

	// 효과를 적용한다.
}
