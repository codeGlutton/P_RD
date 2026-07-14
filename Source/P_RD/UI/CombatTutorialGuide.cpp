#include "UI/CombatTutorialGuide.h"

#include "UI/CombatTileMapHUDWidget.h"

#include "Actor/TileMap/TileMapModel.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/TextBlock.h"
#include "GameMode/RDGameModeBase.h"
#include "PCGStage/Room.h"
#include "Setting/GamePlaySettings.h"
#include "Singleton/InstanceSubsystem/PersistentData.h"
#include "Singleton/InstanceSubsystem/SaveGameSubsystem.h"
#include "Singleton/WorldSubsystem/SRPGCombatModel.h"
#include "Styling/SlateTypes.h"
#include "UI/UIRuntimeLayout.h"

namespace
{
	constexpr int32 ScrimZOrder = 2000;
	constexpr int32 OutlineZOrder = 2010;
	constexpr int32 InputBlockerZOrder = 2020;
	constexpr int32 PanelZOrder = 2030;
	constexpr int32 SkipZOrder = 2040;
	constexpr float OutlineThickness = 6.0f;
	constexpr float TargetPadding = 18.0f;
	constexpr float MissingTargetGraceSeconds = 4.0f;

	void ConfigureTutorialInvisibleButton(UButton* Button)
	{
		if (Button == nullptr)
		{
			return;
		}

		FSlateBrush EmptyBrush;
		EmptyBrush.DrawAs = ESlateBrushDrawType::NoDrawType;
		FButtonStyle Style;
		Style.SetNormal(EmptyBrush);
		Style.SetHovered(EmptyBrush);
		Style.SetPressed(EmptyBrush);
		Style.SetDisabled(EmptyBrush);
		Button->SetStyle(Style);
		Button->SetBackgroundColor(FLinearColor::Transparent);
	}

	FText GetInstruction(ECombatTutorialGuideStep Step)
	{
		switch (Step)
		{
		case ECombatTutorialGuideStep::RollDiceBoard: return NSLOCTEXT("CombatTutorial", "RollDiceBoard", "1 / 17   주사위 판을 눌러 주사위를 굴리세요");
		case ECombatTutorialGuideStep::WaitDiceResult: return NSLOCTEXT("CombatTutorial", "WaitDiceResult", "주사위가 멈출 때까지 기다리세요");
		case ECombatTutorialGuideStep::DismissDiceBoard: return NSLOCTEXT("CombatTutorial", "DismissDiceBoard", "2 / 17   결과를 확인하고 주사위 판을 한 번 더 누르세요");
		case ECombatTutorialGuideStep::SelectStepSkill: return NSLOCTEXT("CombatTutorial", "SelectStepSkill", "3 / 17   아래쪽 STEP 스킬을 누르세요");
		case ECombatTutorialGuideStep::SelectStepDice: return NSLOCTEXT("CombatTutorial", "SelectStepDice", "4 / 17   금색 박스의 주사위를 STEP에 올리세요");
		case ECombatTutorialGuideStep::PreviewStep: return NSLOCTEXT("CombatTutorial", "PreviewStep", "5 / 17   내 캐릭터가 서 있는 칸을 누르세요");
		case ECombatTutorialGuideStep::ConfirmStep: return NSLOCTEXT("CombatTutorial", "ConfirmStep", "6 / 17   같은 칸을 한 번 더 눌러 STEP을 확정하세요");
		case ECombatTutorialGuideStep::WaitStepResolved: return NSLOCTEXT("CombatTutorial", "WaitStepResolved", "STEP으로 이동력을 얻는 중입니다");
		case ECombatTutorialGuideStep::PressMove: return NSLOCTEXT("CombatTutorial", "PressMove", "7 / 17   오른쪽 MOVE 버튼을 누르세요");
		case ECombatTutorialGuideStep::PreviewMove: return NSLOCTEXT("CombatTutorial", "PreviewMove", "8 / 17   금색 박스의 칸을 눌러 이동 경로를 확인하세요");
		case ECombatTutorialGuideStep::ConfirmMove: return NSLOCTEXT("CombatTutorial", "ConfirmMove", "9 / 17   같은 칸을 한 번 더 눌러 이동하세요");
		case ECombatTutorialGuideStep::WaitMoveResolved: return NSLOCTEXT("CombatTutorial", "WaitMoveResolved", "공격할 위치로 이동 중입니다");
		case ECombatTutorialGuideStep::SelectAttackSkill: return NSLOCTEXT("CombatTutorial", "SelectAttackSkill", "10 / 17   위쪽 기본 공격 스킬을 누르세요");
		case ECombatTutorialGuideStep::SelectAttackDice: return NSLOCTEXT("CombatTutorial", "SelectAttackDice", "11 / 17   남은 주사위 하나를 공격 스킬에 올리세요");
		case ECombatTutorialGuideStep::PreviewAttack: return NSLOCTEXT("CombatTutorial", "PreviewAttack", "12 / 17   적을 눌러 예상 피해를 확인하세요");
		case ECombatTutorialGuideStep::ConfirmAttack: return NSLOCTEXT("CombatTutorial", "ConfirmAttack", "13 / 17   같은 적을 한 번 더 눌러 공격을 확정하세요");
		case ECombatTutorialGuideStep::WaitAttackResolved: return NSLOCTEXT("CombatTutorial", "WaitAttackResolved", "공격 결과를 확인하는 중입니다");
		case ECombatTutorialGuideStep::SelectBuffSkill: return NSLOCTEXT("CombatTutorial", "SelectBuffSkill", "14 / 17   금색 박스의 방어 스킬을 누르세요");
		case ECombatTutorialGuideStep::SelectBuffDice: return NSLOCTEXT("CombatTutorial", "SelectBuffDice", "15 / 17   남은 주사위 하나를 방어 스킬에 올리세요");
		case ECombatTutorialGuideStep::PreviewBuff: return NSLOCTEXT("CombatTutorial", "PreviewBuff", "16 / 17   내 캐릭터가 서 있는 칸을 누르세요");
		case ECombatTutorialGuideStep::ConfirmBuff: return NSLOCTEXT("CombatTutorial", "ConfirmBuff", "17 / 17   같은 칸을 한 번 더 눌러 방어 스킬을 확정하세요");
		case ECombatTutorialGuideStep::WaitBuffResolved: return NSLOCTEXT("CombatTutorial", "WaitBuffResolved", "방어력이 증가하는 것을 확인하세요");
		default: return FText::GetEmpty();
		}
	}
}

