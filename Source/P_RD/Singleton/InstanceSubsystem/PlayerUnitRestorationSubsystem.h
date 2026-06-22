/*****************************************************************//**
 * @file   PlayerUnitRestorationSubsystem.h
 * @brief  플레이어 유닛 복구 Subsystem 구현 헤더
 * @author 모호재
 * @date   2026-05-21
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Singleton/InstanceSubsystem/PersistentDataWriter.h"

#include "PlayerUnitRestorationSubsystem.generated.h"

 // Player Unit Restoration 신규 로그 카테고리 등록
DECLARE_LOG_CATEGORY_EXTERN(LogPlayerUnitRestoration, Log, All)

class UPlayerUnitModel;

/**
 * @brief  플레이어 유닛 복구 Subsystem
 */
UCLASS()
class P_RD_API UPlayerUnitRestorationSubsystem : public UGameInstanceSubsystem, public IRunDataWriter
{
	GENERATED_BODY()

public:
	UPlayerUnitModel* SpawnPlayerUnit(UWorld* World) const;
	void RegisterPlayerUnit(UPlayerUnitModel* PlayerUnit) const;
};
