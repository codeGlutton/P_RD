#include "Tutorial/FirstPlayTutorialSubsystem.h"
#include "Engine/GameInstance.h"
#include "Components/Button.h"
#include "SRPGFramework/SRPGMoveAction.h"
#include "SRPGFramework/SRPGSkillAction.h"
#include "Singleton/InstanceSubsystem/PersistentData.h"
#include "Singleton/InstanceSubsystem/SaveGameSubsystem.h"
#include "UI/Tutorial/GuidedTutorialWidget.h"
#include "UI/Combat/CombatLayoutHUDWidget.h"
#include "Tutorial/TutorialInputGate.h"
#include "Engine/GameViewportClient.h"
#include "Framework/Application/SlateApplication.h"
#include "Layout/WidgetPath.h"
#include "Widgets/SViewport.h"
#include "Tutorial/FirstBattleScenario.h"
#include "Tutorial/StaticTutorialRoomSpawnData.h"
#include "Pawn/Player/PlayerUnitModel.h"

bool UFirstPlayTutorialSubsystem::NeedsShopGuide() const
{
	return !GetUserMutableData()->SeenShopWalkthrough;
}

void UFirstPlayTutorialSubsystem::AcknowledgeShopGuide()
{
	GetUserMutableData()->SeenShopWalkthrough = true;
	Save();
	UE_LOG(LogTemp, Display, TEXT("First shop guide acknowledged"));
}

void UFirstPlayTutorialSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	if (!FSlateApplication::IsInitialized()) return;
	InputGate = MakeShared<FTutorialInputGate>();
	InputGate->IsActive = [this] { return IsInputRestricted(); };
	InputGate->IsAllowed = [this](const FVector2D& Position) { return IsPointerAllowed(Position); };
	InputGate->IsInViewport = [this](const FVector2D& Position)
	{
		const auto* Client = GetGameInstance()->GetGameViewportClient();
		const auto Viewport = Client ? Client->GetGameViewportWidget() : nullptr;
		return Viewport.IsValid() && Viewport->GetCachedGeometry().IsUnderLocation(Position);
	};
	InputGate->IsGameFocused = [this]
	{
		const auto* Client = GetGameInstance()->GetGameViewportClient();
		const auto Viewport = Client ? Client->GetGameViewportWidget() : nullptr;
		return Viewport.IsValid() && (Viewport->HasKeyboardFocus() || Viewport->HasFocusedDescendants());
	};
	FSlateApplication::Get().RegisterInputPreProcessor(InputGate, 0);
}

void UFirstPlayTutorialSubsystem::Deinitialize()
{
	HideEncounterHint();
	if (InputGate && FSlateApplication::IsInitialized())
		FSlateApplication::Get().UnregisterInputPreProcessor(InputGate);
	InputGate.Reset();
	HideGuided();
	Super::Deinitialize();
}

bool UFirstPlayTutorialSubsystem::IsInputRestricted() const
{
	if (IsEncounterHintVisible()) return true;
	if (!bInCombat) return false;
	const auto* Data = GetUserMutableData();
	const auto& Flow = Data->GuidedTutorial;
	return Flow.Enrolled && !Data->TutorialProgress.Skipped &&
		(Flow.CanGuideCombat() || Flow.SystemsActive || Flow.Lesson != EGuidedLesson::None);
}

bool UFirstPlayTutorialSubsystem::IsPointerAllowed(const FVector2D& Position) const
{
	if (!IsInputRestricted()) return true;
	auto* HUD = LastHUD.Get();
	if (!bPlayerTurn || bExecutingAction || !HUD || HUD->IsGuidedOverlayObscured()) return false;
	if (IsEncounterHintVisible())
	{
		const auto* Button = EncounterWidget->GetContinueButton();
		if (!Button || !Button->GetCachedGeometry().IsUnderLocation(Position)) return false;
		auto& Slate = FSlateApplication::Get();
		const FWidgetPath Path = Slate.LocateWindowUnderMouse(Position, Slate.GetInteractiveTopLevelWindows());
		for (int32 Index = 0; Index < Path.Widgets.Num(); ++Index)
			if (Path.Widgets[Index].Widget == Button->GetCachedWidget()) return true;
		return false;
	}
	const auto Stage = GetUserMutableData()->GuidedTutorial.Stage;
	auto Next = Stage;
	bool Board = false, Retry = false;
	const auto* Target = HUD->ResolveGuidedTarget(Stage, Next, Board, Retry);
	// Wait for the next HUD update to present the new instruction before accepting input.
	if (Next != Stage) return false;
	if (FGuidedTutorialProgress::IsReadingStep(Stage))
		Target = GuidedWidget ? GuidedWidget->GetContinueButton() : nullptr;
	auto& Slate = FSlateApplication::Get();
	const FWidgetPath Path = Slate.LocateWindowUnderMouse(Position, Slate.GetInteractiveTopLevelWindows());
	if (Board)
	{
		if (!HUD->IsGuidedBoardInputAt(Position) || !IsScenarioPointerAllowed(Position)) return false;
		bool InHUD = false;
		for (int32 Index = 0; Index < Path.Widgets.Num(); ++Index)
		{
			const auto& Entry = Path.Widgets[Index];
			InHUD |= Entry.Widget == HUD->GetCachedWidget();
			if (Entry.Widget->GetType() == FName(TEXT("SButton"))) return false;
		}
		return InHUD;
	}
	if (!Target || !Target->IsVisible() || !Target->GetIsEnabled() ||
		!Target->GetCachedGeometry().IsUnderLocation(Position)) return false;
	for (int32 Index = 0; Index < Path.Widgets.Num(); ++Index)
		if (Path.Widgets[Index].Widget == Target->GetCachedWidget()) return true;
	return false;
}

