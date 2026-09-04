/*****************************************************************//**
 * @file   CombatLayoutCaptureTests.cpp
 * @brief  전투 HUD 배치안 WBP를 PNG로 뽑아 눈으로 비교할 수 있게 한다.
 * @details
 * 처음에는 배치안 스무 갈래를 나란히 놓고 고르려고 만들었다. 고르고 난
 * 지금은 **구운 WBP 가 뜻대로 나왔나 보는 눈**으로 쓴다. 에디터에서 열어
 * 보면 창 크기와 확대율이 매번 달라 비교가 안 되므로, 같은 해상도로
 * 오프스크린 렌더해 파일로 남긴다.
 *
 * 렌더 결과가 단색이면 실패로 처리한다. 위젯 수명이나 표시 상태가 깨지면
 * 빈 화면이 나오는데, 그것도 "성공한 캡처"처럼 보이기 때문이다.
 * @author 박용수
 * @date   2026-07-26
 *********************************************************************/

#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/OverlaySlot.h"
#include "Components/Button.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/WidgetSwitcher.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "ImageUtils.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "RHI.h"
#include "Slate/WidgetRenderer.h"
#include "TextureCompiler.h"
#include "UI/Combat/CombatLayoutHUDWidget.h"
#include "UI/Combat/CombatUIModel.h"
#include "UI/Hire/MercenaryHireWidget.h"
#include "UI/Reward/RewardSettlementWidgetBase.h"
#include "UI/Reward/RewardUIModel.h"
#include "Widgets/Colors/SColorBlock.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"

#if WITH_EDITOR
#include "Editor.h"
#endif

#if WITH_EDITOR

namespace CombatLayoutCapture
{
	/**
	 * @brief 이 위젯이 쓰는 텍스처를 다 올려 놓고 기다린다.
	 *
	 * @details
	 * 오프스크린으로 한 번 그리고 끝내므로, 스트리밍이 다음 프레임에 올릴
	 * 셈이면 영영 안 올라온다. 밉을 붙잡아 두고 스트리밍을 한 번 돌린다.
	 * @param Root 훑을 위젯
	 */
	void ForceTexturesResident(UWidget* Root)
	{
		UUserWidget* User = Cast<UUserWidget>(Root);
		if (User == nullptr || User->WidgetTree == nullptr)
		{
			return;
		}
		User->WidgetTree->ForEachWidget([](UWidget* Widget)
		{
			UImage* Image = Cast<UImage>(Widget);
			UTexture2D* Texture = Image != nullptr
				? Cast<UTexture2D>(Image->GetBrush().GetResourceObject()) : nullptr;
			if (Texture != nullptr)
			{
				// 밉을 붙잡아 두고 **그 자리에서 기다린다.** 스트리밍 관리자를
				// 통째로 돌리면 오히려 지금 올라와 있던 것까지 내려간다 --
				// 그렇게 해 봤더니 판이 하나도 안 남았다.
				Texture->bForceMiplevelsToBeResident = true;
				Texture->SetForceMipLevelsToBeResident(30.f);
				Texture->WaitForStreaming();
			}
		});
		FlushRenderingCommands();
	}

	/**
	 * @brief 찍을 배치. 시안4로 정해져서 하나뿐이다.
	 *
	 * @details
	 * 스무 갈래를 만들어 놓고 고르던 때에는 여기에 스무 줄이 있었다. 고르고
	 * 나서 열아홉을 지웠는데, **찍는 기계는 남겼다** -- 구운 WBP 가 뜻대로
	 * 나왔는지 눈으로 볼 길이 이것뿐이다. 코드를 읽고 "맞다" 고 말하는 것과
	 * 나온 그림이 그런 것은 다른 이야기다.
	 */
	const TCHAR* LayoutClassPaths[] = {
		TEXT("/Game/UI/CombatLayouts/WBP_CombatHUD04.WBP_CombatHUD04_C"),
	};

	/** @brief 폰 가로 화면 실물 크기. 배치안 평가는 이 한 장이면 충분하다. */
	//: 시안 원본과 같은 크기로 찍는다.
	//:
	//: 배치는 1920x1080 캔버스에 짜여 있고, 인게임에서는 UI 스케일 규칙이
	//: 짧은변 941 을 만나 0.871 로 줄여 그린다. 캡처도 그 배율을 그대로 걸어야
	//: 시안(1672x941)과 픽셀이 1:1 로 맞는다. 1920 으로 찍고 시안을 확대해
	//: 비교하면 비율만 맞고 크기는 안 맞아, 요소 단위로 보면 어긋난다.
	constexpr int32 DesignWidth = 1920;
	constexpr int32 DesignHeight = 1080;
	constexpr int32 CaptureWidth = 1672;
	constexpr int32 CaptureHeight = 941;

	/** @brief 1920 캔버스를 1672 로 줄여 그리도록 감싼다. 인게임과 같은 배율. */
	TSharedRef<SWidget> ScaleToCapture(const TSharedRef<SWidget>& Inner,
		const int32 TargetHeight = CaptureHeight)
	{
		return SNew(SScaleBox)
			.Stretch(EStretch::UserSpecified)
			.UserSpecifiedScale(float(TargetHeight) / float(DesignHeight))
			[
				SNew(SBox)
				.WidthOverride(float(DesignWidth))
				.HeightOverride(float(DesignHeight))
				[
					Inner
				]
			];
	}