void UCombatTutorialGuide::Initialize(UCombatTileMapHUDWidget* InOwner)
{
	mOwner = InOwner;
	UWidgetTree* WidgetTree = mOwner != nullptr ? mOwner->GetTutorialWidgetTree() : nullptr;
	UCanvasPanel* RootCanvas = mOwner != nullptr ? mOwner->GetTutorialOverlayCanvas() : nullptr;
	if (WidgetTree == nullptr || RootCanvas == nullptr)
	{
		return;
	}

	for (int32 Index = 0; Index < 4; ++Index)
	{
		UBorder* Scrim = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), FName(*FString::Printf(TEXT("TutorialScrim_%d"), Index)));
		UButton* Blocker = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), FName(*FString::Printf(TEXT("TutorialBlocker_%d"), Index)));
		UBorder* Edge = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), FName(*FString::Printf(TEXT("TutorialOutline_%d"), Index)));
		if (Scrim != nullptr)
		{
			Scrim->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.62f));
			Scrim->SetVisibility(ESlateVisibility::Collapsed);
			RootCanvas->AddChildToCanvas(Scrim);
			mScrims.Add(Scrim);
		}
		if (Blocker != nullptr)
		{
			ConfigureTutorialInvisibleButton(Blocker);
			Blocker->SetVisibility(ESlateVisibility::Collapsed);
			RootCanvas->AddChildToCanvas(Blocker);
			mInputBlockers.Add(Blocker);
		}
		if (Edge != nullptr)
		{
			Edge->SetBrushColor(FLinearColor(1.0f, 0.72f, 0.12f, 1.0f));
			Edge->SetVisibility(ESlateVisibility::Collapsed);
			RootCanvas->AddChildToCanvas(Edge);
			mOutlineEdges.Add(Edge);
		}
	}

	mGuidePanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("TutorialGuidePanel"));
	mGuideText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TutorialGuideText"));
	mSkipButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("TutorialSkipButton"));
	UTextBlock* SkipText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TutorialSkipText"));
	if (mGuidePanel == nullptr || mGuideText == nullptr || mSkipButton == nullptr || SkipText == nullptr)
	{
		return;
	}

	RootCanvas->AddChildToCanvas(mGuidePanel);
	RootCanvas->AddChildToCanvas(mSkipButton);
	mGuidePanel->SetBrushColor(FLinearColor(0.015f, 0.025f, 0.055f, 0.96f));
	mGuidePanel->SetPadding(FMargin(28.0f, 18.0f));
	mGuidePanel->SetHorizontalAlignment(HAlign_Fill);
	mGuidePanel->SetVerticalAlignment(VAlign_Fill);
	mGuidePanel->AddChild(mGuideText);
	mGuidePanel->SetVisibility(ESlateVisibility::Collapsed);
	mGuideText->SetJustification(ETextJustify::Center);
	mGuideText->SetAutoWrapText(true);
	mGuideText->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.93f, 0.72f, 1.0f)));
	FSlateFontInfo GuideFont = mGuideText->GetFont();
	GuideFont.Size = 28;
	mGuideText->SetFont(GuideFont);

	SkipText->SetText(NSLOCTEXT("CombatTutorial", "Skip", "건너뛰기"));
	SkipText->SetJustification(ETextJustify::Center);
	SkipText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	FSlateFontInfo SkipFont = SkipText->GetFont();
	SkipFont.Size = 20;
	SkipText->SetFont(SkipFont);
	mSkipButton->SetBackgroundColor(FLinearColor(0.12f, 0.14f, 0.18f, 0.95f));
	mSkipButton->AddChild(SkipText);
	mSkipButton->OnClicked.AddUniqueDynamic(this, &UCombatTutorialGuide::HandleSkipClicked);
	mSkipButton->SetVisibility(ESlateVisibility::Collapsed);

	BindCombatUIModel(nullptr);
	SetVisible(false);
}

