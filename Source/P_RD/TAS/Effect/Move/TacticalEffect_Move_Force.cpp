// Fill out your copyright notice in the Description page of Project Settings.


#include "TAS/Effect/Move/TacticalEffect_Move_Force.h"
#include "TAS/Effect/Move/TacticalEffectContext_Move_Force.h"
#include "Actor/BoardActor/BoardActorModel.h"
#include "Component/AttributeComponent/AttributeSetComponentModel.h"
#include "Actor/TileMap/TileMap.h"
#include "SRPGFramework/SRPGFrameworkType.h"

//void UTacticalEffect_Move_Force::ActivateEffect(const UBoardActorModel& Caster, const FTileIndex& TargetTile, const class UTacticalEffectContext* EffectContext)
//{
//	Super::ActivateEffect(Caster, TargetTile, EffectContext);
//
//	// 캐스터의 ASC를 가져온다.
//	TWeakObjectPtr<UAttributeSetComponentModel> AttributeSet = Caster.FindComponentModelByClass<UAttributeSetComponentModel>();
//	checkf(AttributeSet.IsValid(), TEXT("시전자의 ASC가 없습니다."));
//
//	// 컨텍스트를 가져온다.
//	TWeakObjectPtr<const UTacticalEffectContext_Move_Force> EffectContext_Move_Force = Cast<UTacticalEffectContext_Move_Force>(EffectContext);
//	checkf(EffectContext_Move_Force.IsValid(), TEXT("컨텍스트가 이상합니다."));
//
//	// 서브 시스템에 접근하여 타일맵 모델을 가져온다.
//	TWeakObjectPtr<UTileMapModel> TMModel;
//	checkf(TMModel.IsValid(), TEXT("타일맵 모델이 존재하지 않습니다.."));
//	// ===============================================================
//
//	// 해당 타일에 유닛 모델을 가져온다.
//	TWeakObjectPtr<UBoardActorModel> TargetBoardModel = TMModel->GetActorOnTile<UBoardActorModel>(TargetTile, ETileLayerFlag::Unit);
//
//	// 유닛이 없다면 반환한다.
//	if (!TargetBoardModel.IsValid())
//		return;
//
//	// Caster(UBoardActorModel)의 타일 인덱스 반환 기능 필요 ===========
//	FTileIndex CasterTileIndex;
//	// ===============================================================
//
//	// Caster(UBoardActorModel)와 TileIndex의 벡터 방향을 구하는 기능 필요
//	FTileIndex Direction;// = TargetTile - CasterTileIndex;
//	// ===============================================================
//
//	// TileMapModel에서 해당 방향으로 Range만큼 이동한다면 나오는 타일 구하기 기능 필요
//	FTileIndex Destination;
//	// ===============================================================
//
//	// TileMapModel에서 Actor를 Destination으로 이동시키는 기능 필요
//	
//	// ===============================================================
//
//}
