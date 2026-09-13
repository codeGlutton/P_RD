#pragma once
#include "DataAsset/RoomSpawnData/StaticCombatRoomSpawnData.h"
#include "StaticTutorialRoomSpawnData.generated.h"

// An authored monster room, selected explicitly for onboarding, never by stage level.
UCLASS()
class P_RD_API UStaticTutorialRoomSpawnData : public UStaticMonsterRoomSpawnData
{
	GENERATED_BODY()
public:
	UStaticTutorialRoomSpawnData();
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tutorial")
	FTileIndex MoveDestination;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tutorial", meta = (ClampMin = "0"))
	int32 TargetEnemyIndex = 0;
	FTileIndex GetTargetTile() const;
	bool ValidateLayout(TArray<FText>& Errors) const;
#if WITH_EDITOR
	EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif
};
