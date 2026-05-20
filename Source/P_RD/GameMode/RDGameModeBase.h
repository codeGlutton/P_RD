/*****************************************************************//**
 * @file   RDGameModeBase.h
 * @brief  RD 프로젝트 게임 모드 베이스 정의 헤더
 * @author 모호재
 * @date   2026-05-19
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "RDGameModeBase.generated.h"

class UUserPersistData;
class URunPersistData;

/**
 * @brief  RD 프로젝트 게임 모드 베이스
 */
UCLASS(abstract)
class P_RD_API ARDGameModeBase : public AGameModeBase
{
	GENERATED_BODY()
	
public:
	const UUserPersistData* GetUserPersistData() const;
	const URunPersistData* GetRunPersistData() const;

public:
	UPROPERTY(Category = "UI", EditDefaultsOnly, BlueprintReadOnly, meta = (DisplayName = "MainUI"))
	TSubclassOf<UUserWidget> mMainUI;
};
