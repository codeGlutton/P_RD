// Fill out your copyright notice in the Description page of Project Settings.


#include "Actor/BoardActor/Obstacle/ObstacleModel.h"

UObstacleModel::UObstacleModel()
{
}

void UObstacleModel::PostInitializeComponentModels()
{
	Super::PostInitializeComponentModels();
}

void UObstacleModel::SetGenericTeamId(const FGenericTeamId& TeamID)
{
	mTeamId = TeamID;
}

FGenericTeamId UObstacleModel::GetGenericTeamId() const
{
	return mTeamId;
}
