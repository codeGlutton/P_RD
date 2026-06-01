/*****************************************************************//**
 * @file   TitleGameMode.h
 * @brief  타이틀 방에 대한 GameMode 정의 헤더
 * @author 모호재
 * @date   2026-05-19
 *********************************************************************/

#pragma once

#include "GameMode/RoomGameModeBase.h"
#include "Singleton/InstanceSubsystem/PersistentDataWriter.h"
#include "TitleGameMode.generated.h"

class UTitleMenuWidget;
struct FStage;

/**
 * @brief  타이틀 방에 대한 GameMode
 */
UCLASS()
class P_RD_API ATitleGameMode : public ARoomGameModeBase, public IRunDataWriter
{
	GENERATED_BODY()

public:
	ATitleGameMode();

protected:
	void InitializeCommonRoom() override;
	void BeginRoom() override;

private:
	UFUNCTION()
	void HandleTitleConfirmRequested();

	UFUNCTION()
	void HandleTitleContinueRequested();

	void HandleStageCreated(const FStage& NewStage);
	void RefreshTitleMapFromRun();
	bool StartPreviewRun();

private:
	UPROPERTY()
	TWeakObjectPtr<UTitleMenuWidget> mTitleMenuWidget;
};
