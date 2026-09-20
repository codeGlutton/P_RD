#include "Misc/AutomationTest.h"

#include "Engine/Texture2D.h"
#include "UI/Combat/SkillCutInWidget.h"
#include "UI/Combat/CombatLayoutHUDWidget.h"
#include "Components/CanvasPanel.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "UI/Combat/CombatUIModel.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "Engine/LocalPlayer.h"
#include "Editor.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Slate/WidgetRenderer.h"
#include "ImageUtils.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "RHI.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatWorldHpVisibilityTest,
	"P_RD.UI.CombatHUD.WorldHpNotHiddenBySkillCards",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatWorldHpVisibilityTest::RunTest(const FString& Parameters)
{
	UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	if (!TestNotNull(TEXT("Editor world"), World)) return false;
	UClass* Class = LoadClass<UCombatLayoutHUDWidget>(nullptr,
		TEXT("/Game/UI/CombatLayouts/WBP_CombatHUD04.WBP_CombatHUD04_C"));
	if (!TestNotNull(TEXT("Combat HUD class"), Class)) return false;
	UCombatLayoutHUDWidget* HUD = CreateWidget<UCombatLayoutHUDWidget>(World, Class);
	if (!TestNotNull(TEXT("Combat HUD"), HUD)) return false;
	HUD->TakeWidget();
	// Isolate the visibility gate from world projection. A missing controller
	// leaves the last projected frame intact, as during input/view transitions.
	HUD->mUnitHpBars.Reset();
	FCombatUnitHpBarWidget& Bar = HUD->mUnitHpBars.AddDefaulted_GetRef();
	Bar.mRoot = CreateWidget<USkillCutInWidget>(World);
	Bar.mRoot->SetVisibility(ESlateVisibility::HitTestInvisible);
	HUD->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	HUD->mCommandsShown = true;
	HUD->UpdateUnitHpBars();
	TestEqual(TEXT("Open skill cards retain world HP"), Bar.mRoot->GetVisibility(), ESlateVisibility::HitTestInvisible);
	HUD->mSkillCutInPlaying = true;
	HUD->UpdateUnitHpBars();
	TestEqual(TEXT("Skill cut-in does not hide world HP"), Bar.mRoot->GetVisibility(), ESlateVisibility::HitTestInvisible);
	HUD->SetVisibility(ESlateVisibility::Collapsed);
	HUD->UpdateUnitHpBars();
	TestEqual(TEXT("Hidden HUD still hides world HP"), Bar.mRoot->GetVisibility(), ESlateVisibility::Collapsed);
	HUD->mSkillCutInPlaying = false;
	// Exercise the data-update path too: damage must update the value while
	// cards/preview/cut-in are active, even without an offscreen test viewport.
	APlayerController* Controller = World->SpawnActor<APlayerController>();
	if (!TestNotNull(TEXT("HP test controller"), Controller)) return false;
	// Editor-world spawns are not registered like game-world controllers.
	World->AddController(Controller);
	Controller->SetPlayer(NewObject<ULocalPlayer>(GEngine));
	HUD->SetOwningPlayer(Controller);
	UCombatUIModel* Model = NewObject<UCombatUIModel>(HUD);
	FUnitUI Unit;
	Unit.mUnitId = 1;
	Unit.mHP = 100;
	Unit.mMaxHP = 100;
	Model->SetUnitUIs({Unit});
	HUD->mUIModel = Model;
	TestEqual(TEXT("HP fixture has one unit"), Model->GetUnitUIs().Num(), 1);
	TestNotNull(TEXT("HP fixture owning controller"), HUD->GetOwningPlayer());
	Bar.mValueText = NewObject<UTextBlock>(HUD);
	HUD->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	HUD->UpdateUnitHpBars();
	TestEqual(TEXT("Open cards retain current HP value"), Bar.mValueText->GetText().ToString(), FString(TEXT("100/100")));
	HUD->mSkillCutInPlaying = true;
	Unit.mHP = 65;
	Model->SetUnitUIs({Unit});
	HUD->UpdateUnitHpBars();
	TestEqual(TEXT("Damage refreshes HP before cards close"), Bar.mValueText->GetText().ToString(), FString(TEXT("65/100")));
	HUD->mSkillCutInPlaying = false;
	HUD->mUIModel = nullptr;
	HUD->SetOwningPlayer(nullptr);
	World->RemoveController(Controller);
	Controller->Destroy();
	return true;
}

#endif
