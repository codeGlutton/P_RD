#include "UI/TopMenuBarWidget.h"

#include "Components/TextBlock.h"
#include "GameMode/RoomGameModeBase.h"
#include "Singleton/InstanceSubsystem/PersistentData.h"

/**
 * @brief WBP_TopMenuBar가 필수 버튼 바인딩을 제공하는지 로그로 확인한다.
 */
void UTopMenuBarWidget::ValidateDesignerBindings() const
{
	if (MapButton == nullptr)
	{
		UE_LOG(LogRD, Warning, TEXT("TopMenuBarWidget: MapButton is not connected."));
	}

	if (SettingsButton == nullptr)
	{
		UE_LOG(LogRD, Warning, TEXT("TopMenuBarWidget: SettingsButton is not connected."));
	}

	if (DiceButton == nullptr)
	{
		UE_LOG(LogRD, Verbose, TEXT("TopMenuBarWidget: DiceButton is not connected."));
	}

	if (SkillButton == nullptr)
	{
		UE_LOG(LogRD, Verbose, TEXT("TopMenuBarWidget: SkillButton is not connected."));
	}
}

/**
 * @brief 기본 라벨과 현재 방 요약 문구를 채운다.
 */
void UTopMenuBarWidget::SyncDefaultText() const
{
	if (MapButtonText != nullptr)
	{
		MapButtonText->SetText(NSLOCTEXT("TopMenuBarWidget", "MapButtonText", "MAP"));
	}

	if (SettingsButtonText != nullptr)
	{
		SettingsButtonText->SetText(NSLOCTEXT("TopMenuBarWidget", "SettingsButtonText", "SET"));
	}

	if (DiceButtonText != nullptr)
	{
		DiceButtonText->SetText(NSLOCTEXT("TopMenuBarWidget", "DiceButtonText", "DICE 0"));
	}

	if (SkillButtonText != nullptr)
	{
		SkillButtonText->SetText(NSLOCTEXT("TopMenuBarWidget", "SkillButtonText", "SKILL 0"));
	}

	if (TitleTextBlock != nullptr)
	{
		TitleTextBlock->SetText(NSLOCTEXT("TopMenuBarWidget", "TitleText", "ROOM"));
	}

	if (SummaryTextBlock != nullptr)
	{
		SummaryTextBlock->SetText(FText::GetEmpty());
	}

	RefreshRoomInfo();
}

/**
 * @brief RoomGameMode가 제공하는 현재 런 정보를 탑바 제목/요약으로 변환한다.
 */
void UTopMenuBarWidget::RefreshRoomInfo() const
{
	ARoomGameModeBase* GameMode = GetWorld() != nullptr ? GetWorld()->GetAuthGameMode<ARoomGameModeBase>() : nullptr;
	FRunControlView RunControlView;
	if (GameMode == nullptr || !GameMode->GetRunControlView(OUT RunControlView))
	{
		if (TitleTextBlock != nullptr)
		{
			TitleTextBlock->SetText(NSLOCTEXT("TopMenuBarWidget", "TitleTextFallback", "ROOM"));
		}
		if (SummaryTextBlock != nullptr)
		{
			SummaryTextBlock->SetText(FText::GetEmpty());
		}
		return;
	}

	if (TitleTextBlock != nullptr)
	{
		TitleTextBlock->SetText(FText::Format(
			NSLOCTEXT("TopMenuBarWidget", "RoomTitleFormat", "ROOM {0}-{1}"),
			FText::AsNumber(RunControlView.mRow + 1),
			FText::AsNumber(RunControlView.mColumn + 1)));
	}

	if (SummaryTextBlock != nullptr)
	{
		SummaryTextBlock->SetText(FText::Format(
			NSLOCTEXT("TopMenuBarWidget", "RoomSummaryFormat", "Lv {0} | Difficulty {1}"),
			FText::AsNumber(RunControlView.mPlayerLevel),
			FText::AsNumber(RunControlView.mDifficulty)));
	}

	const URunPersistData* RunPersistData = GameMode->GetRunPersistData();
	if (DiceButtonText != nullptr)
	{
		const int32 DiceCount = RunPersistData != nullptr ? RunPersistData->GetDiceIds().Num() : 0;
		DiceButtonText->SetText(FText::Format(
			NSLOCTEXT("TopMenuBarWidget", "DiceButtonCountFormat", "DICE {0}"),
			FText::AsNumber(DiceCount)
		));
	}

	if (SkillButtonText != nullptr)
	{
		SkillButtonText->SetText(NSLOCTEXT("TopMenuBarWidget", "SkillButtonText", "SKILL 6"));
	}
}