	FString OutputDirectory()
	{
		return FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("UI"), TEXT("CombatLayouts"));
	}

	/**
	 * @brief 브러시가 쓰는 텍스처를 전부 상주시킨다.
	 *
	 * @details
	 * 오프스크린 렌더는 한 번에 끝나서 스트리밍을 기다려 주지 않는다. 큰
	 * 텍스처는 아직 안 올라온 채로 그려지고, 그 결과가 "프레임 조각(작은
	 * 텍스처)만 보이고 초상화·링·아이콘은 안 보이는" 화면이다. 인게임에서는
	 * 정상적으로 스트리밍되므로 이건 캡처 쪽 문제지 WBP 문제가 아니다.
	 *
	 * @return 상주시킨 텍스처 수. 0이면 브러시가 비어 있다는 뜻이다.
	 */
	int32 ResidentBrushTextures(UUserWidget& Widget)
	{
		if (Widget.WidgetTree == nullptr)
		{
			return 0;
		}

		TArray<UWidget*> Widgets;
		Widget.WidgetTree->GetAllWidgets(Widgets);
		int32 Count = 0;

		auto MakeResident = [&Count](const FSlateBrush& Brush)
		{
			UTexture2D* Texture = Cast<UTexture2D>(Brush.GetResourceObject());
			if (Texture == nullptr)
			{
				return;
			}
			// 순서가 중요하다. UpdateResource() 가 그릴 자원을 만드는
			// 호출이고, 기다리기는 그 다음이다. 자원이 없는 텍스처는 아무리
			// 기다려도 안 올라온다 -- 한 번도 안 그려 본 텍스처가 그렇다.
			//
			// 이 줄을 지웠다가 열아홉 장이 통째로 비었다. 지우기 전에는
			// 기다린 뒤에 불러서, 방금 기다린 것이 무효가 되는 반대 문제가
			// 있었다. 빼는 것이 아니라 앞으로 옮기는 것이 답이다.
			Texture->UpdateResource();
			Texture->SetForceMipLevelsToBeResident(30.0f);
			Texture->WaitForStreaming();
			++Count;
		};

		for (UWidget* Candidate : Widgets)
		{
			// Every widget type that can carry a texture, not just the two that
			// happened to matter first. The HP bar drew nothing for a whole
			// pass because its brushes live inside a style struct and this walk
			// only looked at Image and Border.
			if (const UImage* Image = Cast<UImage>(Candidate))
			{
				MakeResident(Image->GetBrush());
			}
			else if (const UBorder* Border = Cast<UBorder>(Candidate))
			{
				MakeResident(Border->Background);
			}
			else if (const UProgressBar* Bar = Cast<UProgressBar>(Candidate))
			{
				MakeResident(Bar->GetWidgetStyle().BackgroundImage);
				MakeResident(Bar->GetWidgetStyle().FillImage);
			}
			else if (const UButton* Button = Cast<UButton>(Candidate))
			{
				MakeResident(Button->GetStyle().Normal);
				MakeResident(Button->GetStyle().Hovered);
				MakeResident(Button->GetStyle().Pressed);
				MakeResident(Button->GetStyle().Disabled);
			}
		}
		FlushRenderingCommands();
		return Count;
	}

	/**
	 * @brief 배치안 하나를 렌더해서 PNG로 저장한다. 실패 사유는 OutError로.
	 * @param bShowMercenaryPanel 참이면 전투 기본 HUD 대신 보유 용병 탭을 찍는다.
	 */
	bool CaptureLayout(UWorld& World, const TCHAR* ClassPath, FString& OutError,
		const bool bShowMercenaryPanel = false,
		const bool bShowInventoryPage = false,
		const bool bShowRewardChoices = false,
		const bool bGoldOnlyReward = false,
		const bool bShowRewardExperience = false,
		const int32 OutputWidth = CaptureWidth,
		const int32 OutputHeight = CaptureHeight,
		const bool bUseDirectViewportSize = false,
		const TCHAR* AdditionalSuffix = TEXT(""))
	{
		UClass* LayoutClass = LoadClass<UUserWidget>(nullptr, ClassPath);
		if (LayoutClass == nullptr)
		{
			OutError = FString::Printf(TEXT("배치안 클래스를 못 찾음: %s"), ClassPath);
			return false;
		}

		// 공통 부모로 만든다. 캡처는 그려서 찍는 일이라 화면이 전투 배치안인지
		// 고용 게시판인지 알 필요가 없다.
		UUserWidget* Layout = CreateWidget<UUserWidget>(&World, LayoutClass);
		if (Layout == nullptr)
		{
			OutError = FString::Printf(TEXT("위젯 생성 실패: %s"), ClassPath);
			return false;
		}

		// URDUserWidget은 OpenUI() 전까지 Collapsed다. 여기서는 뷰포트에 올리지
		// 않고 그리기만 하므로 표시 상태를 직접 세운다.
		Layout->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		const bool bShowTargetingControls =
			FCString::Strcmp(AdditionalSuffix, TEXT("_Targeting")) == 0;
		const bool bShowWaitingConfirm =
			FCString::Strcmp(AdditionalSuffix, TEXT("_WaitingConfirm")) == 0;
		const bool bShowCommandCards =
			FCString::Strcmp(AdditionalSuffix, TEXT("_Commands")) == 0;
		const bool bShowStatuslessSummary =
			FCString::Strcmp(AdditionalSuffix, TEXT("_NoStatus")) == 0;

		// 몬스터 탭 WBP 에는 번역 키 없이 구워진 라벨이 남아 있고, 실게임은
		// EnsureMonsterTabWidget() 이 로컬라이즈 텍스트로 갈아 끼운다. 캡처는
		// 그 배선을 우회하므로 같은 덮어쓰기를 여기서 재현한다 -- 안 하면
		// 언어별 캡처에 구형 한글이 남는다(0823 검수).
		if (UTextBlock* CritLabel = Cast<UTextBlock>(
			Layout->GetWidgetFromName(TEXT("MonsterChip2Label"))))
		{
			CritLabel->SetText(NSLOCTEXT("CombatHUD", "MercenaryCrit", "치명타"));
		}
		if (UTextBlock* BackText = Cast<UTextBlock>(
			Layout->GetWidgetFromName(TEXT("MonsterBackText"))))
		{
			BackText->SetText(NSLOCTEXT("CombatHUD", "MercenaryBack", "닫기"));
		}
		const TSharedRef<SWidget> LayoutSlate = Layout->TakeWidget();
		// 기본 HUD 캡처에도 실제 플레이어 턴을 흘린다. 그래야 렌더 틱에서
		// 퀵바가 다시 접히지 않고, 런타임과 같은 경로로 쿨타임 숫자까지 남는다.
		if (!bShowMercenaryPanel)
		{
			if (UCombatLayoutHUDWidget* CombatHUD = Cast<UCombatLayoutHUDWidget>(Layout))
			{
				UCombatUIModel* PreviewModel = NewObject<UCombatUIModel>(CombatHUD);
				CombatHUD->BindUIModel(PreviewModel);
				FUnitUI Player;
				Player.mUnitId = 1;
				Player.mIsPlayer = true;
				Player.mName = bShowStatuslessSummary
					? FText::FromString(TEXT("Ranger")) : FText::GetEmpty();
				Player.mHP = bShowStatuslessSummary ? 80.f : 0.f;
				Player.mMaxHP = bShowStatuslessSummary ? 80.f : 0.f;
				Player.mActionPoints = bShowStatuslessSummary ? 0 : 12;
				Player.mMaxActionPoints = bShowStatuslessSummary ? 12 : 15;
				Player.mSpeedPoint = bShowStatuslessSummary ? 8.f : 0.f;
				Player.mMovementPoint = bShowStatuslessSummary ? 0.f : 12.f;
				Player.mMaxMovementPoint = bShowStatuslessSummary ? 12.f : 15.f;
				if (bShowStatuslessSummary)
				{
					Player.mPortrait = LoadObject<UTexture2D>(nullptr,
						TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/"
							"Mercenaries/T_MB_HireIcon_Ranger.T_MB_HireIcon_Ranger"));
				}
				PreviewModel->SetUnitUIs({ Player });

				const TCHAR* SkillIconPaths[] = {
					TEXT("/Game/SVN/OutSideAsset/AICreation/UI/CombatHUD/SkillIcons/T_SkillIcon_Barrier.T_SkillIcon_Barrier"),
					TEXT("/Game/SVN/OutSideAsset/AICreation/UI/CombatHUD/SkillIcons/T_SkillIcon_Slash.T_SkillIcon_Slash"),
					TEXT("/Game/SVN/OutSideAsset/AICreation/UI/CombatHUD/SkillIcons/T_SkillIcon_Whirlwind.T_SkillIcon_Whirlwind"),
					TEXT("/Game/SVN/OutSideAsset/AICreation/UI/CombatHUD/SkillIcons/T_SkillIcon_HeavySmash.T_SkillIcon_HeavySmash"),
					TEXT("/Game/SVN/OutSideAsset/AICreation/UI/CombatHUD/SkillIcons/T_SkillIcon_Charge.T_SkillIcon_Charge") };
				TArray<FSkillUI> Skills;
				for (int32 Index = 0; Index < UE_ARRAY_COUNT(SkillIconPaths); ++Index)
				{
					FSkillUI Skill;
					Skill.mSkillIndex = Index;
					Skill.mIcon = LoadObject<UTexture2D>(nullptr, SkillIconPaths[Index]);
					Skill.mIsUsable = Index != 3;
					Skill.mCooldownTurns = Index == 3 ? 4 : 0;
					Skill.mRemainingCooldown = Index == 3 ? 2 : 0;
					Skills.Add(Skill);
				}
				PreviewModel->SetSkillUIs(Skills);
				FCombatPendingActionUI PendingAction;
				PendingAction.mType = ECombatPendingActionType::Skill;
				PendingAction.mActionPointCost = 1;
				PreviewModel->SetPendingAction(PendingAction);
				FTurnUI Turn;
				Turn.mCurrentUnitId = Player.mUnitId;
				Turn.mTurnOrderUnitIds = { Player.mUnitId };
				Turn.mPhase = bShowTargetingControls
					? ECombatBuildPhaseUI::Preview
					: bShowWaitingConfirm
						? ECombatBuildPhaseUI::AimSelection
						: ECombatBuildPhaseUI::None;
				PreviewModel->SetTurnUI(Turn);
				PreviewModel->OnBeginAnyTurn.Broadcast(nullptr);
				if (bShowStatuslessSummary)
				{
					UProgressBar* HPBar = Cast<UProgressBar>(
						CombatHUD->GetWidgetFromName(TEXT("AllyHPBar")));
					UOverlaySlot* HPSlot = HPBar != nullptr
						? Cast<UOverlaySlot>(HPBar->Slot) : nullptr;
					if (HPSlot == nullptr
						|| HPSlot->GetPadding().Left > 8.01f
						|| HPSlot->GetPadding().Right > 8.01f)
					{
						OutError = TEXT("HP 바 좌우 패딩 때문에 100%가 프레임을 채우지 못함");
						return false;
					}
				}
				if (bShowCommandCards)
				{
					if (UButton* SkillToggle = Cast<UButton>(
						CombatHUD->GetWidgetFromName(TEXT("SkillToggleButton"))))
					{
						SkillToggle->OnClicked.Broadcast();
					}
				}
			}
		}
		if (bUseDirectViewportSize)
		{
			if (UMercenaryHireWidget* Hire = Cast<UMercenaryHireWidget>(Layout))
			{
				Hire->ApplyResponsiveLayoutForTest(
					FVector2D(OutputWidth, OutputHeight));
			}
		}
		if (URewardSettlementWidgetBase* RewardWidget =
			Cast<URewardSettlementWidgetBase>(Layout))
		{
			URewardUIModel* PreviewModel = NewObject<URewardUIModel>(RewardWidget);
			FRewardUI PreviewReward;
			PreviewReward.mTitle = NSLOCTEXT(
				"CombatLayoutCapture", "RewardTitle", "전투 보상");
			PreviewReward.mGoldGained = 22;
			PreviewReward.mExpGained = 50;
			UTexture2D* PreviewPortraits[] = {
				LoadObject<UTexture2D>(nullptr, TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Mercenaries/T_MB_HireIcon_Knight.T_MB_HireIcon_Knight")),
				LoadObject<UTexture2D>(nullptr, TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Mercenaries/T_MB_HireIcon_Mage.T_MB_HireIcon_Mage")),
				LoadObject<UTexture2D>(nullptr, TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Mercenaries/T_MB_HireIcon_Rogue.T_MB_HireIcon_Rogue")) };
			const FText PreviewNames[] = {
				NSLOCTEXT("CombatLayoutCapture", "RewardKnight", "기사"),
				NSLOCTEXT("CombatLayoutCapture", "RewardMage", "마법사"),
				NSLOCTEXT("CombatLayoutCapture", "RewardRogue", "도적") };
			for (int32 Index = 0; Index < 3; ++Index)
			{
				FRewardMercenaryExpUI Mercenary;
				Mercenary.mName = PreviewNames[Index];
				Mercenary.mPortrait = PreviewPortraits[Index];
				Mercenary.mLevel = Index + 1;
				Mercenary.mExpBefore = 25.f + Index * 40.f;
				Mercenary.mExpAfter = Mercenary.mExpBefore + 50.f;
				Mercenary.mMaxExp = 250.f;
				PreviewReward.mMercenaryExp.Add(Mercenary);
			}
			PreviewModel->SetReward(PreviewReward);
			TArray<FRewardChoiceUI> PreviewChoices;
			const FText Names[] = {
				NSLOCTEXT("CombatLayoutCapture", "BloodChaliceChoice", "피의 성배"),
				NSLOCTEXT("CombatLayoutCapture", "FangAmuletChoice", "야수의 송곳니"),
				NSLOCTEXT("CombatLayoutCapture", "LuckyCoinChoice", "행운의 주화") };
			UTexture2D* Icons[] = {
				LoadObject<UTexture2D>(nullptr, TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Artifacts/T_Artifact_BloodChalice.T_Artifact_BloodChalice")),
				LoadObject<UTexture2D>(nullptr, TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Artifacts/T_Artifact_FangAmulet.T_Artifact_FangAmulet")),
				LoadObject<UTexture2D>(nullptr, TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Artifacts/T_Artifact_LuckyCoin.T_Artifact_LuckyCoin")) };
			for (int32 Index = 0; Index < 3; ++Index)
			{
				FRewardChoiceUI Choice;
				Choice.mChoiceIndex = Index;
				Choice.mKind = ERewardChoiceKind::Artifact;
				Choice.mSourceAssetId = FPrimaryAssetId(
					TEXT("Artifact"), FName(*FString::Printf(TEXT("CaptureArtifact_%d"), Index)));
				Choice.mName = Names[Index];
				Choice.mIcon = Icons[Index];
				PreviewChoices.Add(Choice);
			}
			PreviewModel->SetRewardChoices(bGoldOnlyReward
				? TArray<FRewardChoiceUI>() : PreviewChoices);
			RewardWidget->BindUIModel(PreviewModel);
			if (!bShowRewardExperience)
			{
				RewardWidget->ContinueToNext();
			}
			if (bShowRewardChoices)
			{
				RewardWidget->CompleteChestRevealForTest();
				if (!bGoldOnlyReward)
				{
					RewardWidget->AdvanceGoldToArtifactForTest();
				}
			}
		}
		// TakeWidget()에서 NativeConstruct가 돌며 패널을 기본 Collapsed로
		// 되돌린다. 변형 상태는 Construct가 끝난 뒤 세워야 캡처에 남는다.
		if (bShowMercenaryPanel && Layout->WidgetTree != nullptr)
		{
			auto FindDeep = [Layout](const FName Name) -> UWidget*
			{
				if (UWidget* Direct = Layout->WidgetTree->FindWidget(Name))
				{
					return Direct;
				}
				UWidget* Found = nullptr;
				Layout->WidgetTree->ForEachWidget([&Found, Name](UWidget* Candidate)
				{
					if (Found != nullptr)
					{
						return;
					}
					if (const UUserWidget* Nested = Cast<UUserWidget>(Candidate))
					{
						if (Nested->WidgetTree != nullptr)
						{
							Found = Nested->WidgetTree->FindWidget(Name);
						}
					}
				});
				return Found;
			};
			// 용병 패널은 자기 판(WBP_MercenaryPanel)으로 나갔다. HUD 트리에서
			// 바로 찾으면 안 나오므로 자식 UserWidget 안까지 훑는다.
			UWidget* MercenaryPanel = FindDeep(TEXT("MercenaryPanel"));
			if (MercenaryPanel != nullptr)
			{
				MercenaryPanel->SetVisibility(ESlateVisibility::Visible);
			}
			UWidgetTree* MercenaryTree = MercenaryPanel != nullptr
				? MercenaryPanel->GetTypedOuter<UWidgetTree>() : nullptr;
			if (bShowInventoryPage)
			{
				// 캡처는 버튼 클릭을 거치지 않고 페이지 가시성만 직접 바꾼다.
				// 런타임과 같은 최종 화면을 남기도록 제목도 함께 전환한다.
				if (UTextBlock* Title = Cast<UTextBlock>(MercenaryTree != nullptr
					? MercenaryTree->FindWidget(TEXT("MercenaryTitleText")) : nullptr))
				{
					// 런타임과 같은 키를 써야 언어별 캡처에서 번역이 붙는다.
					Title->SetText(NSLOCTEXT("CombatHUD",
						"MercenaryInventoryPageTitle", "인벤토리"));
				}
				for (const TCHAR* Name : {
					TEXT("MercDetailSection"), TEXT("MercenaryHeroPortrait"),
					TEXT("MercenaryPortraitFrame"), TEXT("MercenaryNamePlate"),
					TEXT("MercenaryDetailName"), TEXT("MercenaryDetailHP"),
					TEXT("MercenaryDetailAP"), TEXT("MercenaryDetailSpeed"),
					TEXT("MercenaryCritPlate"), TEXT("MercenaryCritLabel"),
					TEXT("MercenaryCritValue"), TEXT("MercenarySkillHeading"),
					TEXT("MercenarySkillDivider") })
				{
					if (UWidget* Detail = MercenaryTree != nullptr
						? MercenaryTree->FindWidget(FName(Name)) : nullptr)
					{
						Detail->SetVisibility(ESlateVisibility::Collapsed);
					}
				}
				for (int32 Index = 0; Index < 3; ++Index)
				{
					for (const TCHAR* Prefix : { TEXT("MercenaryChip") })
					{
						for (const TCHAR* Suffix : { TEXT("Frame"), TEXT("Label") })
						{
							if (UWidget* Detail = MercenaryTree != nullptr
								? MercenaryTree->FindWidget(FName(*FString::Printf(
									TEXT("%s%d%s"), Prefix, Index, Suffix))) : nullptr)
							{
								Detail->SetVisibility(ESlateVisibility::Collapsed);
							}
						}
					}
				}
				for (int32 Index = 0; Index < 6; ++Index)
				{
					for (const TCHAR* Prefix : { TEXT("MercenarySkillFrame"),
						TEXT("MercenarySkillIcon"), TEXT("MercenarySkillName"),
						TEXT("MercenarySkillCost"), TEXT("MercenarySkillButton") })
					{
						if (UWidget* Detail = MercenaryTree != nullptr
							? MercenaryTree->FindWidget(FName(*FString::Printf(
								TEXT("%s_%d"), Prefix, Index))) : nullptr)
						{
							Detail->SetVisibility(ESlateVisibility::Collapsed);
						}
					}
				}
				if (UWidget* Inventory = MercenaryTree != nullptr
					? MercenaryTree->FindWidget(TEXT("MercenaryInventoryPage")) : nullptr)
				{
					Inventory->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
				}
			}
		}
		Layout->ForceLayoutPrepass();

		const int32 TextureCount = ResidentBrushTextures(*Layout);
		if (TextureCount == 0)
		{
			OutError = TEXT("브러시에 텍스처가 하나도 없다. 아트가 안 붙었다");
			return false;
		}
		UE_LOG(LogTemp, Display, TEXT("[CombatLayout] %d textures resident"),
			TextureCount);

		// 전장이 뒤에 깔린다고 가정한 어두운 바탕. 완전한 검정에 대고 보면
		// 패널이 실제보다 잘 읽혀서 배치 판단이 후해진다.
		// 기지값 색 띠. 리니어 {0, 0.05, 0.2158, 1.0}은 감마 인코딩을 정확히
		// 한 번 거치면 sRGB {0, 65, 128, 255}가 된다. 다르게 읽히면 파이프라인
		// 어딘가에서 변환이 빠졌거나 두 번 들어간 것이다.
		// Legacy 16:9 comparisons reproduce the device DPI scale explicitly. Fold
		// captures instead allocate the WBP the full framebuffer geometry so its own
		// anchors and regional ScaleBoxes receive the real 2176x1812 aspect ratio.
		const TSharedRef<SWidget> PresentedLayout = bUseDirectViewportSize
			? LayoutSlate
			: ScaleToCapture(LayoutSlate, OutputHeight);
		const TSharedRef<SWidget> CaptureRoot =
			SNew(SOverlay)
			+ SOverlay::Slot()
			[
				SNew(SColorBlock).Color(FLinearColor(0.008f, 0.009f, 0.011f, 1.0f))
			]
			+ SOverlay::Slot()
			[
				PresentedLayout
			]
			// 색 띠는 배치 위에 그린다.
			//
			// 아래에 깔았더니 라운드 판을 화면 맨 위까지 올린 순간 띠가 가려져
			// 열 장이 통째로 저장을 거부당했다. 검증용 표식이 검증 대상에
			// 가려지면 안 된다. 읽고 나서 지우므로 결과 그림에는 안 남는다.
			+ SOverlay::Slot()
			.HAlign(HAlign_Left).VAlign(VAlign_Top)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth()
				[ SNew(SBox).WidthOverride(40).HeightOverride(10)
					[ SNew(SColorBlock).Color(FLinearColor(0.f, 0.f, 0.f)) ] ]
				+ SHorizontalBox::Slot().AutoWidth()
				[ SNew(SBox).WidthOverride(40).HeightOverride(10)
					[ SNew(SColorBlock).Color(FLinearColor(0.05f, 0.05f, 0.05f)) ] ]
				+ SHorizontalBox::Slot().AutoWidth()
				[ SNew(SBox).WidthOverride(40).HeightOverride(10)
					[ SNew(SColorBlock).Color(FLinearColor(0.2158f, 0.2158f, 0.2158f)) ] ]
				+ SHorizontalBox::Slot().AutoWidth()
				[ SNew(SBox).WidthOverride(40).HeightOverride(10)
					[ SNew(SColorBlock).Color(FLinearColor(1.f, 1.f, 1.f)) ] ]
			];

		// 렌더러가 감마 공간에 직접 그린다.
		//
		// 세 번 틀리고 내린 결론: RGBA8 타깃에서는 ReadPixels의 LinearToGamma
		// 플래그가 적용되지 않는다. 변환을 읽기 단계에 미루면 리니어 값이
		// 바이트로 그대로 나가 절반쯤 어두운 그림이 남는다 -- 면 텍스처 111이
		// 48로 찍힌 원인. 그래서 변환은 렌더러가 하고, 읽기는 그대로 옮긴다.
		//
		// 예전에 이 조합을 "씻긴다"며 버렸는데, 그때 씻겨 보인 건 배경 색을
		// sRGB 감각으로 적어 놓고 리니어로 해석시킨 탓이었다. 파이프라인이
		// 아니라 배경 값이 문제였다.
		//
		// 그리고 이번부터 캡처가 스스로 증명한다: 아래에서 기지값 색 띠를
		// 같이 그려 읽은 값이 기대값과 다르면 캡처 자체를 실패로 처리한다.
		// 판 그림을 먼저 다 올려 놓는다.
		//
		// 스트리밍이 아직 안 올린 텍스처는 그리는 순간 빈 자리로 나온다. 매번
		// **다른 판**이 빠져서, 굽는 쪽이 깨진 것처럼 보였다 -- 한 번은 위쪽
		// 세 장이, 다음 번에는 두루마리 한 장이 없었다. 찍는 눈이 흔들리면
		// 없는 문제를 쫓게 된다.
		// 위젯 생성 시점에 처음 로드된 텍스처는 비동기 컴파일이 걸려 있어
		// 스트리밍 대기만으로는 빈 자리로 그려진다. 렌더 직전에 컴파일을
		// 끝까지 기다린다 (경험치 판이 세션 첫 캡처마다 비던 원인).
		FTextureCompilingManager::Get().FinishAllCompilation();
		ForceTexturesResident(Layout);

		FWidgetRenderer Renderer(true, true);
		Renderer.SetIsPrepassNeeded(true);
		UTextureRenderTarget2D* RenderTarget = Renderer.DrawWidget(
			CaptureRoot, FVector2D(OutputWidth, OutputHeight));
		if (RenderTarget == nullptr)
		{
			OutError = TEXT("렌더 타깃이 만들어지지 않음");
			return false;
		}

		FlushRenderingCommands();
		TArray<FColor> Pixels;
		FReadSurfaceDataFlags ReadFlags(RCM_UNorm);
		ReadFlags.SetLinearToGamma(false);
		if (!RenderTarget->GameThread_GetRenderTargetResource()->ReadPixels(Pixels, ReadFlags)
			|| Pixels.Num() != OutputWidth * OutputHeight)
		{
			OutError = TEXT("렌더 결과를 읽지 못함");
			return false;
		}

		// 이 경로는 인코딩이 두 번 걸린다. 색 띠 실측: 리니어 0.05가 136으로
		// 읽혔는데 0.05를 두 번 인코딩하면 정확히 137이다. 그래서 저장 전에
		// 한 번 되돌린다 -- 결과는 정확히 한 번 인코딩된 sRGB가 되고, 아래
		// 검증이 그걸 확인한다. 엔진 플래그 조합을 더 뒤지는 것보다 측정값에
		// 맞춘 보정 한 줄이 낫고, 틀리면 검증이 잡는다.
		for (FColor& Pixel : Pixels)
		{
			Pixel.R = uint8(FMath::RoundToInt(
				255.f * FMath::Pow(Pixel.R / 255.f, 2.2f)));
			Pixel.G = uint8(FMath::RoundToInt(
				255.f * FMath::Pow(Pixel.G / 255.f, 2.2f)));
			Pixel.B = uint8(FMath::RoundToInt(
				255.f * FMath::Pow(Pixel.B / 255.f, 2.2f)));
		}

		// 색 띠 검증: 위 보정까지 거친 결과가 정확히 한 번 인코딩이라면
		// 이 값이 나와야 한다.
		{
			const int32 Expected[4] = { 0, 65, 128, 255 };
			const int32 SampleX[4] = { 20, 60, 100, 140 };
			for (int32 Step = 0; Step < 4; ++Step)
			{
				const FColor& Pixel = Pixels[5 * OutputWidth + SampleX[Step]];
				if (FMath::Abs(int32(Pixel.R) - Expected[Step]) > 6)
				{
					OutError = FString::Printf(
						TEXT("감마 검증 실패: 띠 %d칸이 %d로 읽힘 (기대 %d). ")
						TEXT("이 캡처의 색은 믿을 수 없다"),
						Step, Pixel.R, Expected[Step]);
					return false;
				}
			}
			// 통과했으면 띠를 배경색으로 지워 그림을 깨끗하게 남긴다.
			const FColor Background = Pixels[30 * OutputWidth + 400];
			for (int32 Y = 0; Y < 12; ++Y)
			{
				for (int32 X = 0; X < 170; ++X)
				{
					Pixels[Y * OutputWidth + X] = Background;
				}
			}
		}

		// 단색이면 위젯이 안 그려진 것이다. 그대로 저장하면 "배경만 나온 캡처"가
		// 성공처럼 남는다.
		uint8 MinChannel = 255;
		uint8 MaxChannel = 0;
		for (const FColor& Pixel : Pixels)
		{
			MinChannel = FMath::Min3(MinChannel, Pixel.R, FMath::Min(Pixel.G, Pixel.B));
			MaxChannel = FMath::Max3(MaxChannel, Pixel.R, FMath::Max(Pixel.G, Pixel.B));
		}
		if (int32(MaxChannel) - int32(MinChannel) < 8)
		{
			OutError = TEXT("캡처가 단색이다. 위젯이 그려지지 않았다");
			return false;
		}

		FString Stem = FString(ClassPath);
		Stem.Split(TEXT("."), nullptr, &Stem, ESearchCase::CaseSensitive, ESearchDir::FromEnd);
		Stem.RemoveFromEnd(TEXT("_C"));
		FString CaptureSuffix = bShowRewardExperience ? TEXT("_Experience")
			: (bGoldOnlyReward ? TEXT("_GoldOnly")
				: (bShowRewardChoices ? TEXT("_Choices")
					: (bShowInventoryPage ? TEXT("_Inventory")
						: (bShowMercenaryPanel ? TEXT("_Mercenaries") : TEXT("")))));
		CaptureSuffix += AdditionalSuffix;
		const FString OutputPath = FPaths::Combine(OutputDirectory(),
			FString::Printf(TEXT("%s%s.png"), *Stem, *CaptureSuffix));

		TArray64<uint8> PngData;
		FImageUtils::PNGCompressImageArray(OutputWidth, OutputHeight, Pixels, PngData);
		IFileManager::Get().MakeDirectory(*FPaths::GetPath(OutputPath), true);
		if (!FFileHelper::SaveArrayToFile(PngData, *OutputPath))
		{
			OutError = FString::Printf(TEXT("파일을 쓰지 못함: %s"), *OutputPath);
			return false;
		}
		UE_LOG(LogTemp, Display, TEXT("[CombatLayout] captured %s"), *OutputPath);
		return true;
	}
}

namespace CombatLayoutCapture
{
	/**
	 * @brief 요소 하나만 남기고 전부 접은 뒤 그 자리를 잘라 저장한다.
	 *
	 * @details
	 * 전체 화면을 찍어 잘라내는 것과 다르다. 잘라내면 이웃 부품과 뒤판이 같이
	 * 들어와 그 요소가 실제로 어떻게 생겼는지 안 보인다. 여기서는 대상과 그
	 * 조상만 남기고 나머지를 접은 뒤 그린다 -- 요소만 홀로 남는다.
	 *
	 * 조상을 남기는 이유는 부모를 접으면 자식도 같이 사라지기 때문이다.
	 * 자리는 캐시된 기하에서 읽는다. 이 키트는 거의 전부 캔버스 패널이라
	 * 형제를 접어도 절대 좌표가 흔들리지 않는다.
	 */
	bool CaptureElements(UWorld& World, const TCHAR* ClassPath, FString& OutError)
	{
		UClass* LayoutClass = LoadClass<UUserWidget>(nullptr, ClassPath);
		if (LayoutClass == nullptr)
		{
			OutError = FString::Printf(TEXT("클래스를 못 찾음: %s"), ClassPath);
			return false;
		}
		// 공통 부모로 만든다. 캡처는 그려서 찍는 일이라 화면이 전투 배치안인지
		// 고용 게시판인지 알 필요가 없다.
		UUserWidget* Layout = CreateWidget<UUserWidget>(&World, LayoutClass);
		if (Layout == nullptr || Layout->WidgetTree == nullptr)
		{
			OutError = TEXT("위젯 생성 실패");
			return false;
		}
		Layout->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		const TSharedRef<SWidget> LayoutSlate = Layout->TakeWidget();
		if (UCombatLayoutHUDWidget* CombatHUD = Cast<UCombatLayoutHUDWidget>(Layout))
		{
			CombatHUD->ApplyActionLabelOpticalAlignmentForCapture();
		}
		Layout->ForceLayoutPrepass();
		ResidentBrushTextures(*Layout);

		TArray<UWidget*> All;
		Layout->WidgetTree->GetAllWidgets(All);

		FString Stem = FString(ClassPath);
		Stem.Split(TEXT("."), nullptr, &Stem, ESearchCase::CaseSensitive,
			ESearchDir::FromEnd);
		Stem.RemoveFromEnd(TEXT("_C"));
		const FString Dir = FPaths::Combine(OutputDirectory(), TEXT("Elements"), Stem);
		IFileManager::Get().DeleteDirectory(*Dir, false, true);
		IFileManager::Get().MakeDirectory(*Dir, true);

		// 한 번 그려서 기하를 채운다. 그리기 전에는 캐시가 비어 있다.
		FWidgetRenderer Probe(true, true);
		Probe.SetIsPrepassNeeded(true);
		const TSharedRef<SWidget> Scaled = ScaleToCapture(LayoutSlate);
		Probe.DrawWidget(Scaled, FVector2D(CaptureWidth, CaptureHeight));
		FlushRenderingCommands();

		TMap<UWidget*, ESlateVisibility> Original;
		for (UWidget* Widget : All)
		{
			Original.Add(Widget, Widget->GetVisibility());
		}

		int32 Saved = 0;
		for (UWidget* Target : All)
		{
			if (Target == nullptr || Target->GetName().StartsWith(TEXT("__")))
			{
				continue;
			}
			// 크기는 반드시 화면 크기로 읽는다. GetLocalSize 는 설계 좌표
			// (1920 캔버스) 값이라, 0.871 로 줄여 그린 화면 위치와 섞이면
			// 자를 상자가 실제보다 15% 커진다 -- 처음에 그렇게 나왔다.
			const FGeometry Geometry = Target->GetCachedGeometry();
			const FVector2D Size = FVector2D(Geometry.GetAbsoluteSize());
			const FVector2D Pos = FVector2D(Geometry.GetAbsolutePosition());
			if (Size.X < 8.0 || Size.Y < 8.0)
			{
				continue;
			}

			// 대상의 조상 사슬을 모은다. 부모를 접으면 대상도 사라진다.
			TSet<UWidget*> Keep;
			for (UWidget* Walk = Target; Walk != nullptr; Walk = Walk->GetParent())
			{
				Keep.Add(Walk);
			}
			for (UWidget* Widget : All)
			{
				const bool bUnder = Widget->IsChildOf(Target) || Keep.Contains(Widget);
				Widget->SetVisibility(bUnder
					? Original[Widget]
					: ESlateVisibility::Hidden);
			}

			FWidgetRenderer Renderer(true, true);
			Renderer.SetIsPrepassNeeded(true);
			UTextureRenderTarget2D* RT = Renderer.DrawWidget(
				Scaled, FVector2D(CaptureWidth, CaptureHeight));
			if (RT == nullptr)
			{
				continue;
			}
			FlushRenderingCommands();
			TArray<FColor> Pixels;
			FReadSurfaceDataFlags ReadFlags(RCM_UNorm);
			ReadFlags.SetLinearToGamma(false);
			if (!RT->GameThread_GetRenderTargetResource()->ReadPixels(Pixels, ReadFlags))
			{
				continue;
			}
			// 전체 캡처와 같은 역보정. 렌더러가 두 번 인코딩하므로 한 번 되돌린다.
			for (FColor& Pixel : Pixels)
			{
				Pixel.R = uint8(FMath::RoundToInt(255.f * FMath::Pow(Pixel.R / 255.f, 2.2f)));
				Pixel.G = uint8(FMath::RoundToInt(255.f * FMath::Pow(Pixel.G / 255.f, 2.2f)));
				Pixel.B = uint8(FMath::RoundToInt(255.f * FMath::Pow(Pixel.B / 255.f, 2.2f)));
			}

			// 잘라내지 않는다.
			//
			// 부품만 오려 내면 그게 화면 어디에 앉는지가 사라진다 -- 정작
			// 알고 싶은 게 그것이다. 전체 화면을 그대로 두고 그 부품만 켠
			// 그림을 남긴다. 시안 위에 그대로 겹칠 수 있고, 여러 장을 넘겨
			// 보면 부품들이 제자리에 있는지 한눈에 읽힌다.
			TArray64<uint8> Png;
			FImageUtils::PNGCompressImageArray(CaptureWidth, CaptureHeight, Pixels, Png);
			const FString File = FPaths::Combine(Dir, FString::Printf(
				TEXT("%s_%s_%dx%d_at%d_%d.png"),
				*Target->GetClass()->GetName(), *Target->GetName(),
				int32(Size.X), int32(Size.Y), int32(Pos.X), int32(Pos.Y)));
			if (FFileHelper::SaveArrayToFile(Png, *File))
			{
				++Saved;
			}
		}

		for (const TPair<UWidget*, ESlateVisibility>& Pair : Original)
		{
			Pair.Key->SetVisibility(Pair.Value);
		}
		UE_LOG(LogTemp, Display, TEXT("[CombatLayout] %s 요소 %d장 -> %s"),
			*Stem, Saved, *Dir);
		return Saved > 0;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCombatLayoutElementCaptureTest,
	"P_RD.UI.CombatLayout.CaptureElements",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatLayoutElementCaptureTest::RunTest(const FString& Parameters)
{
	using namespace CombatLayoutCapture;

	// NullRHI 환경에서는 오프스크린 렌더가 불가능하므로 캡처 생략
	if (GUsingNullRHI == true)
	{
		AddInfo(TEXT("NullRHI 환경이라 캡처 생략"));
		return true;
	}

	UWorld* World = GEditor != nullptr ? GEditor->GetEditorWorldContext().World() : nullptr;
	if (!TestNotNull(TEXT("에디터 월드가 있어야 위젯을 만들 수 있다"), World))
	{
		return false;
	}
	// 한 배치에 오백 장이 넘게 나온다.
	FString Error;
	if (!CaptureElements(*World, LayoutClassPaths[0], Error))
	{
		AddError(Error);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCombatLayoutCaptureTest,
	"P_RD.UI.CombatLayout.Capture",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatLayoutCaptureTest::RunTest(const FString& Parameters)
{
	using namespace CombatLayoutCapture;

	// NullRHI 환경에서는 오프스크린 렌더가 불가능하므로 캡처 생략
	if (GUsingNullRHI == true)
	{
		AddInfo(TEXT("NullRHI 환경이라 캡처 생략"));
		return true;
	}

	UWorld* World = GEditor != nullptr ? GEditor->GetEditorWorldContext().World() : nullptr;
	if (!TestNotNull(TEXT("에디터 월드가 있어야 위젯을 만들 수 있다"), World))
	{
		return false;
	}

	for (const TCHAR* ClassPath : LayoutClassPaths)
	{
		FString Error;
		if (!CaptureLayout(*World, ClassPath, Error))
		{
			AddError(FString::Printf(TEXT("%s: %s"), ClassPath, *Error));
		}
		// 고정 1672 시안 캡처만 보면 720p DPI에서 카드가 한 번 더 줄어드는
		// 회귀를 볼 수 없다. 실제 플레이 창과 같은 크기를 WBP에 직접 할당한다.
		Error.Reset();
		if (!CaptureLayout(*World, ClassPath, Error,
			false, false, false, false, false,
			1280, 720, true, TEXT("_1280x720")))
		{
			AddError(FString::Printf(TEXT("%s 1280x720: %s"), ClassPath, *Error));
		}
		// 실제 720p PIE는 1920x1080 저작 위젯을 0.667 DPI로 표현한다.
		// 직접 720 좌표를 할당하는 위 캡처와 별도로 그 경로도 남긴다.
		Error.Reset();
		if (!CaptureLayout(*World, ClassPath, Error,
			false, false, false, false, false,
			1280, 720, false, TEXT("_PIE1280x720")))
		{
			AddError(FString::Printf(TEXT("%s PIE 1280x720: %s"), ClassPath, *Error));
		}
		Error.Reset();
		if (!CaptureLayout(*World, ClassPath, Error,
			false, false, false, false, false,
			1280, 720, false, TEXT("_Targeting")))
		{
			AddError(FString::Printf(TEXT("%s 조준 조작부: %s"), ClassPath, *Error));
		}
		Error.Reset();
		if (!CaptureLayout(*World, ClassPath, Error,
			false, false, false, false, false,
			1280, 720, false, TEXT("_WaitingConfirm")))
		{
			AddError(FString::Printf(TEXT("%s 확정 대기 조작부: %s"), ClassPath, *Error));
		}
		Error.Reset();
		if (!CaptureLayout(*World, ClassPath, Error,
			false, false, false, false, false,
			1280, 720, false, TEXT("_Commands")))
		{
			AddError(FString::Printf(TEXT("%s 스킬 카드: %s"), ClassPath, *Error));
		}
		Error.Reset();
		if (!CaptureLayout(*World, ClassPath, Error,
			false, false, false, false, false,
			CaptureWidth, CaptureHeight, false, TEXT("_NoStatus")))
		{
			AddError(FString::Printf(TEXT("%s 상태 없는 요약판: %s"),
				ClassPath, *Error));
		}
		Error.Reset();
		if (!CaptureLayout(*World, ClassPath, Error, true))
		{
			AddError(FString::Printf(TEXT("%s 용병 탭: %s"), ClassPath, *Error));
		}
		Error.Reset();
		if (!CaptureLayout(*World, ClassPath, Error, true, true))
		{
			AddError(FString::Printf(TEXT("%s 인벤토리 탭: %s"), ClassPath, *Error));
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRewardSettlementV3CaptureTest,
	"P_RD.UI.ResultBoards.CaptureV3",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRewardSettlementV3CaptureTest::RunTest(const FString& Parameters)
{
	using namespace CombatLayoutCapture;
	if (GUsingNullRHI)
	{
		AddInfo(TEXT("NullRHI 환경이라 V3 보상 캡처 생략"));
		return true;
	}
	UWorld* World = GEditor != nullptr ? GEditor->GetEditorWorldContext().World() : nullptr;
	if (!TestNotNull(TEXT("V3 보상 캡처 월드"), World))
	{
		return false;
	}
	FString Error;
	if (!CaptureLayout(*World,
		TEXT("/Game/UI/RewardSettlement/WBP_RewardSettlement_V3.WBP_RewardSettlement_V3_C"),
		Error, false, false, false, false, true))
	{
		AddError(Error);
		return false;
	}
	Error.Reset();
	if (!CaptureLayout(*World,
		TEXT("/Game/UI/RewardSettlement/WBP_RewardSettlement_V3.WBP_RewardSettlement_V3_C"),
		Error))
	{
		AddError(Error);
		return false;
	}
	if (!CaptureLayout(*World,
		TEXT("/Game/UI/RewardSettlement/WBP_RewardSettlement_V3.WBP_RewardSettlement_V3_C"),
		Error, false, false, true))
	{
		AddError(Error);
		return false;
	}
	Error.Reset();
	if (!CaptureLayout(*World,
		TEXT("/Game/UI/RewardSettlement/WBP_RewardSettlement_V3.WBP_RewardSettlement_V3_C"),
		Error, false, false, true, true))
	{
		AddError(Error);
		return false;
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRewardConceptBoardsCaptureTest,
	"P_RD.UI.ResultBoards.CaptureConcepts",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRewardConceptBoardsCaptureTest::RunTest(const FString& Parameters)
{
	using namespace CombatLayoutCapture;
	if (GUsingNullRHI)
	{
		AddInfo(TEXT("NullRHI 환경이라 시안 보드 캡처 생략"));
		return true;
	}
	UWorld* World = GEditor != nullptr ? GEditor->GetEditorWorldContext().World() : nullptr;
	if (!TestNotNull(TEXT("시안 보드 캡처 월드"), World))
	{
		return false;
	}
	for (const TCHAR* Concept : { TEXT("02"), TEXT("03"), TEXT("06") })
	{
		for (const TCHAR* Stage :
			{ TEXT("Experience"), TEXT("Chest"), TEXT("Gold"), TEXT("Artifact") })
		{
			const FString Path = FString::Printf(
				TEXT("/Game/UI/RewardSettlement/Concepts/WBP_RC%s_%s.WBP_RC%s_%s_C"),
				Concept, Stage, Concept, Stage);
			FString Error;
			if (!CaptureLayout(*World, *Path, Error))
			{
				AddError(FString::Printf(TEXT("%s: %s"), *Path, *Error));
			}
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRewardC03BoardsCaptureTest,
	"P_RD.UI.ResultBoards.CaptureC03",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRewardC03BoardsCaptureTest::RunTest(const FString& Parameters)
{
	using namespace CombatLayoutCapture;
	if (GUsingNullRHI)
	{
		AddInfo(TEXT("NullRHI 환경이라 C03 보드 캡처 생략"));
		return true;
	}
	UWorld* World = GEditor != nullptr ? GEditor->GetEditorWorldContext().World() : nullptr;
	if (!TestNotNull(TEXT("C03 보드 캡처 월드"), World))
	{
		return false;
	}
	// 갓 임포트된 텍스처는 비동기 컴파일이 끝나기 전까지 빈 자리로 그려진다.
	// WaitForStreaming은 이를 커버하지 못하므로 컴파일 완료를 명시적으로 기다린다.
	FTextureCompilingManager::Get().FinishAllCompilation();
	for (const TCHAR* Stage :
		{ TEXT("Experience"), TEXT("Chest"), TEXT("Gold"), TEXT("Artifact") })
	{
		const FString Path = FString::Printf(
			TEXT("/Game/UI/RewardSettlement/Concepts/WBP_C03_%s.WBP_C03_%s_C"),
			Stage, Stage);
		FString Error;
		if (!CaptureLayout(*World, *Path, Error))
		{
			AddError(FString::Printf(TEXT("%s: %s"), *Path, *Error));
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRewardBSBoardsCaptureTest,
	"P_RD.UI.ResultBoards.CaptureBS",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRewardBSBoardsCaptureTest::RunTest(const FString& Parameters)
{
	using namespace CombatLayoutCapture;
	if (GUsingNullRHI)
	{
		AddInfo(TEXT("NullRHI 환경이라 BS 보드 캡처 생략"));
		return true;
	}
	UWorld* World = GEditor != nullptr ? GEditor->GetEditorWorldContext().World() : nullptr;
	if (!TestNotNull(TEXT("BS 보드 캡처 월드"), World))
	{
		return false;
	}
	FTextureCompilingManager::Get().FinishAllCompilation();
	for (const TCHAR* Stage :
		{ TEXT("Experience"), TEXT("Chest"), TEXT("Gold"), TEXT("Artifact") })
	{
		const FString Path = FString::Printf(
			TEXT("/Game/UI/RewardSettlement/Concepts/WBP_BS_%s.WBP_BS_%s_C"),
			Stage, Stage);
		FString Error;
		if (!CaptureLayout(*World, *Path, Error,
			false, false, false, false, false, 1536, 864))
		{
			AddError(FString::Printf(TEXT("%s: %s"), *Path, *Error));
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMercenaryHireLayoutCaptureTest,
	"P_RD.UI.MercenaryHireMarchbound.Capture",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMercenaryHireLayoutCaptureTest::RunTest(const FString& Parameters)
{
	using namespace CombatLayoutCapture;

	if (GUsingNullRHI == true)
	{
		AddInfo(TEXT("NullRHI 환경이라 캡처 생략"));
		return true;
	}

	UWorld* World = GEditor != nullptr ? GEditor->GetEditorWorldContext().World() : nullptr;
	if (!TestNotNull(TEXT("에디터 월드가 있어야 용병 선택 WBP를 만들 수 있다"), World))
	{
		return false;
	}

	FString Error;
	if (!CaptureLayout(*World,
		TEXT("/Game/UI/CombatLayouts/WBP_MercenaryHire_Marchbound.WBP_MercenaryHire_Marchbound_C"),
		Error))
	{
		AddError(Error);
	}

	FString FoldError;
	if (!CaptureLayout(*World,
		TEXT("/Game/UI/CombatLayouts/WBP_MercenaryHire_Marchbound.WBP_MercenaryHire_Marchbound_C"),
		FoldError, false, false, false, false, false,
		2176, 1812, true, TEXT("_Fold2176x1812")))
	{
		AddError(FoldError);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMonsterTabLayoutCaptureTest,
	"P_RD.UI.MonsterTab.Capture",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMonsterTabLayoutCaptureTest::RunTest(const FString& Parameters)
{
	using namespace CombatLayoutCapture;

	if (GUsingNullRHI == true)
	{
		AddInfo(TEXT("NullRHI 환경이라 몬스터 탭 캡처 생략"));
		return true;
	}

	UWorld* World = GEditor != nullptr
		? GEditor->GetEditorWorldContext().World() : nullptr;
	if (!TestNotNull(TEXT("에디터 월드가 있어야 몬스터 탭 WBP를 만들 수 있다"), World))
	{
		return false;
	}

	FString Error;
	if (!CaptureLayout(*World,
		TEXT("/Game/UI/MonsterTab/WBP_MonsterTab_Marchbound.WBP_MonsterTab_Marchbound_C"),
		Error))
	{
		AddError(Error);
	}
	else
	{
		const FString CapturePath = FPaths::Combine(OutputDirectory(),
			TEXT("WBP_MonsterTab_Marchbound.png"));
		TestTrue(TEXT("현대식 몬스터 탭 PNG가 생성됨"),
			IFileManager::Get().FileExists(*CapturePath));
		TestTrue(TEXT("몬스터 탭 캡처가 빈 파일이 아님"),
			IFileManager::Get().FileSize(*CapturePath) > 100000);
	}
	return true;
}

/**
 * @brief 틱 없이 한 프레임만 그려도 스킬 카드가 제 모습으로 나오는지 본다.
 *
 * @details 카드 등장 연출(0824)은 시작 상태가 "투명 + 0.86배"다. 편집기
 * 캡처는 한 프레임만 그리고 끝나 연출이 앞으로 가지 않으므로, 연출을
 * 아무 데서나 걸면 그 시작 상태로 굳어 캡처에서 카드가 통째로 사라진다 --
 * 실제로 한 번 그렇게 나왔다. 연출은 게임 월드에서만 걸어야 한다.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCombatLayoutCardTransformTest,
	"P_RD.UI.CombatLayout.CardsSurviveSingleFrame",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatLayoutCardTransformTest::RunTest(const FString& Parameters)
{
	UWorld* World = GEditor != nullptr
		? GEditor->GetEditorWorldContext().World() : nullptr;
	UClass* LayoutClass = LoadClass<UUserWidget>(nullptr,
		TEXT("/Game/UI/CombatLayouts/WBP_CombatHUD04.WBP_CombatHUD04_C"));
	if (!TestNotNull(TEXT("에디터 월드"), World)
		|| !TestNotNull(TEXT("전투 HUD 클래스"), LayoutClass))
	{
		return false;
	}
	UUserWidget* Layout = CreateWidget<UUserWidget>(World, LayoutClass);
	if (!TestNotNull(TEXT("전투 HUD 인스턴스"), Layout))
	{
		return false;
	}
	const TSharedRef<SWidget> LayoutSlate = Layout->TakeWidget();

	int32 AuditedCards = 0;
	for (int32 Index = 0; Index < 6; ++Index)
	{
		UWidget* Card = Layout->GetWidgetFromName(
			FName(*FString::Printf(TEXT("CommandCard_%d"), Index)));
		if (Card == nullptr)
		{
			continue;
		}
		++AuditedCards;
		TestEqual(*FString::Printf(TEXT("카드 %d 불투명"), Index),
			Card->GetRenderOpacity(), 1.f);
		TestEqual(*FString::Printf(TEXT("카드 %d 원본 배율"), Index),
			Card->GetRenderTransform().Scale, FVector2D(1.f, 1.f));
	}
	TestTrue(TEXT("카드를 실제로 검사함"), AuditedCards > 0);
	return !HasAnyErrors();
}

#endif // WITH_EDITOR
