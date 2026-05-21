/*****************************************************************//**
 * @file   BinarySaveGame.h
 * @brief  바이트 데이터를 저장하는 객체 정의 헤더
 * @author 모호재
 * @date   2026-05-21
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "GameFramework/SaveGame.h"

#include "BinarySaveGame.generated.h"

/**
 * @brief  바이트 데이터를 저장하는 객체
 */
UCLASS()
class P_RD_API UBinarySaveGame : public USaveGame
{
	GENERATED_BODY()
	
public:
	UPROPERTY(Category = User, VisibleAnywhere, meta = (DisplayName = "Data"))
	TArray<uint8> mData;
};

