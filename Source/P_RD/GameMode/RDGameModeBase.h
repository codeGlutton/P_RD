/*****************************************************************//**
 * @file   RDGameModeBase.h
 * @brief  RD 프로젝트 게임 모드 베이스 정의 헤더
 * @author 모호재
 * @date   2026-05-19
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Blueprint/UserWidget.h"
#include "Singleton/WorldSubsystem/WorldWidgetType.h"
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
	ARDGameModeBase();

	/* AGameModeBase 상속 */
protected:
	void BeginPlay() override;

protected:
	virtual void InitializeCommonRoom();
	virtual void InitializeRoom();
	virtual void BeginRoom();

public:
	const UUserPersistData* GetUserPersistData() const;
	const URunPersistData* GetRunPersistData() const;

protected:
	UPROPERTY(Category = "UI", EditDefaultsOnly, BlueprintReadOnly, meta = (DisplayName = "HUDClass"))
	TSubclassOf<UUserWidget> mHUDClass;
	UPROPERTY(Category = "UI", EditDefaultsOnly, BlueprintReadOnly, meta = (DisplayName = "WorldWidgets"))
	TSet<EWorldWidgetType> mWorldWidgets;
};
