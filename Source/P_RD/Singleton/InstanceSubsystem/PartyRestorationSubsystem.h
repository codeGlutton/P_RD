/*****************************************************************//**
 * @file   PartyRestorationSubsystem.h
 * @brief  파티 복구 Subsystem 구현 헤더
 * @author 모호재
 * @date   2026-05-21
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Singleton/InstanceSubsystem/PersistentDataWriter.h"

#include "PartyRestorationSubsystem.generated.h"

 // Party Restoration 신규 로그 카테고리 등록
DECLARE_LOG_CATEGORY_EXTERN(LogPartyRestoration, Log, All)

class UPartyModel;

/**
 * @brief  파티 유닛 복구 Subsystem
 */
UCLASS()
class P_RD_API UPartyRestorationSubsystem : public UGameInstanceSubsystem, public IRunDataWriter
{
	GENERATED_BODY()

public:
	UPartyModel* RestorePartyFromPersistData(UWorld* World) const;
};