void UCombatTutorialGuide::BindCombatUIModel(UCombatUIModel* InCombatUIModel)
{
	if (mCombatUIModel != nullptr)
	{
		mCombatUIModel->OnUIChanged.RemoveDynamic(this, &UCombatTutorialGuide::HandleCombatUIChanged);
		mCombatUIModel->OnActionResolved.RemoveDynamic(this, &UCombatTutorialGuide::HandleActionResolved);
		mCombatUIModel->OnCombatCommand.RemoveDynamic(this, &UCombatTutorialGuide::HandleCombatCommand);
	}
	mCombatUIModel = InCombatUIModel;
	if (mCombatUIModel != nullptr)
	{
		mCombatUIModel->OnUIChanged.AddUniqueDynamic(this, &UCombatTutorialGuide::HandleCombatUIChanged);
		mCombatUIModel->OnActionResolved.AddUniqueDynamic(this, &UCombatTutorialGuide::HandleActionResolved);
		mCombatUIModel->OnCombatCommand.AddUniqueDynamic(this, &UCombatTutorialGuide::HandleCombatCommand);
	}
}

void UCombatTutorialGuide::BeginDestroy()
{
	BindCombatUIModel(nullptr);
	if (mSkipButton != nullptr)
	{
		mSkipButton->OnClicked.RemoveDynamic(this, &UCombatTutorialGuide::HandleSkipClicked);
	}
	Super::BeginDestroy();
}

bool UCombatTutorialGuide::ShouldShow() const
{
	const UWorld* World = mOwner != nullptr ? mOwner->GetWorld() : nullptr;
	const ARDGameModeBase* GameMode = World != nullptr ? World->GetAuthGameMode<ARDGameModeBase>() : nullptr;
	const URunPersistData* RunData = GameMode != nullptr ? GameMode->GetRunPersistData() : nullptr;
	const UUserPersistData* UserData = GameMode != nullptr ? GameMode->GetUserPersistData() : nullptr;
	if (RunData == nullptr || RunData->IsActive() == false || RunData->ShouldRunTutorial() == false
		|| (UserData != nullptr && UserData->IsTutorialCompleted()))
	{
		return false;
	}

	const FPrimaryAssetId TutorialRoomId = GetDefault<UGamePlaySettings>()->mTutorialRoomId;
	return TutorialRoomId.IsValid() && RunData->GetCurrentRoom().mStaticRoomSpawnDataId == TutorialRoomId;
}

void UCombatTutorialGuide::BeginTutorial()
{
	if (ShouldShow() == false || mStep != ECombatTutorialGuideStep::None || mCombatUIModel == nullptr)
	{
		return;
	}

	const UGamePlaySettings* Settings = GetDefault<UGamePlaySettings>();
	mAttackSkillId = Settings->mTutorialAttackSkillId;
	mStepSkillId = Settings->mTutorialStepSkillId;
	mDefenseSkillId = Settings->mTutorialDefenseSkillId;
	mMoveOriginTile = FTileIndex::Invalid;
	mMoveTargetTile = FTileIndex::Invalid;
	mEnemyUnitId = INDEX_NONE;
	mMissingTargetElapsed = 0.0f;
	for (const FUnitUI& Unit : mCombatUIModel->GetUnitUIs())
	{
		if (Unit.mIsPlayer == false)
		{
			mEnemyUnitId = Unit.mUnitId;
			break;
		}
	}

	if (mAttackSkillId.IsValid() == false || mStepSkillId.IsValid() == false || mDefenseSkillId.IsValid() == false)
	{
		FinishTutorial(true);
		return;
	}
	SetStep(ECombatTutorialGuideStep::RollDiceBoard);
}

bool UCombatTutorialGuide::IsActive() const
{
	return mStep != ECombatTutorialGuideStep::None && mStep != ECombatTutorialGuideStep::Complete;
}

