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
	/**
	 * @brief 저장된 파티 전원을 스폰한다.
	 *
	 * 한 명만 스폰하던 자리였다. 런을 여섯 중 셋으로 시작하도록 바뀌면서
	 * 저장본이 파티를 들고 있고, 방에 들어갈 때마다 그 전원을 다시 세운다.
	 * @param World 스폰할 월드
	 * @return 저장본에 적힌 차례대로의 유닛들. 하나라도 못 세우면 그 자리는 빠진다
	 */
	TArray<UPlayerUnitModel*> SpawnPartyUnits(UWorld* World) const;

	/** @brief 스폰한 유닛을 제 파티 칸에 잇는다. */
	void RegisterPlayerUnit(UPlayerUnitModel* PlayerUnit) const;

private:
	UPlayerUnitModel* SpawnOne(UWorld* World, const FPrimaryAssetId& PlayerUnitId) const;
};
