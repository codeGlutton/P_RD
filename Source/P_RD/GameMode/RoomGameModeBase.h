/*****************************************************************//**
 * @file   RoomGameModeBase.h
 * @brief  방에 대한 베이스 GameMode 정의 헤더
 * @author 모호재
 * @date   2026-05-08
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "RoomGameModeBase.generated.h"

class AUnit;

/**
 * @brief  방에 대한 베이스 GameMode
 */
UCLASS(abstract)
class P_RD_API ARoomGameModeBase : public AGameModeBase
{
	GENERATED_BODY()
	
protected:
	void BeginPlay() override;

public:
	AUnit* GetPlayerUnit();

protected:
	UPROPERTY()
	TWeakObjectPtr<AUnit> mPlayerUnit;
};