void UCombatTutorialGuide::SetStep(ECombatTutorialGuideStep Step)
{
	if (mStep == Step)
	{
		return;
	}
	mStep = Step;
	mMissingTargetElapsed = 0.0f;
	SetVisible(IsActive());
	UpdateLayout();
}

void UCombatTutorialGuide::Tick(float InDeltaTime)
{
	if (mStep == ECombatTutorialGuideStep::None && mOwner != nullptr && mOwner->IsTutorialDiceRollReady())
	{
		BeginTutorial();
	}
	if (IsActive() == false)
	{
		return;
	}

	RefreshProgress();
	UpdateLayout();

	FVector2D TopLeft;
	FVector2D BottomRight;
	if (GetTargetRect(TopLeft, BottomRight))
	{
		mMissingTargetElapsed = 0.0f;
	}
	else
	{
		mMissingTargetElapsed += InDeltaTime;
		if (mMissingTargetElapsed >= MissingTargetGraceSeconds)
		{
			UE_LOG(LogTemp, Warning, TEXT("Combat tutorial skipped: target unavailable at step %d"), static_cast<int32>(mStep));
			FinishTutorial(true);
		}
	}
}

void UCombatTutorialGuide::HandleCombatUIChanged(ECombatUIDomain Domain)
{
	RefreshProgress();
}

void UCombatTutorialGuide::HandleActionResolved()
{
	RefreshProgress();
}

void UCombatTutorialGuide::HandleCombatCommand(ECombatInputType Type, int32 IntPayload)
{
	if (mStep == ECombatTutorialGuideStep::RollDiceBoard && Type == ECombatInputType::RollDice)
	{
		SetStep(ECombatTutorialGuideStep::WaitDiceResult);
	}
	else if (mStep == ECombatTutorialGuideStep::PressMove && Type == ECombatInputType::Move)
	{
		SetStep(ECombatTutorialGuideStep::PreviewMove);
	}
	RefreshProgress();
}

FPrimaryAssetId UCombatTutorialGuide::GetSelectedSkillId() const
{
	if (mCombatUIModel == nullptr)
	{
		return FPrimaryAssetId();
	}
	const int32 SkillIndex = mCombatUIModel->GetSelectedSkillIndex();
	const TArray<FSkillUI>& Skills = mCombatUIModel->GetSkillUIs();
	return Skills.IsValidIndex(SkillIndex) ? Skills[SkillIndex].mSkillId : FPrimaryAssetId();
}

bool UCombatTutorialGuide::HasSkill(const FPrimaryAssetId& SkillId) const
{
	return mCombatUIModel != nullptr && mCombatUIModel->GetSkillUIs().ContainsByPredicate([&SkillId](const FSkillUI& Skill)
	{
		return Skill.mSkillId == SkillId;
	});
}

