/*****************************************************************//**
 * @file   RewardPreviewCommand.cpp
 * @brief  저장 데이터에 손대지 않고 현재 보상 화면을 띄우는 개발용 명령.
 *********************************************************************/

#include "RDMinimal.h"

#include "Blueprint/UserWidget.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "UI/Reward/MockRewardDriver.h"
#include "UI/Reward/RewardUIModel.h"
#include "UI/Reward/RewardUITypes.h"
#include "UI/Reward/RewardSettlementWidgetBase.h"
#include "UObject/StrongObjectPtr.h"

#if !UE_BUILD_SHIPPING

namespace RewardPreview
{
	const TCHAR* RewardWidgetPath =
		TEXT("/Game/UI/RewardSettlement/WBP_RewardSettlement_Runtime.WBP_RewardSettlement_Runtime_C");

	TWeakObjectPtr<URewardSettlementWidgetBase> ShownWidget;
	TStrongObjectPtr<UMockRewardDriver> PreviewDriver;

	void ReleasePreviewDriver()
	{
		PreviewDriver.Reset();
	}

	void HandleWorldCleanup(
		UWorld* World,
		bool bSessionEnded,
		bool bCleanupResources)
	{
		if (ShownWidget.IsValid() && ShownWidget->GetWorld() == World)
		{
			PreviewDriver.Reset();
			ShownWidget.Reset();
		}
	}

	struct FWorldCleanupRegistration
	{
		FWorldCleanupRegistration()
			: Handle(FWorldDelegates::OnWorldCleanup.AddStatic(&HandleWorldCleanup))
		{
		}

		~FWorldCleanupRegistration()
		{
			if (Handle.IsValid())
			{
				FWorldDelegates::OnWorldCleanup.Remove(Handle);
			}
		}

		FDelegateHandle Handle;
	};

	FWorldCleanupRegistration WorldCleanupRegistration;

