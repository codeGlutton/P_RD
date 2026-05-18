#include "GameMode/RoomGameModeBase.h"
#include "Singleton/InstanceSubsystem/PersistentDataSubsystem.h"

void ARoomGameModeBase::BeginPlay()
{
	Super::BeginPlay();

	UPersistentDataSubsystem* PersistentDataSubsystem = GetGameInstance()->GetSubsystem<UPersistentDataSubsystem>();

	PersistentDataSubsystem->GetRunPersistData()->MakeNewRun(1);
	PersistentDataSubsystem->GetRunPersistData()->MakeStageAsync(EStageLevelType::Stage1, FOnCreateStage());
}

AUnit* ARoomGameModeBase::GetPlayerUnit()
{
	return mPlayerUnit.Get();
}