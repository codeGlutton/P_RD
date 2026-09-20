/*****************************************************************//**
 * @file   BoardMovementConsoleCommand.cpp
 * @brief  보드 이동 테스트용 콘솔 명령
 * @details
 * RD.Teleport <유닛인덱스> <X> <Y>  플레이어 유닛을 타일 (X,Y)로 텔레포트 (연출 포함). 인덱스는 RD.ListUnits 기준
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
	// 플레이어 유닛 하나를 지정 타일로 텔레포트
	void TeleportPlayerUnit(const TArray<FString>& Args, UWorld* World)
	{
		if (Args.Num() < 3 || Args[0].IsNumeric() == false || Args[1].IsNumeric() == false || Args[2].IsNumeric() == false)
		{
			UE_LOG(LogRD, Warning, TEXT("사용법: RD.Teleport <유닛인덱스> <X> <Y>"));
			return;
		}

		USRPGCombatModel* CombatModel = (World != nullptr) ? GetWorldSubsystemModel<USRPGCombatModel>(World) : nullptr;
		if (CombatModel == nullptr)
		{
			UE_LOG(LogRD, Warning, TEXT("전투 모델 없음: 전투 방에서만 사용 가능"));
			return;
		}

		const int32 PlayerIndex = FCString::Atoi(*Args[0]);
		const TArray<TObjectPtr<UUnitModel>>& PlayerUnits = CombatModel->GetPlayerUnits();
		if (PlayerUnits.IsValidIndex(PlayerIndex) == false || PlayerUnits[PlayerIndex] == nullptr)
		{
			UE_LOG(LogRD, Warning, TEXT("유닛인덱스 범위 밖: %d (플레이어 %d명, RD.ListUnits로 확인)"), PlayerIndex, PlayerUnits.Num());
			return;
		}

		UUnitModel* Unit = PlayerUnits[PlayerIndex];
		UBoardMovementComponentModel* Movement = Unit->GetBoardMovementComponentModel();
		if (Movement == nullptr)
		{
			UE_LOG(LogRD, Warning, TEXT("이동 컴포넌트 모델 없음"));
			return;
		}

		// 바라보는 방향은 유지
		const FTileIndex TargetIndex(FCString::Atoi(*Args[1]), FCString::Atoi(*Args[2]));
		const bool Started = Movement->TeleportTo(FTileTransform(TargetIndex, Unit->GetTileTransform().mDirection));
		UE_LOG(LogRD, Log, TEXT("텔레포트 %s: [%d] %s -> (%d,%d)"),
			Started ? TEXT("시작") : TEXT("거부 (이동 중이거나 막힌 타일)"),
			PlayerIndex, *Unit->GetBoardActorDisplayName().ToString(), TargetIndex.mX, TargetIndex.mY);
	}

	FAutoConsoleCommandWithWorldAndArgs TeleportCommand(
		TEXT("RD.Teleport"),
		TEXT("플레이어 유닛을 타일 (X,Y)로 텔레포트한다. RD.Teleport <유닛인덱스> <X> <Y>"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&TeleportPlayerUnit));
}

#endif