void UCombatTutorialGuide::RefreshProgress()
{
	if (IsActive() == false || mCombatUIModel == nullptr || mOwner == nullptr)
	{
		return;
	}

	const FPrimaryAssetId SelectedSkillId = GetSelectedSkillId();
	const int32 SelectedDiceCount = mCombatUIModel->GetSelectedDiceIndices().Num();
	const ECombatBuildPhaseUI BuildPhase = mCombatUIModel->GetTurnUI().mPhase;
	const TArray<FUnitUI>& Units = mCombatUIModel->GetUnitUIs();
	const FUnitUI* Player = Units.FindByPredicate([](const FUnitUI& Unit) { return Unit.mIsPlayer; });
	const FUnitUI* Enemy = Units.FindByPredicate([this](const FUnitUI& Unit)
	{
		return Unit.mIsPlayer == false && (mEnemyUnitId == INDEX_NONE || Unit.mUnitId == mEnemyUnitId);
	});

	switch (mStep)
	{
	case ECombatTutorialGuideStep::RollDiceBoard:
		if (mOwner->IsTutorialDiceRollActive()) SetStep(ECombatTutorialGuideStep::WaitDiceResult);
		break;
	case ECombatTutorialGuideStep::WaitDiceResult:
		if (mOwner->IsTutorialDiceResultWaitingForDismiss()) SetStep(ECombatTutorialGuideStep::DismissDiceBoard);
		break;
	case ECombatTutorialGuideStep::DismissDiceBoard:
		if (mOwner->IsTutorialDiceRollReady() == false && mOwner->IsTutorialDiceRollActive() == false
			&& mOwner->IsTutorialDiceResultWaitingForDismiss() == false)
		{
			if (HasSkill(mStepSkillId) && HasSkill(mAttackSkillId) && HasSkill(mDefenseSkillId))
			{
				SetStep(ECombatTutorialGuideStep::SelectStepSkill);
			}
			else
			{
				FinishTutorial(true);
			}
		}
		break;
	case ECombatTutorialGuideStep::SelectStepSkill:
		if (SelectedSkillId == mStepSkillId) SetStep(ECombatTutorialGuideStep::SelectStepDice);
		break;
	case ECombatTutorialGuideStep::SelectStepDice:
		if (SelectedSkillId == mStepSkillId && SelectedDiceCount >= 1) SetStep(ECombatTutorialGuideStep::PreviewStep);
		break;
	case ECombatTutorialGuideStep::PreviewStep:
		if (BuildPhase == ECombatBuildPhaseUI::Preview) SetStep(ECombatTutorialGuideStep::ConfirmStep);
		break;
	case ECombatTutorialGuideStep::ConfirmStep:
		if (BuildPhase == ECombatBuildPhaseUI::None) SetStep(ECombatTutorialGuideStep::WaitStepResolved);
		break;
	case ECombatTutorialGuideStep::WaitStepResolved:
		if (Player != nullptr && Player->mMovementPoint > 0.0f)
		{
			mMoveOriginTile = Player->mTile;
			if (ResolveMoveTarget(mMoveTargetTile)) SetStep(ECombatTutorialGuideStep::PressMove);
		}
		break;
	case ECombatTutorialGuideStep::PreviewMove:
		if (BuildPhase == ECombatBuildPhaseUI::Preview) SetStep(ECombatTutorialGuideStep::ConfirmMove);
		break;
	case ECombatTutorialGuideStep::ConfirmMove:
		if (BuildPhase == ECombatBuildPhaseUI::None) SetStep(ECombatTutorialGuideStep::WaitMoveResolved);
		break;
	case ECombatTutorialGuideStep::WaitMoveResolved:
		if (Player != nullptr && Player->mTile != mMoveOriginTile)
		{
			if (Enemy != nullptr)
			{
				mEnemyHPBeforeAttack = Enemy->mHP;
				mEnemyDefenseBeforeAttack = Enemy->mDefensePoint;
			}
			SetStep(ECombatTutorialGuideStep::SelectAttackSkill);
		}
		break;
	case ECombatTutorialGuideStep::SelectAttackSkill:
		if (SelectedSkillId == mAttackSkillId) SetStep(ECombatTutorialGuideStep::SelectAttackDice);
		break;
	case ECombatTutorialGuideStep::SelectAttackDice:
		if (SelectedSkillId == mAttackSkillId && SelectedDiceCount >= 1) SetStep(ECombatTutorialGuideStep::PreviewAttack);
		break;
	case ECombatTutorialGuideStep::PreviewAttack:
		if (BuildPhase == ECombatBuildPhaseUI::Preview) SetStep(ECombatTutorialGuideStep::ConfirmAttack);
		break;
	case ECombatTutorialGuideStep::ConfirmAttack:
		if (BuildPhase == ECombatBuildPhaseUI::None) SetStep(ECombatTutorialGuideStep::WaitAttackResolved);
		break;
	case ECombatTutorialGuideStep::WaitAttackResolved:
		if (Enemy == nullptr || Enemy->mHP < mEnemyHPBeforeAttack || Enemy->mDefensePoint < mEnemyDefenseBeforeAttack)
		{
			if (Player != nullptr) mPlayerDefenseBeforeBuff = Player->mDefensePoint;
			SetStep(ECombatTutorialGuideStep::SelectBuffSkill);
		}
		break;
	case ECombatTutorialGuideStep::SelectBuffSkill:
		if (SelectedSkillId == mDefenseSkillId) SetStep(ECombatTutorialGuideStep::SelectBuffDice);
		break;
	case ECombatTutorialGuideStep::SelectBuffDice:
		if (SelectedSkillId == mDefenseSkillId && SelectedDiceCount >= 1) SetStep(ECombatTutorialGuideStep::PreviewBuff);
		break;
	case ECombatTutorialGuideStep::PreviewBuff:
		if (BuildPhase == ECombatBuildPhaseUI::Preview) SetStep(ECombatTutorialGuideStep::ConfirmBuff);
		break;
	case ECombatTutorialGuideStep::ConfirmBuff:
		if (BuildPhase == ECombatBuildPhaseUI::None) SetStep(ECombatTutorialGuideStep::WaitBuffResolved);
		break;
	case ECombatTutorialGuideStep::WaitBuffResolved:
		if (Player != nullptr && Player->mDefensePoint > mPlayerDefenseBeforeBuff) FinishTutorial(false);
		break;
	default:
		break;
	}
}

void UCombatTutorialGuide::HandleSkipClicked()
{
	FinishTutorial(true);
}

