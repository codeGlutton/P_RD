/*****************************************************************//**
 * @file   UnitConsoleCommand.cpp
 * @brief  유닛 조회 테스트용 콘솔 명령
 * @details
 * RD.ListUnits  전투 중인 유닛 목록 (인덱스, 이름, 타일, 이동 중 여부). 플레이어 인덱스는 RD.Teleport 등에 사용
 * @author 이문환
 * @date   2026-09-16
 *********************************************************************/

#if !UE_BUILD_SHIPPING

#include "RDMinimal.h"
#include "HAL/IConsoleManager.h"
#include "Engine/World.h"

#include "Singleton/WorldSubsystem/SRPGCombatModel.h"
#include "Pawn/UnitModel.h"
#include "Component/BoardMovementComponent/BoardMovementComponentModel.h"

namespace
{
	// 유닛 목록 한 줄 출력
	void LogUnit(int32 Index, const UUnitModel* Unit)
	{
		if (Unit == nullptr)
		{
			UE_LOG(LogRD, Log, TEXT("  [%d] (null)"), Index);
			return;
		}
		const FTileIndex& Tile = Unit->GetTileTransform().mIndex;
		const UBoardMovementComponentModel* Movement = Unit->GetBoardMovementComponentModel();
		const bool Moving = (Movement != nullptr) && Movement->IsMoving();
		UE_LOG(LogRD, Log, TEXT("  [%d] %s  타일(%d,%d)%s"), Index, *Unit->GetBoardActorDisplayName().ToString(), Tile.mX, Tile.mY, Moving ? TEXT("  이동 중") : TEXT(""));
	}

	// 전투 중인 유닛 목록 출력
	void ListUnits(const TArray<FString>& Args, UWorld* World)
	{
		USRPGCombatModel* CombatModel = (World != nullptr) ? GetWorldSubsystemModel<USRPGCombatModel>(World) : nullptr;
		if (CombatModel == nullptr)
		{
			UE_LOG(LogRD, Warning, TEXT("전투 모델 없음: 전투 방에서만 사용 가능"));
			return;
		}

		const TArray<TObjectPtr<UUnitModel>>& PlayerUnits = CombatModel->GetPlayerUnits();
		UE_LOG(LogRD, Log, TEXT("플레이어 유닛 %d명"), PlayerUnits.Num());
		for (int32 Index = 0; Index < PlayerUnits.Num(); ++Index)
		{
			LogUnit(Index, PlayerUnits[Index]);
		}

		int32 OtherNum = 0;
		UE_LOG(LogRD, Log, TEXT("그 외 유닛"));
		for (const TObjectPtr<UUnitModel>& Unit : CombatModel->GetUnits())
		{
			if (PlayerUnits.Contains(Unit) == false)
			{
				LogUnit(OtherNum++, Unit);
			}
		}
	}

	FAutoConsoleCommandWithWorldAndArgs ListUnitsCommand(
		TEXT("RD.ListUnits"),
		TEXT("전투 중인 유닛 목록을 출력한다. 플레이어 인덱스는 RD.Teleport에 사용"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&ListUnits));
}

#endif
