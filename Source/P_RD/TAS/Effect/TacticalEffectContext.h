// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "RDMinimal.h"
#include "UObject/Object.h"
#include "Actor/TileMap/TileLayer.h"
#include "TacticalEffectContext.generated.h"

/**
 * 
 */
UCLASS()
class P_RD_API UTacticalEffectContext : public UObject
{
	GENERATED_BODY()
public:
	FGameplayTag mGameplayTag;
	ETileLayerFlag mTileLayerFlag;
	TSubclassOf<class UTacticalEffect> mTacticalEffect;
	TObjectPtr<class UBoardActorModel> mBoardActorSnapShot;
};