void UCombatTutorialGuide::FinishTutorial(bool bSkipped)
{
	if (mStep == ECombatTutorialGuideStep::Complete)
	{
		return;
	}

	UWorld* World = mOwner != nullptr ? mOwner->GetWorld() : nullptr;
	ARDGameModeBase* GameMode = World != nullptr ? World->GetAuthGameMode<ARDGameModeBase>() : nullptr;
	if (GameMode != nullptr)
	{
		if (UUserPersistData* UserData = GameMode->GetUserPersistData()) UserData->CompleteTutorial();
		if (URunPersistData* RunData = GameMode->GetRunPersistData()) RunData->SetTutorialEnabled(false);
	}
	if (World != nullptr && World->GetGameInstance() != nullptr)
	{
		if (USaveGameSubsystem* SaveSubsystem = World->GetGameInstance()->GetSubsystem<USaveGameSubsystem>())
		{
			SaveSubsystem->SaveUserAsync(FAsyncSaveGameToSlotDelegate());
			SaveSubsystem->SaveRunAsync(FAsyncSaveGameToSlotDelegate());
		}
	}
	UE_LOG(LogTemp, Log, TEXT("Combat tutorial %s"), bSkipped ? TEXT("skipped") : TEXT("completed"));
	SetStep(ECombatTutorialGuideStep::Complete);
}

void UCombatTutorialGuide::SetVisible(bool bVisible) const
{
	const ESlateVisibility VisualVisibility = bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed;
	const ESlateVisibility InputVisibility = bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed;
	if (mGuidePanel != nullptr) mGuidePanel->SetVisibility(VisualVisibility);
	if (mSkipButton != nullptr) mSkipButton->SetVisibility(InputVisibility);
	for (UButton* Blocker : mInputBlockers) if (Blocker != nullptr) Blocker->SetVisibility(InputVisibility);
	for (UBorder* Scrim : mScrims) if (Scrim != nullptr) Scrim->SetVisibility(VisualVisibility);
	for (UBorder* Edge : mOutlineEdges) if (Edge != nullptr) Edge->SetVisibility(VisualVisibility);
}

bool UCombatTutorialGuide::GetWidgetRect(const UWidget* Widget, FVector2D& OutTopLeft, FVector2D& OutBottomRight) const
{
	UCanvasPanel* RootCanvas = mOwner != nullptr ? mOwner->GetTutorialOverlayCanvas() : nullptr;
	if (Widget == nullptr || RootCanvas == nullptr)
	{
		return false;
	}
	const FGeometry& RootGeometry = RootCanvas->GetCachedGeometry();
	const FGeometry& WidgetGeometry = Widget->GetCachedGeometry();
	if (RootGeometry.GetLocalSize().IsNearlyZero() || WidgetGeometry.GetLocalSize().IsNearlyZero())
	{
		return false;
	}
	OutTopLeft = RootGeometry.AbsoluteToLocal(WidgetGeometry.LocalToAbsolute(FVector2D::ZeroVector));
	OutBottomRight = RootGeometry.AbsoluteToLocal(WidgetGeometry.LocalToAbsolute(WidgetGeometry.GetLocalSize()));
	return true;
}

bool UCombatTutorialGuide::GetUnitRect(bool bPlayer, FVector2D& OutTopLeft, FVector2D& OutBottomRight) const
{
	if (mCombatUIModel == nullptr || mOwner == nullptr)
	{
		return false;
	}
	APlayerController* PlayerController = mOwner->GetWorld() != nullptr ? mOwner->GetWorld()->GetFirstPlayerController() : nullptr;
	if (PlayerController == nullptr)
	{
		return false;
	}
	for (const FUnitUI& Unit : mCombatUIModel->GetUnitUIs())
	{
		if (Unit.mIsPlayer != bPlayer || (bPlayer == false && mEnemyUnitId != INDEX_NONE && Unit.mUnitId != mEnemyUnitId))
		{
			continue;
		}
		const FVector WorldLocation = Unit.mViewActor.IsValid() ? Unit.mViewActor->GetActorLocation() : Unit.mWorldLocation;
		FVector2D ScreenPosition;
		if (UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(PlayerController, WorldLocation, ScreenPosition, false) == false)
		{
			return false;
		}
		const FVector2D HalfSize(72.0f, 88.0f);
		OutTopLeft = ScreenPosition - HalfSize;
		OutBottomRight = ScreenPosition + HalfSize;
		return true;
	}
	return false;
}

bool UCombatTutorialGuide::GetTileRect(const FTileIndex& Tile, FVector2D& OutTopLeft, FVector2D& OutBottomRight) const
{
	if (mOwner == nullptr)
	{
		return false;
	}
	USRPGCombatModel* CombatModel = GetWorldSubsystemModel<USRPGCombatModel>(mOwner);
	UTileMapModel* TileMap = CombatModel != nullptr ? CombatModel->GetTileMap() : nullptr;
	APlayerController* PlayerController = mOwner->GetWorld() != nullptr ? mOwner->GetWorld()->GetFirstPlayerController() : nullptr;
	if (TileMap == nullptr || PlayerController == nullptr || TileMap->IsValidIndex(Tile) == false)
	{
		return false;
	}
	FVector2D ScreenPosition;
	if (UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(PlayerController, TileMap->TileToWorldLocation(Tile), ScreenPosition, false) == false)
	{
		return false;
	}
	const FVector2D HalfSize(68.0f, 48.0f);
	OutTopLeft = ScreenPosition - HalfSize;
	OutBottomRight = ScreenPosition + HalfSize;
	return true;
}

