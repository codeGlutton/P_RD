#include "GameMode/RoomGameModeBase.h"
#include "Singleton/InstanceSubsystem/PersistentData.h"
#include "Singleton/InstanceSubsystem/SaveGameSubsystem.h"
#include "Singleton/InstanceSubsystem/GameProfileSubsystem.h"
#include "Singleton/InstanceSubsystem/RoomTransitionSubsystem.h"
#include "Singleton/InstanceSubsystem/PlayerUnitRestorationSubsystem.h"

void ARoomGameModeBase::BeginPlay()
{
	Super::BeginPlay();

	/* 1. 방 전환 즉시 저장 */

	// TODO : Save 중임을 표기하는 UI 생성
	SaveRunAsync(FAsyncSaveGameToSlotDelegate::CreateLambda([](const FString& SlotName, int32 UserIndex, bool IsSuccussed) {
		checkf(IsSuccussed == true, TEXT("방 전환 시점 저장 실패"));
		// TODO : Save 중임을 표기하는 UI 제거
		}));

	// 2. 플레이어 복원 및 바인딩 (스킬, 인벤토리, 주사위 등등)
	RestorePlayerUnit();

	// 3. 방 무관계 UI 생성
	
	// 4. 각 Room에 맞는 World Subsystem Init 타이밍
	// + 메인 유닛 등록 및 배치
	// + 방 타입 연관 유닛 생성
	// + 방 타입 연관 UI 생성
}

void ARoomGameModeBase::EndRun() const
{
	GetGameInstance()->GetSubsystem<UGameProfileSubsystem>()->EndRun();
}

void ARoomGameModeBase::PreloadTitleRoomAsync() const
{
	GetGameInstance()->GetSubsystem<URoomTransitionSubsystem>()->PreloadTitleRoomAsync();
}

void ARoomGameModeBase::PreloadRoomAsync(int32 RoomRowIndex, int32 RoomColumnIndex) const
{
	GetGameInstance()->GetSubsystem<URoomTransitionSubsystem>()->PreloadRoomAsync(RoomRowIndex, RoomColumnIndex);
}

void ARoomGameModeBase::PreloadRoomAsync(EStageLevelType StageLevel) const
{
	GetGameInstance()->GetSubsystem<URoomTransitionSubsystem>()->MakeStageAndPreloadRoomAsync(StageLevel);
}

void ARoomGameModeBase::TransitionLoadedRoomAsync() const
{
	GetGameInstance()->GetSubsystem<URoomTransitionSubsystem>()->TransitLoadedRoomAsync();
}

void ARoomGameModeBase::SaveRunAsync(FAsyncSaveGameToSlotDelegate Callback) const
{
	GetGameInstance()->GetSubsystem<USaveGameSubsystem>()->SaveRunAsync(MoveTemp(Callback));
}

void ARoomGameModeBase::RestorePlayerUnit()
{
	UPlayerUnitRestorationSubsystem* PlayerUnitRestorationSubsystem = GetGameInstance()->GetSubsystem<UPlayerUnitRestorationSubsystem>();
	APlayerUnit* PlayerUnit = PlayerUnitRestorationSubsystem->SpawnPlayerUnit(GetWorld());
	PlayerUnitRestorationSubsystem->RegisterPlayerUnit(PlayerUnit);
}

AUnit* ARoomGameModeBase::GetPlayerUnit() const
{
	return mPlayerUnit.Get();
}