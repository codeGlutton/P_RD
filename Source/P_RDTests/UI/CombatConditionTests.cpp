#include "Misc/AutomationTest.h"
#include "UI/Combat/CombatConditionWidget.h"
#include "UI/Combat/CombatLayoutHUDWidget.h"
#include "UI/Combat/CombatUIModel.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"
#include "Components/CanvasPanelSlot.h"
#include "Editor.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Slate/WidgetRenderer.h"
#include "ImageUtils.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "RHI.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatConditionSummaryTest,"P_RD.UI.CombatHUD.ConditionSummary",
	EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCombatConditionSummaryTest::RunTest(const FString&)
{
	UWorld* World=GEditor->GetEditorWorldContext().World();
	UClass* Class=LoadClass<UCombatLayoutHUDWidget>(nullptr,TEXT("/Game/UI/CombatLayouts/WBP_CombatHUD04.WBP_CombatHUD04_C"));
	if(!TestNotNull(TEXT("HUD class"),Class))return false;
	UCombatLayoutHUDWidget* HUD=CreateWidget<UCombatLayoutHUDWidget>(World,Class);
	HUD->SetVisibility(ESlateVisibility::SelfHitTestInvisible);HUD->TakeWidget();
	UCombatUIModel* Model=NewObject<UCombatUIModel>(HUD);HUD->BindUIModel(Model);
	FUnitUI Ally;Ally.mUnitId=1;Ally.mIsPlayer=true;Ally.mName=FText::FromString(TEXT("기사"));Ally.mHP=80;Ally.mMaxHP=100;
	Ally.mTurnPortrait=LoadObject<UTexture2D>(nullptr,TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Mercenaries/T_MB_HireIcon_Knight.T_MB_HireIcon_Knight"));
	Ally.mActionPoints=5;Ally.mMaxActionPoints=5;
	FUnitUI Enemy=Ally;Enemy.mUnitId=2;Enemy.mIsPlayer=false;
	Model->SetUnitUIs({Ally,Enemy});
	FTurnUI Turn;Turn.mCurrentUnitId=1;Turn.mTurnOrderUnitIds={1,2};Model->SetTurnUI(Turn);
	auto* Badge=Cast<UCombatConditionWidget>(HUD->GetWidgetFromName(TEXT("AllyCondition")));
	auto* EnemyBadge=Cast<UCombatConditionWidget>(HUD->GetWidgetFromName(TEXT("EnemyCondition")));
	if(!TestNotNull(TEXT("Ally face"),Badge)||!TestNotNull(TEXT("Enemy face"),EnemyBadge))return false;
	const FString Folder=FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("UI/CombatCondition"));IFileManager::Get().MakeDirectory(*Folder,true);
	FWidgetRenderer Renderer(true,true);Renderer.SetIsPrepassNeeded(true);
	for(int32 I=0;I<4;++I)
	{
		Ally.mCombatCondition=static_cast<EUnitCombatCondition>(I);Model->SetUnitUIs({Ally,Enemy});
		TestEqual(TEXT("Snapshot updates face"),Badge->GetCondition(),Ally.mCombatCondition);
		TestEqual(TEXT("Face visible"),Badge->GetVisibility(),ESlateVisibility::Visible);
		TestFalse(TEXT("Condition has explanation"),UCombatConditionWidget::Describe(Ally.mCombatCondition).IsEmpty());
		if(!GUsingNullRHI)
		{
			HUD->WidgetTree->ForEachWidget([](UWidget* W){if(auto* Image=Cast<UImage>(W))if(auto* T=Cast<UTexture2D>(Image->GetBrush().GetResourceObject())){T->SetForceMipLevelsToBeResident(30);T->WaitForStreaming();}});
			UWidget* Panel=HUD->GetWidgetFromName(TEXT("AllyPanel"));
			const TSharedRef<SWidget> Slate=Panel->TakeWidget();
			for(int32 N=0;N<3;++N){Renderer.DrawWidget(Slate,FVector2D(168,550));FlushRenderingCommands();}
			auto* Target=Renderer.DrawWidget(Slate,FVector2D(168,550));FlushRenderingCommands();
			TArray<FColor> Pixels;TestTrue(TEXT("Read summary"),Target->GameThread_GetRenderTargetResource()->ReadPixels(Pixels));
			// The forehead must be filled, not just floating eye/mouth strokes.
			// This catches invalid rounded-box radii that make the entire disc disappear.
			if (Pixels.Num() == 168*550)
				TestTrue(TEXT("Condition disc remains opaque"),Pixels[99*168+136].A > 240);
			TArray64<uint8> Png;FImageUtils::PNGCompressImageArray(168,550,Pixels,Png);
			TestTrue(TEXT("Save summary"),FFileHelper::SaveArrayToFile(Png,*FPaths::Combine(Folder,FString::Printf(TEXT("Condition_%d.png"),I))));
		}
	}
	FCombatTargetUI Target;Target.mIsValid=true;Target.mUnitId=2;Enemy.mCombatCondition=EUnitCombatCondition::Bad;
	Model->SetUnitUIs({Ally,Enemy});Model->SetTarget(Target);
	TestEqual(TEXT("Selection uses selected unit"),EnemyBadge->GetCondition(),EUnitCombatCondition::Bad);
	TestEqual(TEXT("Previous unit badge hidden"),Badge->GetVisibility(),ESlateVisibility::Collapsed);
	Enemy.mCombatCondition=EUnitCombatCondition::Excellent;Model->SetUnitUIs({Ally,Enemy});
	TestEqual(TEXT("Selected unit updates live"),EnemyBadge->GetCondition(),EUnitCombatCondition::Excellent);
	Model->SetTarget(FCombatTargetUI());Ally.mHP=0;Model->SetUnitUIs({Ally,Enemy});
	TestEqual(TEXT("Dead current unit has no badge"),Badge->GetVisibility(),ESlateVisibility::Collapsed);
	Badge->SetCondition(EUnitCombatCondition::Count);TestEqual(TEXT("Invalid condition resets safely"),Badge->GetCondition(),EUnitCombatCondition::Normal);
	return !HasAnyErrors();
}
#endif