bool UCombatTutorialGuide::ResolveMoveTarget(FTileIndex& OutTarget) const
{
	if (mCombatUIModel == nullptr || mOwner == nullptr)
	{
		return false;
	}
	const TArray<FUnitUI>& Units = mCombatUIModel->GetUnitUIs();
	const FUnitUI* Player = Units.FindByPredicate([](const FUnitUI& Unit) { return Unit.mIsPlayer; });
	const FUnitUI* Enemy = Units.FindByPredicate([this](const FUnitUI& Unit)
	{
		return Unit.mIsPlayer == false && (mEnemyUnitId == INDEX_NONE || Unit.mUnitId == mEnemyUnitId);
	});
	USRPGCombatModel* CombatModel = GetWorldSubsystemModel<USRPGCombatModel>(mOwner);
	UTileMapModel* TileMap = CombatModel != nullptr ? CombatModel->GetTileMap() : nullptr;
	if (Player == nullptr || Enemy == nullptr || TileMap == nullptr)
	{
		return false;
	}

	const TArray<FTileIndex> Reachable = TileMap->GetReachableTiles(Player->mTile, FMath::FloorToInt(Player->mMovementPoint));
	const FTileIndex Candidates[] = {
		FTileIndex(Enemy->mTile.mX - 1, Enemy->mTile.mY), FTileIndex(Enemy->mTile.mX + 1, Enemy->mTile.mY),
		FTileIndex(Enemy->mTile.mX, Enemy->mTile.mY - 1), FTileIndex(Enemy->mTile.mX, Enemy->mTile.mY + 1),
	};
	int32 BestDistance = MAX_int32;
	OutTarget = FTileIndex::Invalid;
	for (const FTileIndex& Candidate : Candidates)
	{
		if (TileMap->IsValidIndex(Candidate) == false || Reachable.Contains(Candidate) == false) continue;
		const int32 Distance = FMath::Abs(Candidate.mX - Player->mTile.mX) + FMath::Abs(Candidate.mY - Player->mTile.mY);
		if (Distance < BestDistance)
		{
			BestDistance = Distance;
			OutTarget = Candidate;
		}
	}
	return OutTarget != FTileIndex::Invalid;
}

bool UCombatTutorialGuide::GetTargetRect(FVector2D& OutTopLeft, FVector2D& OutBottomRight) const
{
	if (mOwner == nullptr)
	{
		return false;
	}
	switch (mStep)
	{
	case ECombatTutorialGuideStep::RollDiceBoard:
	case ECombatTutorialGuideStep::WaitDiceResult:
	case ECombatTutorialGuideStep::DismissDiceBoard:
		return GetWidgetRect(mOwner->GetTutorialDiceRollAnchor(), OutTopLeft, OutBottomRight);
	case ECombatTutorialGuideStep::SelectStepSkill:
		return GetWidgetRect(mOwner->GetTutorialSkillAnchor(mStepSkillId), OutTopLeft, OutBottomRight);
	case ECombatTutorialGuideStep::SelectAttackSkill:
		return GetWidgetRect(mOwner->GetTutorialSkillAnchor(mAttackSkillId), OutTopLeft, OutBottomRight);
	case ECombatTutorialGuideStep::SelectBuffSkill:
		return GetWidgetRect(mOwner->GetTutorialSkillAnchor(mDefenseSkillId), OutTopLeft, OutBottomRight);
	case ECombatTutorialGuideStep::SelectStepDice:
	case ECombatTutorialGuideStep::SelectAttackDice:
	case ECombatTutorialGuideStep::SelectBuffDice:
		return GetWidgetRect(mOwner->GetTutorialAvailableDiceAnchor(), OutTopLeft, OutBottomRight);
	case ECombatTutorialGuideStep::PreviewStep:
	case ECombatTutorialGuideStep::ConfirmStep:
	case ECombatTutorialGuideStep::WaitStepResolved:
		return GetUnitRect(true, OutTopLeft, OutBottomRight);
	case ECombatTutorialGuideStep::PressMove:
		return GetWidgetRect(mOwner->GetTutorialMoveAnchor(), OutTopLeft, OutBottomRight);
	case ECombatTutorialGuideStep::PreviewMove:
	case ECombatTutorialGuideStep::ConfirmMove:
	case ECombatTutorialGuideStep::WaitMoveResolved:
		return GetTileRect(mMoveTargetTile, OutTopLeft, OutBottomRight);
	case ECombatTutorialGuideStep::PreviewAttack:
	case ECombatTutorialGuideStep::ConfirmAttack:
	case ECombatTutorialGuideStep::WaitAttackResolved:
		return GetUnitRect(false, OutTopLeft, OutBottomRight);
	case ECombatTutorialGuideStep::PreviewBuff:
	case ECombatTutorialGuideStep::ConfirmBuff:
	case ECombatTutorialGuideStep::WaitBuffResolved:
		return GetUnitRect(true, OutTopLeft, OutBottomRight);
	default:
		return false;
	}
}

