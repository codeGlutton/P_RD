#include "Misc/AutomationTest.h"
#include "Editor.h"
#include "UI/TitleMenuWidget.h"
#include "UI/StageVictory/FinalRunVictoryWidget.h"
#include "Setting/GamePlaySettings.h"
#include "Internationalization/Internationalization.h"
#include "Internationalization/Culture.h"
#include "Internationalization/TextLocalizationManager.h"
#include "DataAsset/ArtifactData/StaticArtifactData.h"
#include "DataAsset/EquipmentData/StaticEquipmentData.h"
#include "UI/Combat/SkillDetailUIBuilder.h"
#include "UI/Combat/SkillDetailOverlayPresenter.h"
#include "UI/Combat/CombatLayoutHUDWidget.h"
#include "UI/Combat/CombatUIModel.h"
#include "UI/Reward/RewardConcept03Widget.h"
#include "UI/Reward/RewardUIModel.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"
#include "Components/Button.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInterface.h"
#include "RHI.h"
#include "Engine/TextureRenderTarget2D.h"
#include "AssetCompilingManager.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "DataAsset/UnitSpawnData/StaticEnemyUnitSpawnData.h"
#include "Slate/WidgetRenderer.h"
#include "ImageUtils.h"
#include "Misc/FileHelper.h"
#include "Serialization/BufferArchive.h"

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
namespace FeedbackTests
{
void Capture(UUserWidget *Widget, const TCHAR *Name)
{
	if (!Widget || GUsingNullRHI)
		return;
	const auto Slate = Widget->TakeWidget();
	Slate->SetCanTick(false);
	FAssetCompilingManager::Get().FinishAllCompilation();
	TArray<UWidget *> Children;
	Widget->WidgetTree->GetAllWidgets(Children);
	for (UWidget *Child : Children)
		if (auto *Image = Cast<UImage>(Child))
			if (auto *Texture = Cast<UTexture2D>(Image->GetBrush().GetResourceObject()))
			{
				Texture->SetForceMipLevelsToBeResident(30.f);
				Texture->WaitForStreaming();
			}
	FWidgetRenderer Renderer(true, false);
	auto *Target = Renderer.DrawWidget(Slate, FVector2D(1920, 1080));
	for (int32 Frame = 0; Frame < 3; ++Frame)
		Renderer.DrawWidget(Target, Slate, FVector2D(1920, 1080), 0.016f);
	// Offscreen rendering paints widgets without the application's regular tick.
 for(auto* Child:Children) if(auto* Scroll=Cast<UScrollBox>(Child))
  if(auto SlateScroll=Scroll->GetCachedWidget()) SlateScroll->Tick(SlateScroll->GetPaintSpaceGeometry(),0,0.016f);
 Renderer.DrawWidget(Target,Slate,FVector2D(1920,1080),0.016f);
 FBufferArchive Bytes;
	FImageUtils::ExportRenderTarget2DAsPNG(Target, Bytes);
	const FString Dir = FPaths::ProjectSavedDir() / TEXT("UI/Feedback");
	IFileManager::Get().MakeDirectory(*Dir, true);
	FFileHelper::SaveArrayToFile(Bytes, *(Dir / FString(Name)));
}
} // namespace FeedbackTests
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFeedbackDisplayTest, "P_RD.UI.Feedback.DisplayAndScrolling",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFeedbackDisplayTest::RunTest(const FString &)
{
	auto *World = GEditor->GetEditorWorldContext().World();
	auto *Data = NewObject<UStaticArtifactData>();
	Data->mDescription = FText::FromString(TEXT("Passive: heal 5 HP. Stat: maximum HP +20."));
	FCombatArtifactUI Artifact;
	SkillDetailUIBuilder::FillFromArtifactData(Data, Artifact);
	TestEqual(TEXT("Authored passive and stat description survives DTO"),
	          Artifact.mEffectDescriptions[0].ToString(), Data->mDescription.ToString());
	// An intentionally longer-than-viewport body catches the clipping regression.
	FString Long;
	for (int32 i = 0; i < 45; ++i)
		Long += FString::Printf(
		    TEXT("Effect %d: Increase maximum HP by 20 and heal 5 HP at the start of combat.\n"), i);
	Long += TEXT("END OF DESCRIPTION");
	Artifact.mName = FText::FromString(TEXT("Long artifact description"));
	Artifact.mEffectDescriptions = {FText::FromString(Long)};
	auto *Class = LoadClass<UUserWidget>(
	    nullptr, TEXT("/Game/UI/CombatDetail/WBP_CombatDetailOverlay.WBP_CombatDetailOverlay_C"));
	if (!Class)
		Class = LoadClass<UUserWidget>(nullptr,
		                               TEXT("/Game/UI/WBP_CombatDetailOverlay.WBP_CombatDetailOverlay_C"));
	if (!TestNotNull(TEXT("Detail class"), Class))
		return false;
	auto *Presenter = NewObject<USkillDetailOverlayPresenter>();
	Presenter->Initialize(World, Class, nullptr, 70);
	Presenter->PresentArtifact(Artifact);
	auto *Overlay = Presenter->GetOverlayWidget();
	if (!TestNotNull(TEXT("Actual artifact overlay"), Overlay))
		return false;
	const auto OverlaySlate = Overlay->TakeWidget();
	if (auto *Icon = Presenter->GetIconImage())
		Icon->SetBrushFromTexture(LoadObject<UTexture2D>(
		    nullptr, TEXT("/Game/SVN/OutSideAsset/AICreation/UI/BindingRepair_20260909/"
		                  "T_Artifact_A018_PredatorsHeart.T_Artifact_A018_PredatorsHeart")));
	FeedbackTests::Capture(Overlay, TEXT("artifact-long.png"));
	UScrollBox *BodyScroll = Presenter->GetArtifactDescriptionScroll();
	TArray<UWidget *> Children;
	Overlay->WidgetTree->GetAllWidgets(Children);
	for (auto *Child : Children)
		if (auto *Scroll = Cast<UScrollBox>(Child))
			if (Scroll->IsVisible() && Scroll->GetName().Contains(TEXT("Artifact")))
				BodyScroll = Scroll;
	if (TestNotNull(TEXT("Artifact scroll"), BodyScroll) && !GUsingNullRHI)
	{
		TestTrue(TEXT("Description exceeds viewport"), BodyScroll->GetScrollOffsetOfEnd() > 0.f);
		BodyScroll->ScrollToEnd();
		FeedbackTests::Capture(Overlay, TEXT("artifact-long-end.png"));
		TestTrue(TEXT("Can scroll to final lines"), BodyScroll->GetScrollOffset() > 0.f);
	}
	FUnitDetailEquipmentUI Equipment;
	Equipment.mName = FText::FromString(TEXT("Bone Hardening"));
	Equipment.mDescription = FText::FromString(TEXT("When hit, gain 5 Armor."));
	Presenter->PresentEquipment(Equipment);
	TestFalse(TEXT("Enemy equipment must not claim party-wide scope"),
	          Presenter->GetSubtitleText()->GetText().ToString().Contains(TEXT("파티")));
	Presenter->Teardown();
	auto *RewardClass = LoadClass<URewardConcept03Widget>(
	    nullptr, TEXT("/Game/UI/RewardConcept03New/WBP_RewardConcept03_New.WBP_RewardConcept03_New_C"));
	auto *Reward = CreateWidget<URewardConcept03Widget>(World, RewardClass);
	const auto RewardSlate = Reward->TakeWidget();
	Reward->SetRewardPresentationManualTick(true);
	auto *Rewards = NewObject<URewardUIModel>(Reward);
	FRewardChoiceUI Choice;
	Choice.mChoiceIndex = 0;
	Choice.mName = Artifact.mName;
	Choice.mDescription = FText::FromString(Long);
	Choice.mIcon = LoadObject<UTexture2D>(
	    nullptr, TEXT("/Game/SVN/OutSideAsset/AICreation/UI/BindingRepair_20260909/"
	                  "T_Artifact_A018_PredatorsHeart.T_Artifact_A018_PredatorsHeart"));
	Choice.mSourceAssetId = FPrimaryAssetId(TEXT("Artifact"), TEXT("LongArtifact"));
	Rewards->SetRewardChoices({Choice});
	Reward->BindUIModel(Rewards);
	Reward->ResetRewardFlow();
	for (int32 Step = 0; Step < 8 && Reward->GetCurrentStepIndex() < 3; ++Step)
	{
		if (Reward->GetCurrentStepIndex() == 1)
			Reward->OpenRewardChest();
		Reward->SkipRewardPresentation();
		Reward->AdvanceRewardFlow();
	}
	Reward->SkipRewardPresentation();
	FeedbackTests::Capture(Reward, TEXT("reward-long.png"));
	auto *CardScroll = Cast<UScrollBox>(Reward->GetWidgetFromName(TEXT("NewChoiceDescriptionScroll_0")));
    TestTrue(TEXT("Reward cards defer full description to detail"), !CardScroll || !CardScroll->IsVisible());
    Reward->SelectArtifact(0);

	auto *RewardOverlay = Reward->GetArtifactDetailOverlayForTest();
	if (TestNotNull(TEXT("Reward opens common detail overlay"), RewardOverlay))
		TestTrue(TEXT("Reward detail preserves full text"),
		         Cast<UTextBlock>(RewardOverlay->GetWidgetFromName(TEXT("DetailBodyText")))
		             ->GetText()
		             .ToString()
		             .Contains(TEXT("END OF DESCRIPTION")));

	Reward->HideArtifactDetails();
	auto *Chains = LoadObject<UStaticArtifactData>(
	    nullptr,
	    TEXT(
	        "/Game/BP/DataAsset/Artifact/DA_Artifact_A035_ChainsOfBinding.DA_Artifact_A035_ChainsOfBinding"));
	if (TestNotNull(TEXT("Reported artifact loads"), Chains))
	{
		Choice.mName = Chains->mName;
		Choice.mDescription = Chains->GetDisplayDescription();
		Choice.mIcon = Chains->mIcon.LoadSynchronous();
		Rewards->SetRewardChoices({Choice});
		FeedbackTests::Capture(Reward, TEXT("reward-chains.png"));
		Reward->ShowArtifactDetails(0);
		FeedbackTests::Capture(Reward->GetArtifactDetailOverlayForTest(), TEXT("reward-chains-detail.png"));
	}
	Reward->HideArtifactDetails();
	Reward->RemoveFromParent();

	auto *HUDClass = LoadClass<UCombatLayoutHUDWidget>(
	    nullptr, TEXT("/Game/UI/CombatLayouts/WBP_CombatHUD04.WBP_CombatHUD04_C"));
	auto *HUD = CreateWidget<UCombatLayoutHUDWidget>(World, HUDClass);
	HUD->TakeWidget();
	auto *Model = NewObject<UCombatUIModel>(HUD);
	HUD->BindUIModel(Model);
	auto *Equipped = LoadObject<UStaticEquipmentData>(
	    nullptr,
	    TEXT("/Game/BP/DataAsset/Equipment/DA_Equipment_A004_BoneHardening.DA_Equipment_A004_BoneHardening"));
	if (TestNotNull(TEXT("Actual equipment asset"), Equipped))
	{
		Equipment.mName = Equipped->mName;
		Equipment.mDescription = Equipped->mDescription;
		Equipment.mIcon = Equipped->mIcon.LoadSynchronous();
		TestNotNull(TEXT("Equipment icon"), Equipment.mIcon.Get());
	}
	FUnitUI Unit;
	Unit.mUnitId = 900;
	Unit.mIsPlayer = false;
	Unit.mName = FText::FromString(TEXT("Skeleton"));
	Unit.mHP = Unit.mMaxHP = 50;
	Model->SetUnitUIs({Unit});
	auto *Menu = Cast<UButton>(HUD->GetWidgetFromName(TEXT("MenuButton_2")));
	if (!TestNotNull(TEXT("Monster menu"), Menu))
		return false;
	Menu->OnClicked.Broadcast();
	FUnitDetailUI Detail;
	Detail.mUnitId = 900;
	Detail.mName = Unit.mName;
	Detail.mEquipment.Add(Equipment);
	Model->SetUnitDetail(Detail);
	auto *Tab = HUD->GetMonsterTabWidgetForTest();
	if (!TestNotNull(TEXT("Monster tab"), Tab))
		return false;
	const auto TabSlate = Tab->TakeWidget();
	auto *Button = Cast<UButton>(Tab->GetWidgetFromName(TEXT("MonsterEquipmentButton_0")));
	if (TestNotNull(TEXT("Equipment button"), Button))
	{
		TestTrue(TEXT("Equipped item visible"), Button->IsVisible());
		FeedbackTests::Capture(Tab, TEXT("enemy-equipment.png"));
		Button->OnClicked.Broadcast();
		TestTrue(TEXT("Click opens equipment details"), HUD->IsDetailOverlayShown());
		Detail.mEquipment.Reset();
		Model->SetUnitDetail(Detail);
		TestFalse(TEXT("Removed equipment disappears"), Button->IsVisible());
		Detail.mUnitId = 901;
		Detail.mEquipment.Add(Equipment);
		Model->SetUnitDetail(Detail);
		TestFalse(TEXT("Unrelated unit snapshot cannot restore old equipment"), Button->IsVisible());
	}
	HUD->RemoveFromParent();
	return !HasAnyErrors();
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFeedbackTitleTest, "P_RD.UI.Feedback.TitleLanguage",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFeedbackTitleTest::RunTest(const FString &)
{
	auto *World = GEditor->GetEditorWorldContext().World();
	auto *Class = LoadClass<UTitleMenuWidget>(nullptr, TEXT("/Game/UI/WBP_TitleMenu.WBP_TitleMenu_C"));
	auto *Title = CreateWidget<UTitleMenuWidget>(World, Class);
	Title->TakeWidget();
	const FString Original = FInternationalization::Get().GetCurrentCulture()->GetName();
	const auto *Settings = GetDefault<UGamePlaySettings>();
	for (const FString Culture : {FString(TEXT("en")), FString(TEXT("ko")), FString(TEXT("en"))})
	{
		FInternationalization::Get().SetCurrentCulture(Culture);
		FTextLocalizationManager::Get().WaitForAsyncTasks();
		Title->RefreshLocalizedTitleLogo();
		auto *Logo = Cast<UImage>(Title->GetWidgetFromName(TEXT("TitleLogoImage__base_16_9")));
		if (!Logo)
			Logo = Cast<UImage>(Title->GetWidgetFromName(TEXT("TitleLogoImage")));
		UObject *Texture =
		    Culture == TEXT("ko")
		        ? static_cast<UObject *>(Settings->mTitleLogoTexture.LoadSynchronous())
		        : static_cast<UObject *>(Settings->mTitleLogoEnglishMaterial.LoadSynchronous());
		TestNotNull(TEXT("Localized logo texture cooks/loads"), Texture);
		if (TestNotNull(TEXT("Logo widget"), Logo))
			TestTrue(TEXT("Language switches actual logo brush"),
			         Logo->GetBrush().GetResourceObject() == Texture);
		FeedbackTests::Capture(Title, *FString::Printf(TEXT("title-%s.png"), *Culture));
	}
	FInternationalization::Get().SetCurrentCulture(Original);
	Title->RemoveFromParent();
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFeedbackAssetLocalizationTest,
                                 "P_RD.UI.Feedback.ItemLocalizationAndRatAssets",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFeedbackAssetLocalizationTest::RunTest(const FString &)
{
	auto *Rat = LoadObject<UStaticEnemyUnitSpawnData>(
	    nullptr, TEXT("/Game/BP/DataAsset/Unit/EnemyUnit/Stage2/DA_RatUnit.DA_RatUnit"));
	if (TestNotNull(TEXT("Rat data"), Rat))
		for (const auto &Ref : {Rat->mIcon, Rat->mPortrait, Rat->mShortCut})
		{
			TestNotNull(TEXT("Rat image loads"), Ref.LoadSynchronous());
			TestTrue(TEXT("Rat uses rat art instead of skeleton/mimic"),
			         Ref.ToString().Contains(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Feedback_20260915/T_Rat_")));
		}
	auto &Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
	TArray<FAssetData> Items;
	for (const FName Path :
	     {FName(TEXT("/Game/BP/DataAsset/Artifact")), FName(TEXT("/Game/BP/DataAsset/Equipment"))})
	{
		TArray<FAssetData> Found;
		Registry.GetAssetsByPath(Path, Found, true);
		Items.Append(Found);
	}
	auto HasKorean = [](const FString &Text) {
		for (TCHAR C : Text)
			if (C >= 0xAC00 && C <= 0xD7A3)
				return true;
		return false;
	};
	const FString Original = FInternationalization::Get().GetCurrentCulture()->GetName();
	for (const FString Culture : {FString(TEXT("en")), FString(TEXT("ko")), FString(TEXT("en"))})
	{
		FInternationalization::Get().SetCurrentCulture(Culture);
		auto &Loc = FTextLocalizationManager::Get();
		Loc.WaitForAsyncTasks();
		Loc.UpdateFromLocalizationResource(FPaths::ProjectContentDir() / TEXT("Localization/Game") / Culture /
		                                   TEXT("Game.locres"));
		int32 Count = 0;
		for (const auto &Entry : Items)
		{
			if (Entry.AssetName.ToString().Contains(TEXT("Test")))
				continue;
			UObject *Asset = Entry.GetAsset();
			TArray<FText> Texts;
			if (auto *Artifact = Cast<UStaticArtifactData>(Asset))
			{
				Texts = {Artifact->mName, Artifact->GetDisplayDescription()};
				TestFalse(TEXT("Production artifact has authored description"),
				          Artifact->mDescription.IsEmpty());
			}
			if (auto *Equipment = Cast<UStaticEquipmentData>(Asset))
			{
				Texts = {Equipment->mName, Equipment->mDescription};
				TestNotNull(TEXT("Production enemy equipment icon"), Equipment->mIcon.LoadSynchronous());
			}
			for (const FText &Text : Texts)
			{
				const FString *Source = FTextInspector::GetSourceString(Text);
				if (Source && HasKorean(*Source) && HasKorean(Text.ToString()) != (Culture == TEXT("ko")))
					AddError(FString::Printf(TEXT("%s %s: %s"), *Culture, *Entry.AssetName.ToString(),
					                         *Text.ToString()));
			}
			if (!Texts.IsEmpty())
				++Count;
		}
		TestTrue(TEXT("Production artifacts and equipment inspected"), Count > 30);
	}
	FInternationalization::Get().SetCurrentCulture(Original);
	FTextLocalizationManager::Get().WaitForAsyncTasks();
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatExpandedListsTest, "P_RD.UI.Feedback.ExpandedLists",
 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCombatExpandedListsTest::RunTest(const FString&)
{
 auto* World=GEditor->GetEditorWorldContext().World();
 auto* Class=LoadClass<UCombatLayoutHUDWidget>(nullptr,TEXT("/Game/UI/CombatLayouts/WBP_CombatHUD04.WBP_CombatHUD04_C"));
 auto* HUD=CreateWidget<UCombatLayoutHUDWidget>(World,Class);const auto HUDSlate=HUD->TakeWidget();
 auto* Model=NewObject<UCombatUIModel>(HUD);HUD->BindUIModel(Model);
 TArray<FUnitUI> Units;
 auto* Portrait=LoadObject<UTexture2D>(nullptr,TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Feedback_20260915/T_Rat_Head.T_Rat_Head"));
 for(int32 I=0;I<12;++I)
 {
  FUnitUI U;U.mUnitId=100+I;U.mIsPlayer=false;U.mName=FText::FromString(FString::Printf(TEXT("Rat %02d"),I+1));
  U.mHP=U.mMaxHP=50;U.mPortrait=U.mTurnPortrait=Portrait;Units.Add(U);
 }
 Model->SetUnitUIs(Units);
 CastChecked<UButton>(HUD->GetWidgetFromName(TEXT("MenuButton_2")))->OnClicked.Broadcast();
 auto* Tab=HUD->GetMonsterTabWidgetForTest();
 if(!TestNotNull(TEXT("Enemy tab"),Tab)) return false;
 const auto TabSlate=Tab->TakeWidget();
 auto* Scroll=Cast<UScrollBox>(Tab->GetWidgetFromName(TEXT("MonsterListScroll")));
 TestNotNull(TEXT("Enemy scroll created"),Scroll);
 auto* Last=Cast<UButton>(Tab->GetWidgetFromName(TEXT("MonsterRowButton_11")));
 if(TestNotNull(TEXT("Twelfth enemy button exists"),Last))
 {
  Last->OnClicked.Broadcast();
  TestEqual(TEXT("Last enemy selection uses correct identity"),CastChecked<UTextBlock>(Tab->GetWidgetFromName(TEXT("MonsterDetailNameText")))->GetText().ToString(),FString(TEXT("Rat 12")));
  Units.RemoveAt(0);Model->SetUnitUIs(Units);
  TestEqual(TEXT("Selected identity survives preceding enemy death"),CastChecked<UTextBlock>(Tab->GetWidgetFromName(TEXT("MonsterDetailNameText")))->GetText().ToString(),FString(TEXT("Rat 12")));
 }
 FeedbackTests::Capture(Tab,TEXT("monster-list.png"));
 if(Scroll && !GUsingNullRHI)
 {
  AddInfo(FString::Printf(TEXT("SCROLL monster content=%s view=%s paint=%s end=%f"),*Scroll->GetChildAt(0)->GetDesiredSize().ToString(),*Scroll->GetDesiredSize().ToString(),*Scroll->GetCachedWidget()->GetPaintSpaceGeometry().GetLocalSize().ToString(),Scroll->GetScrollOffsetOfEnd()));
  TestTrue(TEXT("Enemy list exceeds viewport"),Scroll->GetScrollOffsetOfEnd()>0);
  Scroll->ScrollToEnd();FeedbackTests::Capture(Tab,TEXT("monster-list-end.png"));
  TestTrue(TEXT("Last enemy scroll reachable"),Scroll->GetScrollOffset()>0);
 }
 CastChecked<UButton>(Tab->GetWidgetFromName(TEXT("MonsterBackButton")))->OnClicked.Broadcast();
 FPlayerMetaUI Meta;Meta.mGold=123;
 for(int32 I=0;I<23;++I)
 {
  FCombatArtifactUI A;A.mName=FText::FromString(FString::Printf(TEXT("Artifact %02d"),I+1));A.mIcon=Portrait;
  A.mEffectDescriptions={FText::FromString(TEXT("Unique item description"))};Meta.mArtifacts.Add(A);
 }
 Model->SetPlayerMeta(Meta);
 CastChecked<UButton>(HUD->GetWidgetFromName(TEXT("MenuButton_1")))->OnClicked.Broadcast();
 HUD->ShowMercenaryInventoryForTest();
 auto* ArtifactScroll=Cast<UScrollBox>(HUD->GetWidgetFromName(TEXT("InventoryArtifactScroll")));
 TestNotNull(TEXT("Artifact scroll created"),ArtifactScroll);
 auto* LastArtifact=Cast<UButton>(HUD->GetWidgetFromName(TEXT("MercenaryInventoryArtifactButton_22")));
 if(TestNotNull(TEXT("23rd artifact button exists"),LastArtifact))
 {
  LastArtifact->OnClicked.Broadcast();TestTrue(TEXT("Last artifact opens detail"),HUD->IsDetailOverlayShown());
  auto* Detail=HUD->GetDetailOverlayWidgetForTest();bool Found=false;
  if(Detail) Detail->WidgetTree->ForEachWidget([&](UWidget* W){if(auto* T=Cast<UTextBlock>(W)) Found|=T->GetText().ToString()==TEXT("Artifact 23");});
  TestTrue(TEXT("Detail belongs to selected last artifact"),Found);HUD->CloseDetailOverlayForTest();
 }
 HUD->OpenInventoryPanelForTest();
 FeedbackTests::Capture(HUD,TEXT("inventory-list.png"));
 if(ArtifactScroll && !GUsingNullRHI)
 {
  AddInfo(FString::Printf(TEXT("SCROLL artifact content=%s paint=%s"),*ArtifactScroll->GetChildAt(0)->GetDesiredSize().ToString(),*ArtifactScroll->GetCachedWidget()->GetPaintSpaceGeometry().GetLocalSize().ToString()));
  TestTrue(TEXT("Artifact list exceeds viewport"),ArtifactScroll->GetScrollOffsetOfEnd()>0);
  ArtifactScroll->ScrollToEnd();FeedbackTests::Capture(HUD,TEXT("inventory-list-end.png"));
  TestTrue(TEXT("Last artifact scroll reachable"),ArtifactScroll->GetScrollOffset()>0);
 }
 Meta.mArtifacts.SetNum(1);Model->SetPlayerMeta(Meta);
 if(LastArtifact) TestFalse(TEXT("Removed artifact cannot be clicked"),LastArtifact->IsVisible());
 Meta.mArtifacts.Reset();Model->SetPlayerMeta(Meta);
 auto* First=Cast<UButton>(HUD->GetWidgetFromName(TEXT("MercenaryInventoryArtifactButton_0")));
 if(First) TestFalse(TEXT("Empty inventory has no stale clickable slot"),First->IsVisible());
 HUD->RemoveFromParent();return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFinalVictoryTest, "P_RD.UI.Feedback.FinalVictory",
 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFinalVictoryTest::RunTest(const FString&)
{
 auto* World=GEditor->GetEditorWorldContext().World();
 auto* HUD=CreateWidget<UCombatLayoutHUDWidget>(World);HUD->TakeWidget();
 auto* Model=NewObject<UCombatUIModel>(HUD);HUD->BindUIModel(Model);
 FCombatResultUI Result;Result.mIsWin=true;Result.mIsClearStage=true;
 Model->SetCombatResultUI(Result);HUD->FinishRewardsForTest();
 TestNull(TEXT("Intermediate boss does not show final victory"),HUD->GetFinalVictoryForTest());
 Result.mIsLastStage=true;Model->SetCombatResultUI(Result);HUD->FinishRewardsForTest();
 auto* Victory=HUD->GetFinalVictoryForTest();
 if(TestNotNull(TEXT("Last boss reward completion shows final victory"),Victory))
 {
  int32 Calls=0;Victory->SetOnContinue(FSimpleDelegate::CreateLambda([&Calls]{++Calls;}));
  FeedbackTests::Capture(Victory,TEXT("final-victory.png"));
  auto* Button=CastChecked<UButton>(Victory->GetWidgetFromName(TEXT("FinalVictoryContinue")));
  Button->OnClicked.Broadcast();Button->OnClicked.Broadcast();
  TestEqual(TEXT("Repeated confirmation settles run only once"),Calls,1);
  Victory->RemoveFromParent();
 }
 HUD->RemoveFromParent();return !HasAnyErrors();
}
#endif
