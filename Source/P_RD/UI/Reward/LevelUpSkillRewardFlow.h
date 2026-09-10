#pragma once

#include "RDMinimal.h"
#include "Components/SlateWrapperTypes.h"
#include "LevelUpSkillRewardFlow.generated.h"

class URunPersistData;
class UPartyModel;
class UPlayerUnitModel;
class UShopUIModel;
class UShopUIWidgetBase;
class APlayerController;
class UStaticUnitSkillData;
class UUserWidget;
struct FLevelUpSkillReward;
struct FRarityRate;

namespace LevelUpSkillReward
{
	P_RD_API bool IsEligibleForUnit(UPlayerUnitModel* Unit, UStaticUnitSkillData* Skill);
	// Preserve valid saved choices; replace only candidates that are no longer eligible.
	P_RD_API bool RefreshOffer(FLevelUpSkillReward& Reward, UPlayerUnitModel* Unit,
		const TArray<UStaticUnitSkillData*>& Pool, const FRarityRate& Rate, const FRandomStream& Stream);
	P_RD_API TArray<FPrimaryAssetId> ChooseCandidates(const TArray<UStaticUnitSkillData*>& Pool,
		const FRarityRate& Rate, const FRandomStream& Stream, int32 MaxChoices = 3);
	P_RD_API int32 FindPendingReward(const TArray<FLevelUpSkillReward>& Rewards, int32 UnitIndex = INDEX_NONE);
	P_RD_API bool Skip(FLevelUpSkillReward& Reward);
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
	void ShowNext(int32 PreferredUnit = INDEX_NONE);
	void FinishActiveReward();
	UFUNCTION() void SelectRewardUnit(int32 UnitIndex);
	void Save();
	void SuspendRewardScreens();
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
	UPROPERTY() TMap<TObjectPtr<UUserWidget>, ESlateVisibility> mSuspendedRewardWidgets;
	int32 mActiveReward = INDEX_NONE;
};
