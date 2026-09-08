#include "Misc/AutomationTest.h"
#include "Tutorial/GuidedTutorial.h"
#include "UI/Tutorial/GuidedTutorialWidget.h"
#include "UI/Combat/CombatLayoutHUDWidget.h"
#include "UI/Combat/CombatUIModel.h"
#include "Singleton/InstanceSubsystem/PersistentData.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"
#include "Editor.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Blueprint/WidgetTree.h"
#include "Engine/Texture2D.h"
#include "TextureCompiler.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Slate/WidgetRenderer.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Layout/SBox.h"
#include "ImageUtils.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "HAL/FileManager.h"
#include "RHI.h"
#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGuidedWorldFocusRenderTest, "P_RD.Tutorial.Guided.WorldFocusPaint",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGuidedWorldFocusRenderTest::RunTest(const FString&)
{
	if (GUsingNullRHI) { AddError(TEXT("Rendering required")); return false; }
	auto* Guide = CreateWidget<UGuidedTutorialWidget>(GEditor->GetEditorWorldContext().World());
	const auto Slate = Guide->TakeWidget();
	FTextureCompilingManager::Get().FinishAllCompilation();
	FWidgetRenderer Renderer(true, true);
	Renderer.SetIsPrepassNeeded(true);
	for (const auto Stage : {EGuidedStage::MoveTile, EGuidedStage::SkillTarget})
	{
		Guide->Present(Stage, nullptr, true);
		Guide->SetWorldFocus({{100, 200}, {200, 150}, {300, 200}, {200, 250}});
		Renderer.DrawWidget(Slate, FVector2D(800, 600));
		Guide->UpdateLayoutForTest();
		// Exercises NativePaint with four projected corners: the reported crash path.
		auto* Target = Renderer.DrawWidget(Slate, FVector2D(800, 600));
		FlushRenderingCommands();
		TArray<FColor> Pixels;
		Target->GameThread_GetRenderTargetResource()->ReadPixels(Pixels);
		bool HasOutline = false;
		for (int32 Y = 147; Y <= 153; ++Y) for (int32 X = 197; X <= 203; ++X)
			HasOutline |= Pixels.IsValidIndex(Y * 800 + X) && Pixels[Y * 800 + X].A > 0;
		TestTrue(TEXT("Projected movement/attack outline is actually painted"), HasOutline);
		Guide->SetWorldFocus({});
		Guide->UpdateLayoutForTest();
		Renderer.DrawWidget(Slate, FVector2D(800, 600));
		FlushRenderingCommands();
	}
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGuidedFlowTest, "P_RD.Tutorial.Guided.Flow",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGuidedFlowTest::RunTest(const FString&)
{
	FGuidedTutorialProgress P;
	TestFalse(TEXT("Existing profiles are not enrolled by a new default field"), P.Enrolled);
	TestFalse(TEXT("Any existing play save prevents auto enrollment"), P.EnrollIfNoPlayData(true));
	TestTrue(TEXT("No play save enrolls the first battle"), P.EnrollIfNoPlayData(false));
	TestTrue(TEXT("Fresh profile can receive combat guidance"), P.CanGuideCombat());
	TestFalse(TEXT("Unrelated click cannot advance"), P.Advance(EGuidedStage::OpenInventory));
	TestTrue(TEXT("Actual opening advances"), P.Advance(EGuidedStage::OpenSkills));

	TestEqual(TEXT("Move is taught before opening a manual"), P.Stage, EGuidedStage::SelectMove);
	P.Stage = EGuidedStage::MoveTile;
	TestFalse(TEXT("Cancelled movement ignored"), P.CompleteAction(true, true, false));
	TestFalse(TEXT("Enemy movement ignored"), P.CompleteAction(true, false, true));
	TestTrue(TEXT("Actual move finishes first goal"), P.CompleteAction(true, true, true));
	TestEqual(TEXT("Read skill in attack context"), P.Stage, EGuidedStage::HoldSkill);
	TestFalse(TEXT("Short selection cannot skip skill reading"), P.CompleteAction(false, true, true));
	P.Advance(EGuidedStage::HoldSkill);
	P.Advance(EGuidedStage::CloseSkill);
	TestEqual(TEXT("Use skill after reading it"), P.Stage, EGuidedStage::SelectSkill);
	TestTrue(TEXT("Actual skill advances"), P.CompleteAction(false, true, true));
	P.Advance(EGuidedStage::EndTurn);
	TestTrue(TEXT("Core combat is complete"), P.CoreComplete);
	TestEqual(TEXT("Information is a choice, not a compulsory chain"), P.Stage, EGuidedStage::LessonMenu);
	TestFalse(TEXT("Unavailable artifact lesson cannot start"),
	          P.BeginLesson(EGuidedLesson::Artifact, false));
	TestTrue(TEXT("Player can choose enemy lesson first"), P.BeginLesson(EGuidedLesson::Monster, false));
	TestEqual(TEXT("Chosen topic controls entry"), P.Stage, EGuidedStage::OpenMonster);
	P.Stage = EGuidedStage::CloseMonster;
	P.Advance(EGuidedStage::CloseMonster);
	TestTrue(TEXT("Lesson read remembered"), P.HasLearned(EGuidedLesson::Monster));
	TestEqual(TEXT("One lesson does not force another"), P.Stage, EGuidedStage::LessonMenu);
	TestFalse(TEXT("Read lesson is not repeated"), P.BeginLesson(EGuidedLesson::Monster, true));
	P.BeginLesson(EGuidedLesson::Artifact, true);
	P.Advance(EGuidedStage::OpenMercenary);
	TestEqual(TEXT("Artifact lesson skips unrelated party steps"), P.Stage, EGuidedStage::OpenInventory);
	P.FinishFirstBattle();
	TestFalse(TEXT("Later battle rooms never repeat core guidance"), P.CanGuideCombat());
	P.ResumeInNewBattle();
	TestEqual(TEXT("Optional guide does not resume in a hidden panel"), P.Stage, EGuidedStage::LessonMenu);
	P = FGuidedTutorialProgress();
	P.Stage = EGuidedStage::Done;
	P.ResumeInNewBattle();
	TestTrue(TEXT("Legacy completion retained"), P.CoreComplete && P.LessonsRead == 7);
	auto* Source = NewObject<UUserPersistData>();
	Source->GuidedTutorial.EnrollIfNoPlayData(false);
	Source->MakeUser(FText::FromString(TEXT("Player")));
	TestTrue(TEXT("Naming first profile preserves enrollment"), Source->GuidedTutorial.Enrolled);
	Source->GuidedTutorial.FirstBattleFinished = true;
	Source->GuidedTutorial.Stage = EGuidedStage::OpenInventory;
	TArray<uint8> Bytes;
	{
		FMemoryWriter W(Bytes);
		FObjectAndNameAsStringProxyArchive A(W, false);
		A.ArIsSaveGame = true;
		A.ArNoDelta = true;
		Source->Serialize(A);
	}
	auto* Loaded = NewObject<UUserPersistData>();
	{
		FMemoryReader R(Bytes);
		FObjectAndNameAsStringProxyArchive A(R, false);
		A.ArIsSaveGame = true;
		A.ArNoDelta = true;
		Loaded->Serialize(A);
	}
	TestEqual(TEXT("Guided checkpoint persists independently"), Loaded->GuidedTutorial.Stage,
	          EGuidedStage::OpenInventory);
	TestTrue(TEXT("Enrollment and first room completion persist"),
	         Loaded->GuidedTutorial.Enrolled && Loaded->GuidedTutorial.FirstBattleFinished);
	Loaded->ClearUser();
	TestEqual(TEXT("Profile reset resets guided flow"), Loaded->GuidedTutorial.Stage,
	          EGuidedStage::OpenSkills);
	TestFalse(TEXT("Reset alone does not enroll existing players"), Loaded->GuidedTutorial.Enrolled);
	return !HasAnyErrors();
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGuidedRenderTest, "P_RD.Tutorial.Guided.TargetsAndCapture",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGuidedRenderTest::RunTest(const FString&)
{
	if (GUsingNullRHI)
	{
		AddError(TEXT("Rendering required"));
		return false;
	}
	auto* World = GEditor->GetEditorWorldContext().World();
	auto* Class = LoadClass<UCombatLayoutHUDWidget>(
	    nullptr, TEXT("/Game/UI/CombatLayouts/WBP_CombatHUD04.WBP_CombatHUD04_C"));
	if (!TestNotNull(TEXT("HUD"), Class))
		return false;
	auto* HUD = CreateWidget<UCombatLayoutHUDWidget>(World, Class);
	HUD->mUsePreviewData = false;
	HUD->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	const auto HUDSlate = HUD->TakeWidget();
	auto* Model = NewObject<UCombatUIModel>(HUD);
	HUD->BindUIModel(Model);
	FUnitUI U;
	U.mUnitId = 1;
	U.mIsPlayer = true;
	U.mName = FText::FromString(TEXT("기사"));
	U.mHP = 100;
	U.mMaxHP = 100;
	U.mMovementPoint = 12;
	U.mMaxMovementPoint = 12;
	U.mActionPoints = 12;
	U.mMaxActionPoints = 12;
	U.mTurnPortrait =
	    LoadObject<UTexture2D>(nullptr, TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Mercenaries/"
	                                         "T_MB_HireIcon_Knight.T_MB_HireIcon_Knight"));
	Model->SetUnitUIs({U});
	FSkillUI Skill;
	Skill.mSkillIndex = 0;
	Skill.mIsUsable = true;
	Skill.mIcon = LoadObject<UTexture2D>(
	    nullptr,
	    TEXT(
	        "/Game/SVN/OutSideAsset/AICreation/UI/CombatHUD/SkillIcons/T_SkillIcon_Slash.T_SkillIcon_Slash"));
	Model->SetSkillUIs({Skill});
	FTurnUI Turn;
	Turn.mRound = 1;
	Turn.mCurrentUnitId = 1;
	Turn.mTurnOrderUnitIds = {1};
	Model->SetTurnUI(Turn);
	Model->OnBeginAnyTurn.Broadcast(nullptr);
	auto* Guide = CreateWidget<UGuidedTutorialWidget>(World);
	Guide->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	auto Root = SNew(SOverlay) + SOverlay::Slot()[HUDSlate] + SOverlay::Slot()[Guide->TakeWidget()];
	auto Scaled = SNew(SScaleBox).Stretch(
	    EStretch::ScaleToFit)[SNew(SBox).WidthOverride(1920).HeightOverride(1080)[Root]];
	FWidgetRenderer Renderer(true, true);
	Renderer.SetIsPrepassNeeded(true);
	const FString Folder = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("UI/FieldGuide"));
	IFileManager::Get().MakeDirectory(*Folder, true);
	for (EGuidedStage S : {EGuidedStage::ReadTurnOrder, EGuidedStage::ReadAP, EGuidedStage::ReadCondition,
	                       EGuidedStage::ReadStatus, EGuidedStage::OpenSkills, EGuidedStage::HoldSkill,
	                       EGuidedStage::OpenMercenary, EGuidedStage::SelectMercenary,
	                       EGuidedStage::ReadMercenaryStats, EGuidedStage::ReadInventory})
	{
		if (S == EGuidedStage::HoldSkill)
			CastChecked<UButton>(HUD->GetWidgetFromName(TEXT("SkillToggleButton")))->OnClicked.Broadcast();
		if (S == EGuidedStage::SelectMercenary)
			CastChecked<UButton>(HUD->GetWidgetFromName(TEXT("MenuButton_1")))->OnClicked.Broadcast();
		if (S == EGuidedStage::ReadMercenaryStats)
			CastChecked<UButton>(HUD->GetWidgetFromName(TEXT("PartyButton_0")))->OnClicked.Broadcast();
		if (S == EGuidedStage::ReadInventory)
			CastChecked<UButton>(HUD->GetWidgetFromName(TEXT("MercenaryInventoryButton")))->OnClicked.Broadcast();
		bool Board = false, Retry = false;
		EGuidedStage Next = S;
		UWidget* Target = HUD->ResolveGuidedTarget(S, Next, Board, Retry);
		if (S != EGuidedStage::LessonMenu)
			TestNotNull(TEXT("Actual step button exists"), Target);
		TestEqual(TEXT("Opening target alone cannot advance"), Next, S);
		Guide->Present(S, Target, Board, Retry);
		TestEqual(TEXT("Only explanations show the acknowledgement button"),
		          Guide->GetContinueButton()->IsVisible(), FGuidedTutorialProgress::IsReadingStep(S));
		if (S == EGuidedStage::LessonMenu)
			CastChecked<UButton>(HUD->GetWidgetFromName(TEXT("MercenaryCloseButton")))->OnClicked.Broadcast();
		FTextureCompilingManager::Get().FinishAllCompilation();
		HUD->WidgetTree->ForEachWidget([](UWidget* Widget) {
			if (auto* Icon = Cast<UImage>(Widget))
				if (auto* Texture = Cast<UTexture2D>(Icon->GetBrush().GetResourceObject()))
				{
					Texture->SetForceMipLevelsToBeResident(30);
					Texture->WaitForStreaming();
				}
		});
		for (int32 I = 0; I < 4; ++I)
		{
			Renderer.DrawWidget(Scaled, FVector2D(1280, 720));
			FlushRenderingCommands();
			Guide->UpdateLayoutForTest();
		}
		auto* T = Renderer.DrawWidget(Scaled, FVector2D(1280, 720));
		FlushRenderingCommands();
		TArray<FColor> Pixels;
		FReadSurfaceDataFlags Flags(RCM_UNorm);
		Flags.SetLinearToGamma(false);
		T->GameThread_GetRenderTargetResource()->ReadPixels(Pixels, Flags);
		// Match the existing CombatDetail capture path: remove the offscreen RGBA8 double gamma.
		for (FColor& Pixel : Pixels)
		{
			Pixel.R = uint8(FMath::RoundToInt(255.f * FMath::Pow(Pixel.R / 255.f, 2.2f)));
			Pixel.G = uint8(FMath::RoundToInt(255.f * FMath::Pow(Pixel.G / 255.f, 2.2f)));
			Pixel.B = uint8(FMath::RoundToInt(255.f * FMath::Pow(Pixel.B / 255.f, 2.2f)));
		}
		TArray64<uint8> Png;
		FImageUtils::PNGCompressImageArray(1280, 720, Pixels, Png);
		TestTrue(TEXT("Write guided screenshot"),
		         FFileHelper::SaveArrayToFile(
		             Png, *FPaths::Combine(Folder, FString::Printf(TEXT("Step_%d.png"), int32(S)))));
	}
	bool Board = false, Retry = false;
	EGuidedStage Next = EGuidedStage::Artifact;
	HUD->ResolveGuidedTarget(Next, Next, Board, Retry);
	TestEqual(TEXT("Empty artifacts cannot trap player"), Next, EGuidedStage::CloseMercenary);
	Next = EGuidedStage::CloseSkill;
	HUD->ResolveGuidedTarget(EGuidedStage::CloseSkill, Next, Board, Retry);
	TestEqual(TEXT("Closing detail through another input also continues the guide"), Next,
	          EGuidedStage::SelectSkill);
	FSkillDetailUI AccidentalDetail;
	AccidentalDetail.mSkillIndex = 0;
	AccidentalDetail.mName = FText::FromString(TEXT("Skill"));
	Model->SetSkillDetail(AccidentalDetail);
	TestTrue(TEXT("Long-press detail is actually open"), HUD->IsDetailOverlayShown());
	for (const auto Stage : {EGuidedStage::SelectMove, EGuidedStage::SelectSkill})
	{
		const auto* Recovery = HUD->ResolveGuidedTarget(Stage, Next, Board, Retry);
		TestTrue(TEXT("Unexpected hold offers recovery without skipping the step"), Retry && Next == Stage);
		if (TestNotNull(TEXT("Forced guide always permits closing accidental detail"), Recovery))
			TestEqual(TEXT("Recovery targets actual overlay close button"), Recovery->GetFName(), FName(TEXT("DetailCloseButton")));
	}
	const auto DetailSlate = HUD->GetDetailOverlayWidgetForTest()->TakeWidget();
	auto DetailLayers = SNew(SOverlay) + SOverlay::Slot()[DetailSlate] + SOverlay::Slot()[Guide->TakeWidget()];
	auto DetailRoot = SNew(SScaleBox).Stretch(EStretch::ScaleToFit)[SNew(SBox).WidthOverride(1920).HeightOverride(1080)[DetailLayers]];
	for (const auto Stage : {EGuidedStage::ReadSkillCost, EGuidedStage::ReadSkillCooldown,
	                         EGuidedStage::SelectRange, EGuidedStage::EffectRange, EGuidedStage::ReadEnemySkill})
	{
		if (Stage == EGuidedStage::ReadEnemySkill)
		{
			// Enemy skill is a newly opened detail, after closing the mercenary preview.
			HUD->CloseDetailOverlayForTest();
			AccidentalDetail.mSkillIndex = 1;
			AccidentalDetail.mDescription = FText::FromString(TEXT("공격 대상에게 피해를 줍니다."));
			Model->SetSkillDetail(AccidentalDetail);
		}
		auto* Target = HUD->ResolveGuidedTarget(Stage, Next, Board, Retry);
		if (!TestNotNull(TEXT("Authored skill panel supplies actual system target"), Target)) continue;
		Guide->Present(Stage, Target);
		for (int I = 0; I < 3; ++I)
		{
			Renderer.DrawWidget(DetailRoot, FVector2D(1280, 720));
			FlushRenderingCommands();
			Guide->UpdateLayoutForTest();
		}
		TestFalse(TEXT("Skill explanation target is laid out"), Target->GetCachedGeometry().GetLocalSize().IsNearlyZero());
		TestTrue(*FString::Printf(TEXT("Skill explanation %d target is visible"), int(Stage)), Target->IsVisible());
		if (auto* Button = Cast<UButton>(Target)) Button->OnClicked.Broadcast();
		auto* RT = Renderer.DrawWidget(DetailRoot, FVector2D(1280, 720));
		FlushRenderingCommands();
		TArray<FColor> Pixels; RT->GameThread_GetRenderTargetResource()->ReadPixels(Pixels);
		for (auto& Pixel : Pixels)
		{
			Pixel.R = uint8(FMath::RoundToInt(255.f * FMath::Pow(Pixel.R / 255.f, 2.2f)));
			Pixel.G = uint8(FMath::RoundToInt(255.f * FMath::Pow(Pixel.G / 255.f, 2.2f)));
			Pixel.B = uint8(FMath::RoundToInt(255.f * FMath::Pow(Pixel.B / 255.f, 2.2f)));
		}
		TArray64<uint8> Png; FImageUtils::PNGCompressImageArray(1280,720,Pixels,Png);
		FFileHelper::SaveArrayToFile(Png, *FPaths::Combine(Folder,FString::Printf(TEXT("Detail_%d.png"),int(Stage))));
	}
	HUD->CloseDetailOverlayForTest();
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGuidedSystemsFlowTest, "P_RD.Tutorial.Guided.SystemsFlow",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGuidedSystemsFlowTest::RunTest(const FString&)
{
	FGuidedTutorialProgress P;
	P.CoreComplete = true;
	TestFalse(TEXT("Existing players are not forcibly enrolled"), P.BeginSystems());
	P.Enrolled = true; P.CoreComplete = false;
	TestTrue(TEXT("First player turn begins information before combat practice"), P.BeginSystems());
	TestFalse(TEXT("Combat command gate must not block instructed information controls"), P.CanGuideCombat());
	TestFalse(TEXT("Repeated turn event cannot restart active guidance"), P.BeginSystems());
	TestFalse(TEXT("Opening a context cannot replace the fixed system sequence"), P.BeginLesson(EGuidedLesson::Monster, true));
	TestFalse(TEXT("Unrelated action cannot skip a system explanation"), P.Advance(EGuidedStage::EndTurn));
	const EGuidedStage Steps[] = {EGuidedStage::ReadTurnOrder, EGuidedStage::ReadAP,
	 EGuidedStage::ReadCondition, EGuidedStage::ReadStatus, EGuidedStage::OpenMercenary,
	 EGuidedStage::SelectMercenary, EGuidedStage::ReadMercenaryStats, EGuidedStage::MercenarySkill,
	 EGuidedStage::ReadSkillCost, EGuidedStage::ReadSkillCooldown, EGuidedStage::SelectRange,
	 EGuidedStage::EffectRange, EGuidedStage::CloseMercenarySkill, EGuidedStage::OpenInventory,
	 EGuidedStage::ReadInventory, EGuidedStage::Artifact, EGuidedStage::CloseArtifact,
	 EGuidedStage::CloseMercenary, EGuidedStage::OpenMonster, EGuidedStage::SelectMonster,
	 EGuidedStage::ReadEnemy, EGuidedStage::HoldMonsterSkill, EGuidedStage::ReadEnemySkill,
	 EGuidedStage::CloseMonsterSkill, EGuidedStage::CloseMonster};
	for (const auto Stage : Steps)
	{
		TestEqual(TEXT("Each system follows the fixed order"), P.Stage, Stage);
		TestFalse(TEXT("Every action and explanation has text"), UGuidedTutorialWidget::Instruction(Stage).IsEmpty());
		TestTrue(TEXT("Expected action advances the sequence"), P.Advance(Stage));
	}
	TestTrue(TEXT("System completion hands the input gate to combat practice"),
	         P.SystemsComplete && !P.SystemsActive && P.CanGuideCombat());
	TestEqual(TEXT("Closing enemy information starts combat practice in the same turn"), P.Stage, EGuidedStage::OpenSkills);
	TestTrue(TEXT("Shown information lessons are recorded"), P.LessonsRead == 7);
	TestFalse(TEXT("Completed systems never replay on another turn"), P.BeginSystems());
	P.Advance(EGuidedStage::OpenSkills);
	TestEqual(TEXT("Practice starts with the fixed move"), P.Stage, EGuidedStage::SelectMove);
	TestTrue(TEXT("Successful movement teaches skill reading next"), P.CompleteAction(true, true, true));
	P.Advance(EGuidedStage::HoldSkill); P.Advance(EGuidedStage::CloseSkill);
	TestTrue(TEXT("Successful skill use leads to ending the turn"), P.CompleteAction(false, true, true));
	P.Advance(EGuidedStage::EndTurn);
	TestTrue(TEXT("Only finishing both sections releases forced guidance"), P.CoreComplete && !P.CanGuideCombat());
	TestEqual(TEXT("Both sections complete at the lesson hub"), P.Stage, EGuidedStage::LessonMenu);
	P.CoreComplete = false;
	P.SystemsComplete = false; P.LessonsRead = 0; P.BeginSystems();
	P.Stage = EGuidedStage::ReadSkillCost;
	P.ResumeInNewBattle();
	TestFalse(TEXT("Reloading information does not incorrectly mark combat complete"), P.CoreComplete);
	TestTrue(TEXT("Interrupted tour can restart against closed panels"), P.BeginSystems());
	TestEqual(TEXT("Resume begins on the combat HUD"), P.Stage, EGuidedStage::ReadTurnOrder);
	P.Stage = EGuidedStage::CloseMercenary; P.Advance(P.Stage);
	TestFalse(TEXT("Empty inventory does not consume the future artifact lesson"), P.HasLearned(EGuidedLesson::Artifact));
	auto* Source = NewObject<UUserPersistData>();
	Source->GuidedTutorial = P;
	TArray<uint8> Bytes;
	{ FMemoryWriter W(Bytes); FObjectAndNameAsStringProxyArchive A(W,false); A.ArIsSaveGame = true; A.ArNoDelta = true; Source->Serialize(A); }
	auto* Loaded = NewObject<UUserPersistData>();
	{ FMemoryReader R(Bytes); FObjectAndNameAsStringProxyArchive A(R,false); A.ArIsSaveGame = true; A.ArNoDelta = true; Loaded->Serialize(A); }
	TestTrue(TEXT("Interrupted systems activity survives save and load"), Loaded->GuidedTutorial.SystemsActive);
	Source->GuidedTutorial.FinishSystems(); Bytes.Reset();
	{ FMemoryWriter W(Bytes); FObjectAndNameAsStringProxyArchive A(W,false); A.ArIsSaveGame = true; A.ArNoDelta = true; Source->Serialize(A); }
	{ FMemoryReader R(Bytes); FObjectAndNameAsStringProxyArchive A(R,false); A.ArIsSaveGame = true; A.ArNoDelta = true; Loaded->Serialize(A); }
	TestTrue(TEXT("Completed systems survive save and load"), Loaded->GuidedTutorial.SystemsComplete && !Loaded->GuidedTutorial.SystemsActive);
	Loaded->GuidedTutorial.ResumeInNewBattle();
	TestEqual(TEXT("Reloading the handoff retains the combat exercise"), Loaded->GuidedTutorial.Stage, EGuidedStage::OpenSkills);
	TestFalse(TEXT("Reloading the handoff never repeats information"), Loaded->GuidedTutorial.BeginSystems());
	Loaded->GuidedTutorial.CoreComplete = true;
	Loaded->GuidedTutorial.FinishSystems();
	TestEqual(TEXT("Previously completed combat is preserved for existing profiles"), Loaded->GuidedTutorial.Stage, EGuidedStage::LessonMenu);
	return !HasAnyErrors();
}
#endif
