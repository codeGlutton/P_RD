#include "FunctionLibrary/CameraFunctionLibrary.h"
#include "Pawn/Camera/CombatCameraPawn.h"
#include "Singleton/InstanceSubsystem/PersistentDataSubsystem.h"
#include "Singleton/InstanceSubsystem/PersistentData.h"

bool UCameraFunctionLibrary::IsCameraShakePossible(const UObject* WorldContextObject)
{
	if (WorldContextObject == nullptr)
	{
		return false;
	}

	const UWorld* World = WorldContextObject->GetWorld();
	if (World == nullptr)
	{
		return false;
	}

	const UGameInstance* GameInstance = World->GetGameInstance();
	if (GameInstance == nullptr)
	{
		return false;
	}

	const UPersistentDataSubsystem* PersistentSubsystem = GameInstance->GetSubsystem<UPersistentDataSubsystem>();
	if (PersistentSubsystem == nullptr)
	{
		return false;
	}

	const UOptionPersistData* OptionData = PersistentSubsystem->GetOptionPersistData();
	if (OptionData == nullptr)
	{
		return false;
	}

	return OptionData->IsCameraShakeEnabled();
}

ACombatCameraPawn* UCameraFunctionLibrary::GetMainCameraPawn(const UObject* WorldContextObject)
{
	if (WorldContextObject == nullptr)
	{
		return nullptr;
	}

	const UWorld* World = WorldContextObject->GetWorld();
	if (World == nullptr)
	{
		return nullptr;
	}

	const APlayerController* PlayerController = World->GetFirstPlayerController();
	if (PlayerController != nullptr)
	{
		ACombatCameraPawn* CameraPawn = PlayerController->GetPawn<ACombatCameraPawn>();
		if (CameraPawn != nullptr)
		{
			return CameraPawn;
		}
	}

	return nullptr;
}
