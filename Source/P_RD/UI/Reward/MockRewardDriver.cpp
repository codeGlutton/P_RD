#include "UI/Reward/MockRewardDriver.h"

#include "UI/Reward/RewardUIModel.h"

#define LOCTEXT_NAMESPACE "MockRewardDriver"

void UMockRewardDriver::BindAutoConfirm(URewardUIModel* UIModel)
{
	if (UIModel == nullptr)
	{
		return;
	}

	if (mUIModel != UIModel)
	{
		UnbindUIModel();
	}
	mUIModel = UIModel;
	mUIModel->OnRewardClaimRequested.AddUniqueDynamic(
		this, &UMockRewardDriver::HandleClaimRequested);
	mUIModel->OnRewardSelectionRequested.AddUniqueDynamic(
		this, &UMockRewardDriver::HandleSelectionRequested);
	mUIModel->OnRewardGrantBundleRequested.AddUniqueDynamic(
		this, &UMockRewardDriver::HandleGrantBundleRequested);
	mUIModel->OnRewardClaimed.AddUniqueDynamic(
		this, &UMockRewardDriver::HandleClaimed);
}

void UMockRewardDriver::SetOnPreviewClosed(FSimpleDelegate InCallback)
{
	mOnPreviewClosed = MoveTemp(InCallback);
}

/** @brief 개발용 고정 보상 스냅샷을 UIModel에 주입해 WBP 연동만 검증한다. */
void UMockRewardDriver::Start(URewardUIModel* UIModel)
{
	if (UIModel == nullptr)
	{
		return;
	}
	BindAutoConfirm(UIModel);

	// Mock은 실제 보상 지급자가 아니다. Claim/선택 입력이 UI에서 UIModel을 타고 올라오는지만 확인한다.
	mUIModel->OnRewardChosen.AddUniqueDynamic(this, &UMockRewardDriver::HandleChosen);

	// 가짜 보상 fixture: 돈 50, 경험치 30 (레벨 3, 40→70 / 최대 100). 밸런스 데이터로 쓰면 안 된다.
	FRewardUI Reward;
	Reward.mGoldGained = 50;
	Reward.mGoldBalance = 170;
	Reward.mExpGained = 30;
	Reward.mLevelBefore = 3;
	Reward.mLevelAfter = 3;
	Reward.mExpBefore = 40.f;
	Reward.mExpAfter = 70.f;
	Reward.mMaxExp = 100.f;
	mUIModel->SetReward(Reward);

	// 가짜 아티팩트 3중 1택. 골드는 Reward.mGoldGained로 별도 지급된다.
	TArray<FRewardChoiceUI> Choices;
	// mock fixture 이름은 실제 게임 콘텐츠가 아니라 개발용 더미다(실데이터는 DataAsset/API로 대체).
	// 로컬라이즈 대상이 아니므로 FText::FromString으로 런타임 문자열을 그대로 쓴다.
	const TCHAR* MockNames[] = { TEXT("Blood Chalice"), TEXT("Fang Amulet"), TEXT("Lucky Coin") };
	const FLinearColor Colors[] = { FLinearColor(0.86f, 0.98f, 0.94f, 1.f), FLinearColor(0.55f, 0.72f, 1.f, 1.f), FLinearColor(0.82f, 0.58f, 1.f, 1.f) };
	for (int32 i = 0; i < UE_ARRAY_COUNT(MockNames); ++i)
	{
		FRewardChoiceUI Choice;
		Choice.mChoiceIndex = i;
		Choice.mKind = ERewardChoiceKind::Artifact;
		Choice.mSourceAssetId = FPrimaryAssetId(
			TEXT("Artifact"), FName(*FString::Printf(TEXT("MockArtifact_%d"), i)));
		Choice.mName = FText::FromString(MockNames[i]);
		Choice.mDescription = LOCTEXT("(mock reward)", "(mock reward)");
		Choice.mRarityColor = Colors[i];
		Choices.Add(Choice);
	}
	mUIModel->SetRewardChoices(Choices);
}

/** @brief Claim 이벤트가 UIModel을 통해 되돌아왔는지만 확인한다. */
void UMockRewardDriver::HandleClaimed()
{
	UE_LOG(LogRD, Display, TEXT("MockRewardDriver: reward claimed"));

	FSimpleDelegate ClosedCallback = MoveTemp(mOnPreviewClosed);
	UnbindUIModel();
	if (ClosedCallback.IsBound())
	{
		ClosedCallback.Execute();
	}
}

/** @brief 보상 항목 선택 이벤트가 UIModel을 통해 되돌아왔는지만 확인한다. */
void UMockRewardDriver::HandleChosen(int32 ChoiceIndex)
{
	UE_LOG(LogRD, Display, TEXT("MockRewardDriver: reward choice %d"), ChoiceIndex);
}

void UMockRewardDriver::HandleClaimRequested(
	ERewardClaimKind ClaimKind,
	int32 ChoiceIndex)
{
	if (mUIModel != nullptr)
	{
		mUIModel->ConfirmRewardClaim(ClaimKind, ChoiceIndex);
	}
}

void UMockRewardDriver::HandleSelectionRequested(
	const FPrimaryAssetId RewardId)
{
	if (mUIModel != nullptr)
	{
		mUIModel->ConfirmSelectedReward(RewardId);
	}
}

void UMockRewardDriver::HandleGrantBundleRequested()
{
	if (mUIModel == nullptr)
	{
		return;
	}

	FRewardGrantBundleResultUI Result;
	for (const FRewardChoiceUI& Choice : mUIModel->GetGrantBundle().mItems)
	{
		Result.mGrantedItemIds.Add(Choice.mSourceAssetId);
	}
	mUIModel->ConfirmGrantBundle(Result);
}

void UMockRewardDriver::UnbindUIModel()
{
	if (mUIModel == nullptr)
	{
		return;
	}

	mUIModel->OnRewardClaimRequested.RemoveDynamic(
		this, &UMockRewardDriver::HandleClaimRequested);
	mUIModel->OnRewardSelectionRequested.RemoveDynamic(
		this, &UMockRewardDriver::HandleSelectionRequested);
	mUIModel->OnRewardGrantBundleRequested.RemoveDynamic(
		this, &UMockRewardDriver::HandleGrantBundleRequested);
	mUIModel->OnRewardClaimed.RemoveDynamic(
		this, &UMockRewardDriver::HandleClaimed);
	mUIModel->OnRewardChosen.RemoveDynamic(
		this, &UMockRewardDriver::HandleChosen);
	mUIModel = nullptr;
}

#undef LOCTEXT_NAMESPACE
