// Fill out your copyright notice in the Description page of Project Settings.

#include "Setting/GameTeamType.h"
#include "Singleton/WorldSubsystem/TacticalFrameworkModel.h"
#include "Singleton/WorldSubsystem/TacticalFrameworkSubsystem.h"

#include "Actor/BoardActor/Obstacle/ObstacleModel.h"

UObstacleModel::UObstacleModel()
{
	mTileLayerFlags = StaticCast<int32>(ETileLayerFlag::Obstacle);
	mBlockLayerFlags = StaticCast<int32>(ETileLayerFlag::Unit | ETileLayerFlag::Obstacle);
	mReplaceLayerFlags = StaticCast<int32>(ETileLayerFlag::None);
	mOverlayLayerPriority = 0;
}

void UObstacleModel::PostInitializeComponentModels()
{
	Super::PostInitializeComponentModels();
}
