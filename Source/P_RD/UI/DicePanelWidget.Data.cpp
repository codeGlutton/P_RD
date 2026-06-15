#include "UI/DicePanelWidget.h"

#include "GameMode/RDGameModeBase.h"
#include "Singleton/InstanceSubsystem/PersistentData.h"
#include "Singleton/WorldSubsystem/WorldWidgetSubsystem.h"
#include "UI/CombatTileMapHUDWidget.h"

void UDicePanelWidget::RefreshDicePanelViews()
{
	mDicePanelViews.Reset();

	const UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	if (const UWorldWidgetSubsystem* WorldWidgetSubsystem = World->GetSubsystem<UWorldWidgetSubsystem>())
	{
		if (const UCombatTileMapHUDWidget* CombatHUD = WorldWidgetSubsystem->GetHUD<UCombatTileMapHUDWidget>())
		{
			const int32 CombatDiceCount = CombatHUD->GetCombatDiceViewCount();
			for (int32 DiceIndex = 0; DiceIndex < CombatDiceCount && DiceIndex < DicePanelSlotCount; ++DiceIndex)
			{
				FDiceViewData DiceView;
				if (CombatHUD->GetCombatDiceView(DiceIndex, DiceView.mDiceId, DiceView.mRarityType, DiceView.mResultValue, DiceView.mIsRolled))
				{
					mDicePanelViews.Add(DiceView);
				}
			}

			if (mDicePanelViews.IsEmpty() == false)
			{
				return;
			}
		}
	}

	const ARDGameModeBase* GameMode = World->GetAuthGameMode<ARDGameModeBase>();
	const URunPersistData* RunPersistData = GameMode != nullptr ? GameMode->GetRunPersistData() : nullptr;
	if (RunPersistData == nullptr || RunPersistData->IsActive() == false)
	{
		return;
	}

	for (const FPrimaryAssetId& DiceId : RunPersistData->GetDiceIds())
	{
		if (DiceId.IsValid() == false || mDicePanelViews.Num() >= DicePanelSlotCount)
		{
			continue;
		}

		FDiceViewData DiceView;
		DiceView.mDiceId = DiceId;
		DiceView.mRarityType = RDUIDice::ResolveDiceRarity(DiceId);
		DiceView.mIsRolled = false;
		mDicePanelViews.Add(DiceView);
	}
}