	void Show(UWorld* World)
	{
		if (ShownWidget.IsValid())
		{
			ShownWidget->RemoveFromParent();
			ShownWidget.Reset();
		}
		PreviewDriver.Reset();

		if (World == nullptr || World->IsGameWorld() == false)
		{
			UE_LOG(LogRD, Warning, TEXT("RD.RewardPreview: 게임 월드를 찾지 못했습니다."));
			return;
		}

		UClass* WidgetClass = LoadClass<URewardSettlementWidgetBase>(
			nullptr, RewardWidgetPath);
		if (WidgetClass == nullptr)
		{
			UE_LOG(LogRD, Warning,
				TEXT("RD.RewardPreview: %s 클래스를 찾지 못했습니다."),
				RewardWidgetPath);
			return;
		}

		APlayerController* Controller = World->GetFirstPlayerController();
		URewardSettlementWidgetBase* Widget = Controller != nullptr
			? CreateWidget<URewardSettlementWidgetBase>(Controller, WidgetClass)
			: CreateWidget<URewardSettlementWidgetBase>(World, WidgetClass);
		if (Widget == nullptr)
		{
			UE_LOG(LogRD, Warning, TEXT("RD.RewardPreview: 위젯 생성에 실패했습니다."));
			return;
		}

		// 모델의 Outer도 위젯으로 두고 BindUIModel의 UPROPERTY 참조까지 연결한다.
		// MockDriver나 게임플레이 객체가 사라져 프리뷰 값이 GC되는 일을 막는다.
		URewardUIModel* Model = NewObject<URewardUIModel>(Widget);

		FRewardUI Reward;
		Reward.mTitle = NSLOCTEXT(
			"RewardPreview", "RewardPreviewTitle", "전투 보상");
		Reward.mGoldGained = 75;
		Reward.mGoldBalance = 245;
		Reward.mExpGained = 50;

		const FText MercenaryNames[] = {
			NSLOCTEXT("RewardPreview", "KnightName", "Knight"),
			NSLOCTEXT("RewardPreview", "MageName", "Mage"),
			NSLOCTEXT("RewardPreview", "RogueName", "Rogue"),
		};
		UTexture2D* MercenaryPortraits[] = {
			LoadObject<UTexture2D>(nullptr, TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Mercenaries/T_MB_HireIcon_Knight.T_MB_HireIcon_Knight")),
			LoadObject<UTexture2D>(nullptr, TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Mercenaries/T_MB_HireIcon_Mage.T_MB_HireIcon_Mage")),
			LoadObject<UTexture2D>(nullptr, TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Mercenaries/T_MB_HireIcon_Rogue.T_MB_HireIcon_Rogue")),
		};
		for (int32 Index = 0; Index < UE_ARRAY_COUNT(MercenaryNames); ++Index)
		{
			FRewardMercenaryExpUI& Mercenary =
				Reward.mMercenaryExp.AddDefaulted_GetRef();
			Mercenary.mName = MercenaryNames[Index];
			Mercenary.mPortrait = MercenaryPortraits[Index];
			Mercenary.mLevel = Index + 1;
			Mercenary.mExpBefore = 25.f + 30.f * Index;
			Mercenary.mExpAfter = Mercenary.mExpBefore
				+ StaticCast<float>(Reward.mExpGained);
			Mercenary.mMaxExp = 250.f + 50.f * Index;
		}
		Model->SetReward(Reward);

		TArray<FRewardChoiceUI> Choices;
		const FText ArtifactNames[] = {
			NSLOCTEXT("RewardPreview", "ArtifactRewardBloodChalice", "피의 성배"),
			NSLOCTEXT("RewardPreview", "ArtifactRewardFangAmulet", "야수의 송곳니"),
			NSLOCTEXT("RewardPreview", "ArtifactRewardLuckyCoin", "행운의 주화") };
		UTexture2D* ArtifactIcons[] = {
			LoadObject<UTexture2D>(nullptr, TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Artifacts/T_Artifact_BloodChalice.T_Artifact_BloodChalice")),
			LoadObject<UTexture2D>(nullptr, TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Artifacts/T_Artifact_FangAmulet.T_Artifact_FangAmulet")),
			LoadObject<UTexture2D>(nullptr, TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Artifacts/T_Artifact_LuckyCoin.T_Artifact_LuckyCoin")) };
		for (int32 Index = 0; Index < UE_ARRAY_COUNT(ArtifactNames); ++Index)
		{
			FRewardChoiceUI& Artifact = Choices.AddDefaulted_GetRef();
			Artifact.mChoiceIndex = Index;
			Artifact.mKind = ERewardChoiceKind::Artifact;
			Artifact.mSourceAssetId = FPrimaryAssetId(
				TEXT("Artifact"), FName(*FString::Printf(TEXT("PreviewArtifact_%d"), Index)));
			Artifact.mName = ArtifactNames[Index];
			Artifact.mDescription = NSLOCTEXT(
				"RewardPreview", "ArtifactRewardDescription", "아티팩트 선택 보상");
			Artifact.mIcon = ArtifactIcons[Index];
		}
		Model->SetRewardChoices(Choices);

		// 저장/PartyModel을 건드리지 않고 눌린 행을 즉시 성공 처리해
		// 실제 claim 완료 상태와 닫기 버튼까지 프리뷰할 수 있게 한다.
		PreviewDriver.Reset(NewObject<UMockRewardDriver>(GetTransientPackage()));
		PreviewDriver->BindAutoConfirm(Model);
		PreviewDriver->SetOnPreviewClosed(
			FSimpleDelegate::CreateStatic(&ReleasePreviewDriver));

		Widget->BindUIModel(Model);
		Widget->OpenUI();
		ShownWidget = Widget;

		UE_LOG(LogRD, Display,
			TEXT("RD.RewardPreview: 저장 데이터 변경 없이 보상 화면을 열었습니다."));
	}

	void ShowChoiceStep(UWorld* World)
	{
		Show(World);
		if (ShownWidget.IsValid() == false)
		{
			return;
		}

		ShownWidget->ContinueToNext();
		UE_LOG(LogRD, Display,
			TEXT("RD.RewardChoicePreview: 선택 카드 단계를 열었습니다."));
	}

	FAutoConsoleCommandWithWorld ShowCommand(
		TEXT("RD.RewardPreview"),
		TEXT("저장 데이터 변경 없이 골드/용병 EXP/스킬/장비 보상 화면을 연다."),
		FConsoleCommandWithWorldDelegate::CreateStatic(&Show));

	FAutoConsoleCommandWithWorld ShowChoiceCommand(
		TEXT("RD.RewardChoicePreview"),
		TEXT("저장 데이터 변경 없이 보상 선택 카드 단계를 바로 연다."),
		FConsoleCommandWithWorldDelegate::CreateStatic(&ShowChoiceStep));
}

#endif