void UFirstPlayTutorialSubsystem::PrepareFirstProfile()
{
	if (bProfileChecked)
		return;
	bProfileChecked = true;
	if (const auto* Saver = GetGameInstance()->GetSubsystem<USaveGameSubsystem>())
		GetUserMutableData()->GuidedTutorial.EnrollIfNoPlayData(Saver->HasPlaySave());
}
void UFirstPlayTutorialSubsystem::TitleOpened(UUserWidget*)
{
	HideEncounterHint();
	HideGuided();
	bInCombat = false;
	LastHUD.Reset();
	LastLessonContext = EGuidedLesson::None;
}
void UFirstPlayTutorialSubsystem::TitleClosed(UUserWidget*)
{
	HideGuided();
}
void UFirstPlayTutorialSubsystem::TurnStarted(bool PlayerTurn)
{
	const bool WasInCombat = bInCombat;
	bInCombat = true;
	bPlayerTurn = PlayerTurn;
	bExecutingAction = false;
	HideEncounterHint();
	auto* Data = GetUserMutableData();
	if (!Data->GuidedTutorial.Enrolled || (Data->GuidedTutorial.NeedsFirstRoom(Data->TutorialProgress.Skipped) && !HasScenario()))
	{
		HideGuided();
		return;
	}
	if (!WasInCombat)
	{
		Data->GuidedTutorial.ResumeInNewBattle();
		LastLessonContext = EGuidedLesson::None;
	}
	bInCombat = true;
	bPlayerTurn = PlayerTurn;
	bExecutingAction = false;
	if (PlayerTurn && !Data->TutorialProgress.Skipped && Data->GuidedTutorial.BeginSystems())
		Save();
	if (!Data->TutorialProgress.WelcomeSeen && !Data->TutorialProgress.Skipped)
	{
		Data->TutorialProgress.WelcomeSeen = true;
		Save();
	}
	if (!PlayerTurn)
		HideGuided();
}
void UFirstPlayTutorialSubsystem::TurnEnded(bool PlayerTurn, bool Succeeded)
{
	HideEncounterHint();
	if (bInCombat && PlayerTurn && Succeeded)
	{
		AdvanceGuided(EGuidedStage::EndTurn);
		// An exhausted/disabled unit must never trap the next ally in a stale target-selection step.
		auto& Flow = GetUserMutableData()->GuidedTutorial;
		if (Flow.Stage == EGuidedStage::MoveTile || Flow.Stage == EGuidedStage::ConfirmMove)
		{
			Flow.Stage = EGuidedStage::SelectMove;
			Save();
		}
		if (Flow.Stage == EGuidedStage::SkillTarget || Flow.Stage == EGuidedStage::ConfirmSkill)
		{
			Flow.Stage = EGuidedStage::SelectSkill;
			Save();
		}
	}
	bPlayerTurn = false;
	HideGuided();
}
void UFirstPlayTutorialSubsystem::ActionEnded(bool PlayerAction, const USRPGAction* Action, bool Succeeded)
{
	if (!Action || Action->GetActionType() == ESRPGActionType::BuildAction) return;
	bExecutingAction = false;
	if (!bInCombat || !PlayerAction || !Succeeded || !Action)
		return;
	if (IsScenarioGuiding())
	{
		if (Action->GetInstigator() != ScenarioUnit.Get()) return;
		if (Action->IsA<USRPGMoveAction>() && ScenarioUnit->GetTileTransform().mIndex != GetScenarioMoveTile()) return;
	}
	bool Changed = false;
	if (Action->IsA<USRPGMoveAction>())
		Changed = GetUserMutableData()->GuidedTutorial.CompleteAction(true, PlayerAction, Succeeded);
	else if (Action->IsA<USRPGSkillAction>())
		Changed = GetUserMutableData()->GuidedTutorial.CompleteAction(false, PlayerAction, Succeeded);
	if (Changed)
	{
		Save();
		HideGuided();
	}
}
void UFirstPlayTutorialSubsystem::ActionStarted(const USRPGAction* Action)
{
	// Selecting a path/target is an interactive build action, not a blocking animation.
	if (!Action || Action->GetActionType() == ESRPGActionType::BuildAction) return;
	bExecutingAction = true;
	HideEncounterHint();
	HideGuided();
}
void UFirstPlayTutorialSubsystem::CombatEnded(bool Finished)
{
	HideEncounterHint();
	ScenarioRoom = nullptr;
	ScenarioUnit.Reset();
	if (Finished && bInCombat && GetUserMutableData()->GuidedTutorial.Enrolled)
	{
		GetUserMutableData()->GuidedTutorial.FinishFirstBattle();
		Save();
	}
	bInCombat = false;
	bExecutingAction = false;
	HideGuided();
}
void UFirstPlayTutorialSubsystem::HideGuided()
{
	if (BoundButton.IsValid())
		BoundButton->OnClicked.RemoveDynamic(this, &UFirstPlayTutorialSubsystem::GuidedClicked);
	BoundButton.Reset();
	BoundStage = EGuidedStage::Done;
	if (GuidedWidget)
		GuidedWidget->RemoveFromParent();
	GuidedWidget = nullptr;
}
void UFirstPlayTutorialSubsystem::Save()
{
	if (auto* Saver = GetGameInstance()->GetSubsystem<USaveGameSubsystem>())
		if (!Saver->SaveUser())
			UE_LOG(LogTemp, Warning, TEXT("Tutorial progress could not be saved."));
}
void UFirstPlayTutorialSubsystem::AdvanceGuided(EGuidedStage Expected)
{
	auto* Data = GetUserMutableData();
	if (!Data->GuidedTutorial.Enrolled || Data->TutorialProgress.Skipped ||
	    !Data->GuidedTutorial.Advance(Expected))
		return;
	Save();
	HideGuided();
}
void UFirstPlayTutorialSubsystem::GuidedClicked()
{
	AdvanceGuided(BoundStage);
}
void UFirstPlayTutorialSubsystem::UpdateGuidedHUD(UCombatLayoutHUDWidget* HUD)
{
	LastHUD = HUD;
	if (UpdateEncounterHint(HUD)) { HideGuided(); return; }
	auto* Data = GetUserMutableData();
	auto& Flow = Data->GuidedTutorial;
	if (!Flow.Enrolled)
	{
		HideGuided();
		return;
	}
	LastHUD = HUD;
	const EGuidedLesson Context = HUD->GuidedLessonContext();
	const bool LeavingLesson = !Flow.SystemsActive && (Flow.CoreComplete || Flow.FirstBattleFinished) &&
	                           Flow.Lesson != EGuidedLesson::None &&
	                           LastLessonContext != EGuidedLesson::None && Context != LastLessonContext;
	const bool ArtifactTransit = Flow.Lesson == EGuidedLesson::Artifact &&
	                             LastLessonContext == EGuidedLesson::Mercenary &&
	                             Context == EGuidedLesson::Artifact;
	if (LeavingLesson && !ArtifactTransit)
	{
		Flow.Lesson = EGuidedLesson::None;
		Flow.Stage = EGuidedStage::LessonMenu;
		Save();
		HideGuided();
	}
	if (!Flow.SystemsActive && (Flow.CoreComplete || Flow.FirstBattleFinished) && Flow.Stage == EGuidedStage::LessonMenu &&
	    Context != LastLessonContext && Context != EGuidedLesson::None && !Data->TutorialProgress.Skipped)
	{
		if (Flow.BeginLesson(Context, HUD->HasGuidedArtifact()))
		{
			Save();
			HideGuided();
		}
	}
	LastLessonContext = Context;
	if (HUD->IsGuidedOverlayObscured() || !bInCombat || !bPlayerTurn || bExecutingAction ||
	    Data->TutorialProgress.Skipped || Flow.Stage == EGuidedStage::Done ||
	    Flow.Stage == EGuidedStage::LessonMenu)
	{
		HideGuided();
		return;
	}
	// A battle may have ended before the enemy-information step could start.
	if (Flow.SystemsActive && Flow.Stage == EGuidedStage::OpenMonster && !HUD->HasGuidedMonster())
	{
		Flow.FinishSystems(); Save(); HideGuided(); return;
	}
	bool Board = false, Retry = false;
	EGuidedStage Next = Flow.Stage;
	UWidget* Target = HUD->ResolveGuidedTarget(Flow.Stage, Next, Board, Retry);
	if (Next != Flow.Stage)
	{
		if (Next == FGuidedTutorialProgress::NextStage(Flow.Stage))
			Flow.Advance(Flow.Stage);
		else
			Flow.Stage = Next;
		Save();
		HideGuided();
		return;
	}
	bool TargetVisible = Target != nullptr;
	for (UWidget* Parent = Target; Parent; Parent = Parent->GetParent())
		if (!Parent->IsVisible())
		{
			TargetVisible = false;
			break;
		}
	if (Flow.Stage != EGuidedStage::LessonMenu && !Board &&
	    (!TargetVisible || Target->GetCachedGeometry().GetLocalSize().IsNearlyZero()))
	{
		HideGuided();
		return;
	}
	if (!GuidedWidget)
	{
		GuidedWidget = CreateWidget<UGuidedTutorialWidget>(GetGameInstance());
		if (!GuidedWidget)
			return;
		GuidedWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		GuidedWidget->AddToViewport(75);
	}
	GuidedWidget->Present(Flow.Stage, Target, Board, Retry);
	TArray<FVector2D> WorldCorners;
	GetScenarioFocus(WorldCorners);
	GuidedWidget->SetWorldFocus(WorldCorners);
	if (IsScenarioGuiding() && !Retry)
	{
		if (Flow.Stage == EGuidedStage::MoveTile)
			GuidedWidget->SetNotice(NSLOCTEXT("Guided", "FixedMove", "빛나는 칸을 누르세요.\n앞으로 한 칸 이동합니다."));
		else if (Flow.Stage == EGuidedStage::SkillTarget)
			GuidedWidget->SetNotice(NSLOCTEXT("Guided", "FixedEnemy", "표시한 독수리를 누르세요."));
		else if (Flow.Stage == EGuidedStage::HoldSkill)
			GuidedWidget->SetNotice(FText::Format(NSLOCTEXT("Guided", "FixedHold", "{0}\n길게 눌러 설명을 확인하세요."), ScenarioSkillName));
		else if (Flow.Stage == EGuidedStage::SelectSkill)
			GuidedWidget->SetNotice(FText::Format(NSLOCTEXT("Guided", "FixedSkill", "{0}\n짧게 눌러 선택하세요."), ScenarioSkillName));
	}
	if (Retry && Target && Target->GetFName() == TEXT("DetailCloseButton"))
		GuidedWidget->SetNotice(NSLOCTEXT("Guided", "CloseUnexpectedDetail", "설명을 닫고 카드를 짧게 누르세요."));
	else if (Target && Target->GetFName() == TEXT("EndTurnButton") && Flow.Stage != EGuidedStage::EndTurn)
		GuidedWidget->SetNotice(
		    NSLOCTEXT("Guided", "Unavailable", "해당 행동을 할 수 없습니다.\n턴을 종료하세요."));
	else if (Target && Target->GetFName() == TEXT("SkillToggleButton") &&
	         Flow.Stage != EGuidedStage::OpenSkills)
		GuidedWidget->SetNotice(NSLOCTEXT("Guided", "Reopen", "행동 카드를 펼치세요."));
	bool Click = Flow.Stage == EGuidedStage::OpenSkills || Flow.Stage == EGuidedStage::SelectMercenary ||
	             Flow.Stage == EGuidedStage::OpenMercenary || Flow.Stage == EGuidedStage::OpenInventory ||
	             Flow.Stage == EGuidedStage::OpenMonster || Flow.Stage == EGuidedStage::SelectMonster ||
	             Flow.Stage == EGuidedStage::CloseMercenary || Flow.Stage == EGuidedStage::CloseMonster ||
	             Flow.Stage == EGuidedStage::CloseSkill || Flow.Stage == EGuidedStage::CloseMercenarySkill ||
	             Flow.Stage == EGuidedStage::CloseArtifact || Flow.Stage == EGuidedStage::CloseMonsterSkill;
	Click |= Flow.Stage == EGuidedStage::SelectRange || Flow.Stage == EGuidedStage::EffectRange;
	UButton* Button = FGuidedTutorialProgress::IsReadingStep(Flow.Stage)
	                      ? GuidedWidget->GetContinueButton() : (Click ? Cast<UButton>(Target) : nullptr);
	if (BoundButton.Get() != Button || BoundStage != Flow.Stage)
	{
		if (BoundButton.IsValid())
			BoundButton->OnClicked.RemoveDynamic(this, &UFirstPlayTutorialSubsystem::GuidedClicked);
		BoundButton = Button;
		BoundStage = Flow.Stage;
		if (Button)
			Button->OnClicked.AddUniqueDynamic(this, &UFirstPlayTutorialSubsystem::GuidedClicked);
	}
}
