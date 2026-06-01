#include "GameMode/RDGameModeBase.h"
#include "Singleton/InstanceSubsystem/PersistentDataSubsystem.h"

#include "Singleton/WorldSubsystem/WorldWidgetSubsystem.h"

ARDGameModeBase::ARDGameModeBase()
{
	mWorldWidgets = { EWorldWidgetType::MsgNotify, EWorldWidgetType::SaveNotify };
}

void ARDGameModeBase::BeginPlay()
{
	Super::BeginPlay();

	UWorldWidgetSubsystem* WorldWidgetSubsystem = GetWorld()->GetSubsystem<UWorldWidgetSubsystem>();

	/* 공용 방 로직 */

	InitializeCommonRoom();
	for (EWorldWidgetType WorldWidgetType : mWorldWidgets)
	{
		WorldWidgetSubsystem->InitWorldWidget(WorldWidgetType);
	}

	/* 전용 방 로직 */
	
	InitializeRoom();
	WorldWidgetSubsystem->InitHUD(mHUDClass);

	BeginRoom();
}

void ARDGameModeBase::InitializeCommonRoom()
{
}

void ARDGameModeBase::InitializeRoom()
{
}

void ARDGameModeBase::BeginRoom()
{
}

const UUserPersistData* ARDGameModeBase::GetUserPersistData() const
{
	UPersistentDataSubsystem* PersistentDataSubsystem = GetGameInstance()->GetSubsystem<UPersistentDataSubsystem>();
	checkf(PersistentDataSubsystem != nullptr, TEXT("영구 데이터 서브시스템 nullptr"));

	return PersistentDataSubsystem->GetUserPersistData();
}

const URunPersistData* ARDGameModeBase::GetRunPersistData() const
{
	UPersistentDataSubsystem* PersistentDataSubsystem = GetGameInstance()->GetSubsystem<UPersistentDataSubsystem>();
	checkf(PersistentDataSubsystem != nullptr, TEXT("영구 데이터 서브시스템 nullptr"));

	return PersistentDataSubsystem->GetRunPersistData();
}