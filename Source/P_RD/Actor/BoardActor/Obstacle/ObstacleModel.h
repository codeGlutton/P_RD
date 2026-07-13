// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "RDMinimal.h"
#include "Actor/BoardActor/BoardActorModel.h"
#include "GenericTeamAgentInterface.h"

#include "ObstacleModel.generated.h"

/**
 * 
 */
UCLASS(abstract, Blueprintable)
class P_RD_API UObstacleModel : public UBoardActorModel
{
	GENERATED_BODY()

public:
	UObstacleModel();

	/* UBoardActorModel 상속 */
public:
	void PostInitializeComponentModels() override;
};