void UCombatTutorialGuide::UpdateLayout()
{
	UCanvasPanel* RootCanvas = mOwner != nullptr ? mOwner->GetTutorialOverlayCanvas() : nullptr;
	if (IsActive() == false || RootCanvas == nullptr || mScrims.Num() != 4 || mInputBlockers.Num() != 4 || mOutlineEdges.Num() != 4)
	{
		return;
	}
	FVector2D TopLeft;
	FVector2D BottomRight;
	if (GetTargetRect(TopLeft, BottomRight) == false)
	{
		return;
	}
	const FVector2D ViewportSize = RootCanvas->GetCachedGeometry().GetLocalSize();
	TopLeft -= FVector2D(TargetPadding);
	BottomRight += FVector2D(TargetPadding);
	TopLeft.X = FMath::Clamp(TopLeft.X, 0.0f, ViewportSize.X);
	TopLeft.Y = FMath::Clamp(TopLeft.Y, 0.0f, ViewportSize.Y);
	BottomRight.X = FMath::Clamp(BottomRight.X, 0.0f, ViewportSize.X);
	BottomRight.Y = FMath::Clamp(BottomRight.Y, 0.0f, ViewportSize.Y);

	const FVector2D SegmentPositions[] = {
		FVector2D::ZeroVector,
		FVector2D(0.0f, BottomRight.Y),
		FVector2D(0.0f, TopLeft.Y),
		FVector2D(BottomRight.X, TopLeft.Y),
	};
	const FVector2D SegmentSizes[] = {
		FVector2D(ViewportSize.X, TopLeft.Y),
		FVector2D(ViewportSize.X, ViewportSize.Y - BottomRight.Y),
		FVector2D(TopLeft.X, BottomRight.Y - TopLeft.Y),
		FVector2D(ViewportSize.X - BottomRight.X, BottomRight.Y - TopLeft.Y),
	};
	for (int32 Index = 0; Index < 4; ++Index)
	{
		RDUILayout::ApplyFixedSlot(mScrims[Index], FAnchors(0.0f), FVector2D::ZeroVector, SegmentPositions[Index], SegmentSizes[Index], ScrimZOrder);
		RDUILayout::ApplyFixedSlot(mInputBlockers[Index], FAnchors(0.0f), FVector2D::ZeroVector, SegmentPositions[Index], SegmentSizes[Index], InputBlockerZOrder);
	}

	const float Width = BottomRight.X - TopLeft.X;
	const float Height = BottomRight.Y - TopLeft.Y;
	RDUILayout::ApplyFixedSlot(mOutlineEdges[0], FAnchors(0.0f), FVector2D::ZeroVector, TopLeft, FVector2D(Width, OutlineThickness), OutlineZOrder);
	RDUILayout::ApplyFixedSlot(mOutlineEdges[1], FAnchors(0.0f), FVector2D::ZeroVector, FVector2D(TopLeft.X, BottomRight.Y - OutlineThickness), FVector2D(Width, OutlineThickness), OutlineZOrder);
	RDUILayout::ApplyFixedSlot(mOutlineEdges[2], FAnchors(0.0f), FVector2D::ZeroVector, TopLeft, FVector2D(OutlineThickness, Height), OutlineZOrder);
	RDUILayout::ApplyFixedSlot(mOutlineEdges[3], FAnchors(0.0f), FVector2D::ZeroVector, FVector2D(BottomRight.X - OutlineThickness, TopLeft.Y), FVector2D(OutlineThickness, Height), OutlineZOrder);

	const bool bTargetOnBottom = (TopLeft.Y + BottomRight.Y) * 0.5f > ViewportSize.Y * 0.55f;
	RDUILayout::ApplyAnchoredSlot(mGuidePanel,
		bTargetOnBottom ? FAnchors(0.20f, 0.055f, 0.80f, 0.18f) : FAnchors(0.20f, 0.79f, 0.80f, 0.915f), PanelZOrder);
	RDUILayout::ApplyAnchoredSlot(mSkipButton, FAnchors(0.835f, 0.035f, 0.965f, 0.105f), SkipZOrder);
	if (mGuideText != nullptr) mGuideText->SetText(GetInstruction(mStep));
}
