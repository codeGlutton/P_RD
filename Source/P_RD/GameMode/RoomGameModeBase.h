/*****************************************************************//**
 * @file   RoomGameModeBase.h
 * @brief  방에 대한 베이스 GameMode 정의 헤더
 * @author 모호재
 * @date   2026-05-08
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "GameMode/RDGameModeBase.h"
#include "DataAsset/StageSpawnData/StageLevelType.h"
#include "RoomGameModeBase.generated.h"

class AUnit;

/**
 * @brief  방에 대한 베이스 GameMode
 */
UCLASS(abstract)
class P_RD_API ARoomGameModeBase : public ARDGameModeBase
{
	GENERATED_BODY()
	
protected:
	void BeginPlay() override;

public:
	void EndRun() const;

public:
	void PreloadTitleRoomAsync() const;
	void PreloadRoomAsync(int32 RoomRowIndex, int32 RoomColumnIndex) const;
	void PreloadRoomAsync(EStageLevelType StageLevel) const;
	void TransitionLoadedRoomAsync() const;

protected:
	void SaveRunAsync(FAsyncSaveGameToSlotDelegate Callback) const;

protected:
	void RestorePlayerUnit();

public:
	AUnit* GetPlayerUnit() const;

protected:
	UPROPERTY()
	TWeakObjectPtr<AUnit> mPlayerUnit;
};
