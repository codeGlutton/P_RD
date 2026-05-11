/*****************************************************************//**
 * @file   RunSaveGame.h
 * @brief  런 데이터를 저장하는 객체 정의 헤더
 * @author 모호재
 * @date   2026-04-29
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "GameFramework/SaveGame.h"

#include "RunSaveGame.generated.h"

/**
 * @brief  런 데이터를 저장하는 객체
 */
UCLASS()
class P_RD_API URunSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY(Category = Run, VisibleAnywhere, meta = (DisplayName = "Data"))
	TArray<uint8> mData;
};

