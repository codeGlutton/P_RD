#pragma once

#include "RDMinimal.h"
#include "LevelUpSkillRewardFlow.generated.h"

class URunPersistData;
class UPartyModel;
class UPlayerUnitModel;
class UShopUIModel;
class UShopUIWidgetBase;
class APlayerController;
class UStaticUnitSkillData;
struct FLevelUpSkillReward;
struct FRarityRate;

namespace LevelUpSkillReward
{
	P_RD_API TArray<FPrimaryAssetId> ChooseCandidates(const TArray<UStaticUnitSkillData*>& Pool,
		const FRarityRate& Rate, const FRandomStream& Stream);
	P_RD_API bool TryEquip(FLevelUpSkillReward& Reward, UPlayerUnitModel* Unit,
		const FPrimaryAssetId& SkillId, int32 SkillSlot);
}

/** Gameplay adapter for the existing shop selection/equipment screen. */
UCLASS()
class P_RD_API ULevelUpSkillRewardFlow : public UObject
{
	GENERATED_BODY()
public:
	ULevelUpSkillRewardFlow();
	void Open(URunPersistData* Run, UPartyModel* Party, APlayerController* Controller);
	void Close();
private:
	void ShowNext();
	void Save();
	UFUNCTION() void Select(int32 Choice, int32 UnitIndex, int32 SkillSlot);
	UFUNCTION() void ContinueWithoutCandidate();
	UFUNCTION() void ShowDetail(int32 Choice);
	UFUNCTION() void ShowOwnedDetail(int32 UnitIndex, int32 SkillSlot);
	UPROPERTY() TObjectPtr<URunPersistData> mRun;
	UPROPERTY() TObjectPtr<UPartyModel> mParty;
	UPROPERTY() TObjectPtr<APlayerController> mController;
	UPROPERTY() TObjectPtr<UShopUIModel> mUIModel;
	UPROPERTY() TObjectPtr<UShopUIWidgetBase> mWidget;
	UPROPERTY() TSubclassOf<UShopUIWidgetBase> mWidgetClass;
	int32 mActiveReward = INDEX_NONE;
};
