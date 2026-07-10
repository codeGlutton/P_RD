#pragma once

#include "RDMinimal.h"
#include "UI/RDUserWidget.h"
#include "UI/Reward/RewardUITypes.h"

#include "CombatResultOverlayWidget.generated.h"

class SBorder;
class STextBlock;
class SWidget;

UENUM()
enum class ECombatResultOverlayMode : uint8
{
	None,
	VictoryReward,
	DefeatContinue
};

/** @brief 전투 결과 영상 위에 띄우는 최소 C++ 오버레이. */
UCLASS()
class P_RD_API UCombatResultOverlayWidget : public URDUserWidget
{
	GENERATED_BODY()

public:
	UCombatResultOverlayWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	void ShowVictoryReward(const FRewardUI& Reward, FSimpleDelegate ConfirmCallback);
	void ShowDefeatContinue(FSimpleDelegate ContinueCallback);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	FReply HandleConfirmClicked();
	void RefreshSlate();

private:
	ECombatResultOverlayMode mMode = ECombatResultOverlayMode::None;
	FRewardUI mReward;
	FSimpleDelegate mConfirmCallback;

	TSharedPtr<SBorder> mRewardPanel;
	TSharedPtr<STextBlock> mTitleText;
	TSharedPtr<STextBlock> mGoldText;
	TSharedPtr<STextBlock> mExpText;
	TSharedPtr<SBorder> mContinuePanel;
};
