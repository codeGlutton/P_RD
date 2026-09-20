#include "Editor.h"
#include "Engine/Texture2D.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"
#include "Components/Button.h"
#include "Engine/TextureRenderTarget2D.h"
#include "HAL/FileManager.h"
#include "ImageUtils.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "RHI.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"
#include "Singleton/InstanceSubsystem/PersistentData.h"
#include "Slate/WidgetRenderer.h"
#include "Tutorial/FirstPlayProgress.h"
#include "UI/Combat/CombatLayoutHUDWidget.h"
#include "UI/Combat/CombatUIModel.h"
#include "UI/Tutorial/FirstPlayTutorialWidget.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/SOverlay.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFirstPlayProgressTest, "P_RD.Tutorial.Progress",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFirstPlayProgressTest::RunTest(const FString &)
{
	FFirstPlayProgress P;
	TestEqual(TEXT("New profile starts at welcome"), P.GetStep(), EFirstPlayStep::Welcome);
	P.WelcomeSeen = true;
	TestEqual(TEXT("Actual battle starts with overview"), P.GetStep(), EFirstPlayStep::Overview);
	TestFalse(TEXT("Enemy action ignored"), P.Record(EFirstPlayAction::Move, false, true));
	TestFalse(TEXT("Cancelled action ignored"), P.Record(EFirstPlayAction::Move, true, false));
	TestEqual(TEXT("Ignored actions cannot progress"), int32(P.Actions), 0);
	P.Record(EFirstPlayAction::Skill);
	P.Record(EFirstPlayAction::EndTurn);
	TestFalse(TEXT("Actions do not skip overview"), P.Completed);
	P.OverviewSeen = true;
	P.UpdateCompletion();
	TestEqual(TEXT("Out-of-order actions retained; movement remains"), P.GetStep(), EFirstPlayStep::Move);
	TestFalse(TEXT("Duplicate checkpoint ignored"), P.Record(EFirstPlayAction::Skill));
	P.Record(EFirstPlayAction::Move);
	TestTrue(TEXT("All actions complete tutorial"), P.Completed);
	P = FFirstPlayProgress();
	P.Skip();
	TestTrue(TEXT("Skip is persisted completion"), P.Completed && P.Skipped);
	TestFalse(TEXT("Later events cannot undo skip"), P.Record(EFirstPlayAction::Move));
	P = FFirstPlayProgress();
	TestEqual(TEXT("Replay starts over"), P.GetStep(), EFirstPlayStep::Welcome);
	return !HasAnyErrors();
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFirstPlayInfoTest, "P_RD.Tutorial.InfoProgress",
 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFirstPlayInfoTest::RunTest(const FString&)
{
 FFirstPlayProgress P;
 P.Completed=true;
 TestTrue(TEXT("Completed combat still receives unread info"),P.NeedsInfo(EFirstPlayStep::MercenaryInfo));
 P.AcknowledgeInfo(EFirstPlayStep::MercenaryInfo);
 TestFalse(TEXT("Acknowledged topic no longer repeats"),P.NeedsInfo(EFirstPlayStep::MercenaryInfo));
 TestTrue(TEXT("Other topics remain unread"),P.NeedsInfo(EFirstPlayStep::InventoryInfo));
 TestFalse(TEXT("Combat step is not contextual help"),P.NeedsInfo(EFirstPlayStep::Move));
 TestTrue(TEXT("Reading info preserves combat completion"),P.Completed);
 P.Skip();TestFalse(TEXT("Global skip suppresses contextual help"),P.NeedsInfo(EFirstPlayStep::MonsterInfo));
 P=FFirstPlayProgress();TestTrue(TEXT("Replay resets info"),P.NeedsInfo(EFirstPlayStep::MercenaryInfo));
 return !HasAnyErrors();
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFirstPlaySaveTest, "P_RD.Tutorial.SaveRoundTrip",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFirstPlaySaveTest::RunTest(const FString &)
{
	UUserPersistData *Source = NewObject<UUserPersistData>();
	Source->MakeUser(FText::FromString(TEXT("Tutorial test")));
	Source->TutorialProgress.WelcomeSeen = true;
	Source->TutorialProgress.OverviewSeen = true;
	Source->TutorialProgress.Record(EFirstPlayAction::Move);
 Source->TutorialProgress.AcknowledgeInfo(EFirstPlayStep::MercenaryInfo);
	TArray<uint8> Bytes;
	{
		FMemoryWriter Writer(Bytes);
		FObjectAndNameAsStringProxyArchive Ar(Writer, false);
		Ar.ArIsSaveGame = true;
		Ar.ArNoDelta = true;
		Source->Serialize(Ar);
	}
	UUserPersistData *Loaded = NewObject<UUserPersistData>();
	{
		FMemoryReader Reader(Bytes);
		FObjectAndNameAsStringProxyArchive Ar(Reader, false);
		Ar.ArIsSaveGame = true;
		Ar.ArNoDelta = true;
		Loaded->Serialize(Ar);
	}
	TestEqual(TEXT("Resume remaining skill after load"), Loaded->TutorialProgress.GetStep(), EFirstPlayStep::Skill);
	TestEqual(TEXT("Existing profile survives"), Loaded->GetUserName().ToString(), Source->GetUserName().ToString());
	TestFalse(TEXT("Info acknowledgement survives loading"), Loaded->TutorialProgress.NeedsInfo(EFirstPlayStep::MercenaryInfo));
 TestTrue(TEXT("Unread monster guide survives loading"), Loaded->TutorialProgress.NeedsInfo(EFirstPlayStep::MonsterInfo));
 Loaded->ClearUser();
 TestTrue(TEXT("Reset restores contextual help"), Loaded->TutorialProgress.NeedsInfo(EFirstPlayStep::MercenaryInfo));
	TestEqual(TEXT("Profile reset resets tutorial"), Loaded->TutorialProgress.GetStep(), EFirstPlayStep::Welcome);
	// Uses only memory archives; the player's real save slots are never touched.
	return !HasAnyErrors();
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFirstPlayCaptureTest, "P_RD.Tutorial.Capture",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFirstPlayCaptureTest::RunTest(const FString &)
{
	if (GUsingNullRHI)
	{
		AddError(TEXT("Tutorial capture requires rendering"));
		return false;
	}
	UWorld *World = GEditor->GetEditorWorldContext().World();
	UClass *HUDClass =
	    LoadClass<UCombatLayoutHUDWidget>(nullptr, TEXT("/Game/UI/CombatLayouts/WBP_CombatHUD04.WBP_CombatHUD04_C"));
	if (!TestNotNull(TEXT("Real HUD class"), HUDClass))
		return false;
	auto *HUD = CreateWidget<UCombatLayoutHUDWidget>(World, HUDClass);
	HUD->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	auto *Model = NewObject<UCombatUIModel>(HUD);
	HUD->TakeWidget();
	HUD->BindUIModel(Model);
	FUnitUI Unit;
	Unit.mUnitId = 1;
	Unit.mIsPlayer = true;
	Unit.mName = FText::FromString(TEXT("기사"));
	Unit.mHP = 80;
	Unit.mMaxHP = 100;
	Unit.mActionPoints = 5;
	Unit.mMaxActionPoints = 5;
 Unit.mMovementPoint = 5; Unit.mMaxMovementPoint = 5;
 Unit.mPortrait = LoadObject<UTexture2D>(nullptr,TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Mercenaries/T_MB_HireIcon_Knight.T_MB_HireIcon_Knight"));
 Unit.mTurnPortrait = Unit.mPortrait;
 TArray<FSkillUI> Skills;
 for (int32 I=0; I<5; ++I) { FSkillUI Skill; Skill.mSkillIndex=I; Skill.mIsUsable=true; Skill.mIcon=LoadObject<UTexture2D>(nullptr,TEXT("/Game/SVN/OutSideAsset/AICreation/UI/CombatHUD/SkillIcons/T_SkillIcon_Slash.T_SkillIcon_Slash")); Skills.Add(Skill); }
 Model->SetSkillUIs(Skills);
	Model->SetUnitUIs({Unit});
	FTurnUI Turn;
	Turn.mRound=1;
	Turn.mCurrentUnitId = 1;
	Turn.mTurnOrderUnitIds = {1};
	Model->SetTurnUI(Turn);
 Model->OnBeginAnyTurn.Broadcast(nullptr);
 if (auto* Button=Cast<UButton>(HUD->GetWidgetFromName(TEXT("SkillToggleButton")))) Button->OnClicked.Broadcast();
 HUD->WidgetTree->ForEachWidget([](UWidget* Widget){
  if(auto* Image=Cast<UImage>(Widget)) if(auto* Texture=Cast<UTexture2D>(Image->GetBrush().GetResourceObject())) {Texture->SetForceMipLevelsToBeResident(30);Texture->WaitForStreaming();}
 });
	auto *Guide = CreateWidget<UFirstPlayTutorialWidget>(World);
	Guide->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	auto Root = SNew(SOverlay) + SOverlay::Slot()[HUD->TakeWidget()] + SOverlay::Slot()[Guide->TakeWidget()];
	auto Mobile =
	    SNew(SScaleBox).Stretch(EStretch::ScaleToFit)[SNew(SBox).WidthOverride(1920).HeightOverride(1080)[Root]];
	const FString Folder = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("UI/Tutorial"));
	IFileManager::Get().MakeDirectory(*Folder, true);
	FWidgetRenderer Renderer(true, true);
	Renderer.SetIsPrepassNeeded(true);
	for (int32 I = 0; I <= 8; ++I)
	{
		auto Step = static_cast<EFirstPlayStep>(I);
 if (I==6) {
  if (auto* Button=Cast<UButton>(HUD->GetWidgetFromName(TEXT("MenuButton_1")))) Button->OnClicked.Broadcast();
 }
 if (I==7) {
  if (auto* Button=Cast<UButton>(HUD->GetWidgetFromName(TEXT("MenuButton_2")))) Button->OnClicked.Broadcast();
  if (auto* Monster=HUD->GetMonsterTabWidgetForTest()) {Root->ClearChildren();Root->AddSlot()[HUD->TakeWidget()];Root->AddSlot()[Monster->TakeWidget()];Root->AddSlot()[Guide->TakeWidget()];}
 }
 if (I==8) {
  Root->ClearChildren();Root->AddSlot()[HUD->TakeWidget()];Root->AddSlot()[Guide->TakeWidget()];
  if (auto* Button=Cast<UButton>(HUD->GetWidgetFromName(TEXT("MenuButton_1")))) Button->OnClicked.Broadcast();
  HUD->ShowMercenaryInventoryForTest();
 }
		Guide->Present(Step, I == 0);
		TestFalse(TEXT("Step heading exists"), Guide->Heading(Step).IsEmpty());
		for (int32 N = 0; N < 3; ++N)
		{
			Renderer.DrawWidget(Mobile, FVector2D(1280, 720));
			FlushRenderingCommands();
		}
		auto *Target = Renderer.DrawWidget(Mobile, FVector2D(1280, 720));
		FlushRenderingCommands();
		TArray<FColor> Pixels;
		TestTrue(TEXT("Read tutorial"), Target->GameThread_GetRenderTargetResource()->ReadPixels(Pixels));
		TArray64<uint8> Png;
		FImageUtils::PNGCompressImageArray(1280, 720, Pixels, Png);
		TestTrue(TEXT("Save tutorial capture"),
		         FFileHelper::SaveArrayToFile(Png, *FPaths::Combine(Folder, FString::Printf(TEXT("Step_%d.png"), I))));
	}
	return !HasAnyErrors();
}
#endif
