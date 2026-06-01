#include "GameMode/RoomGameModeBase.h"
#include "Singleton/InstanceSubsystem/PersistentData.h"
#include "Singleton/InstanceSubsystem/SaveGameSubsystem.h"
#include "Singleton/InstanceSubsystem/GameProfileSubsystem.h"
#include "Singleton/InstanceSubsystem/RoomTransitionSubsystem.h"
#include "Singleton/InstanceSubsystem/PlayerUnitRestorationSubsystem.h"

#include "Singleton/WorldSubsystem/WorldWidgetSubsystem.h"
#include "Blueprint/UserWidget.h"

ARoomGameModeBase::ARoomGameModeBase()
{
	mWorldWidgets = { EWorldWidgetType::MsgNotify, EWorldWidgetType::SaveNotify, EWorldWidgetType::TopMenuBar };
}

void ARoomGameModeBase::InitializeCommonRoom()
{
	Super::InitializeCommonRoom();

	// 플레이어 복원
	RestorePlayerUnit();
}

void ARoomGameModeBase::BeginRoom()
{
	Super::BeginRoom();

	// 방 전환 즉시 저장
	SaveRunWithUIAsync();
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

void ARoomGameModeBase::SaveRunWithUIAsync() const
{
	UWorldWidgetSubsystem* WorldWidgetSubsystem = GetWorld()->GetSubsystem<UWorldWidgetSubsystem>();
	UUserWidget* SaveNotifyWidget = WorldWidgetSubsystem->GetWorldWidget(EWorldWidgetType::SaveNotify);
	//SaveNotifyWidget->PlayAnimation();
	// 시작 애니메이션
	GetGameInstance()->GetSubsystem<USaveGameSubsystem>()->SaveRunAsync(FAsyncSaveGameToSlotDelegate::CreateLambda([SaveNotifyWidget](const FString& SlotName, int32 UserIndex, bool IsSuccussed) {
		checkf(IsSuccussed == true, TEXT("방 전환 시점 저장 실패"));
		//SaveNotifyWidget->PlayAnimation();
		// 종료 애니메이션
		}));
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