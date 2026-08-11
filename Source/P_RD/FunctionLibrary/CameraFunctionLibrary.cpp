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

FIntVector2 UCameraFunctionLibrary::GetMainViewportSize(const UObject* WorldContextObject)
{
	FIntVector2 ViewportSize = FIntVector2::ZeroValue;

	const APlayerController* PlayerController = GetMainController(WorldContextObject);
	if (PlayerController == nullptr)
	{
		return ViewportSize;
	}

	PlayerController->GetViewportSize(OUT ViewportSize.X, OUT ViewportSize.Y);
	return ViewportSize;
}

FVector2D UCameraFunctionLibrary::GetSizeOnMainViewport(const UObject* WorldContextObject, const FVector2D& SizeRatio)
{
	FIntVector2 ViewportSize = GetMainViewportSize(WorldContextObject);
	return FVector2D(ViewportSize.X * SizeRatio.X, ViewportSize.Y * SizeRatio.Y);
}

ACombatCameraPawn* UCameraFunctionLibrary::GetMainCameraPawn(const UObject* WorldContextObject)
{
	const APlayerController* PlayerController = GetMainController(WorldContextObject);
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

APlayerController* UCameraFunctionLibrary::GetMainController(const UObject* WorldContextObject)
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

	return World->GetFirstPlayerController();
}
