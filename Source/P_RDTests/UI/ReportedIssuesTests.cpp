#include "Misc/AutomationTest.h"
#include "Editor.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "UI/Combat/CombatLayoutHUDWidget.h"
#include "UI/Combat/CombatUIModel.h"
#include "UI/Combat/SkillDetailOverlayPresenter.h"
#include "UI/Shop/ShopUIWidgetBase.h"
#include "UI/Shop/ShopUIModel.h"
#include "UI/ShopUITestListener.h"
#include "UI/FrontendMapWidget.h"
#include "GameMode/ShopGameMode.h"
#include "Singleton/WorldSubsystem/WorldWidgetSubsystem.h"
#include "UObject/UnrealType.h"
#include "UObject/Script.h"
#include "Slate/WidgetRenderer.h"
#include "Framework/Application/SlateApplication.h"
#include "Input/HittestGrid.h"
#include "Layout/WidgetPath.h"
#include "Widgets/SVirtualWindow.h"
#include "Widgets/SOverlay.h"
#include "RenderingThread.h"

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
namespace FeedbackTests { void Capture(UUserWidget* Widget, const TCHAR* Name); }
namespace IssueTests
{
void Click(UUserWidget* W,const TCHAR* Name)
{ if (auto* B=Cast<UButton>(W->GetWidgetFromName(Name))) B->OnClicked.Broadcast(); }
void Call(UObject* Object,const TCHAR* Name)
{ FEditorScriptExecutionGuard Guard; if (auto* F=Object->FindFunction(Name)) Object->ProcessEvent(F,nullptr); }
void Dump(UUserWidget* W)
{
    W->WidgetTree->ForEachWidget([](UWidget* C)
    {
        if (auto* S=Cast<UCanvasPanelSlot>(C->Slot))
            UE_LOG(LogTemp,Display,TEXT("ISSUE_WIDGET %s %s vis=%d pos=%s size=%s z=%d"),
                *C->GetName(),*C->GetClass()->GetName(),int32(C->GetVisibility()),
                *S->GetPosition().ToString(),*S->GetSize().ToString(),S->GetZOrder());
    });
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReportedMenuIssueTest,"P_RD.UI.ReportedIssues.MonsterMenuAndCritical",
 EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FReportedMenuIssueTest::RunTest(const FString&)
{
    FEditorScriptExecutionGuard Guard;
    auto* World=GEditor->GetEditorWorldContext().World();
    auto* C=LoadClass<UCombatLayoutHUDWidget>(nullptr,TEXT("/Game/UI/CombatLayouts/WBP_CombatHUD04.WBP_CombatHUD04_C"));
    auto* HUD=CreateWidget<UCombatLayoutHUDWidget>(World,C); const auto SlateHUD=HUD->TakeWidget();
    auto* Model=NewObject<UCombatUIModel>(HUD);HUD->BindUIModel(Model);
    FUnitUI Ally;Ally.mUnitId=100;Ally.mIsPlayer=true;Ally.mName=FText::FromString(TEXT("Knight"));Ally.mHP=Ally.mMaxHP=100;Ally.mCriticalPoint=27;
    FUnitUI Enemy=Ally;Enemy.mUnitId=200;Enemy.mIsPlayer=false;Enemy.mName=FText::FromString(TEXT("Spider"));
    Model->SetUnitUIs({Ally,Enemy});
    FTurnUI Turn;Turn.mCurrentUnitId=100;Model->SetTurnUI(Turn);
    IssueTests::Click(HUD,TEXT("MenuButton_1"));
    auto* Crit=Cast<UTextBlock>(HUD->GetWidgetFromName(TEXT("MercenaryCritValue")));
    auto* CachedCritProperty=FindFProperty<FObjectPropertyBase>(HUD->GetClass(),TEXT("mMercenaryDetailCritical"));
    auto* CachedCrit=Cast<UTextBlock>(CachedCritProperty->GetObjectPropertyValue_InContainer(HUD));
    UE_LOG(LogTemp,Display,TEXT("ISSUE_CRITICAL direct=%s cached=%s cachedText=%s modelCount=%d"),
        Crit?*Crit->GetText().ToString():TEXT("missing"),*GetNameSafe(CachedCrit),CachedCrit?*CachedCrit->GetText().ToString():TEXT("missing"),Model->GetUnitUIs().Num());
    if(TestNotNull(TEXT("Mercenary critical label"),Crit))TestTrue(TEXT("Critical shows 27, not dash"),Crit->GetText().ToString().Contains(TEXT("27")));
    Ally.mCriticalPoint=39;Model->SetUnitUIs({Ally,Enemy});
    if(Crit)TestTrue(TEXT("Critical updates with unit stats"),Crit->GetText().ToString().Contains(TEXT("39")));
    FWidgetRenderer AlignmentRenderer(true,true);
    for (const FVector2D Size : {FVector2D(1920,1080),FVector2D(1280,888)})
    {
        for(int32 Frame=0;Frame<5;++Frame)
        {
            AlignmentRenderer.DrawWidget(SlateHUD,Size);
            SlateHUD->Tick(SlateHUD->GetTickSpaceGeometry(),Frame*.016,.016f);
        }
        for(const TCHAR* Name:{TEXT("MercenaryDetailHP"),TEXT("MercenaryDetailAP"),TEXT("MercenaryDetailSpeed")})
        {
            auto* Value=HUD->GetWidgetFromName(Name);
            if(!TestNotNull(Name,Value) || !Crit)continue;
            const auto CG=Crit->GetCachedGeometry();const auto VG=Value->GetCachedGeometry();
            TestTrue(TEXT("Visible stat has rendered geometry"),VG.GetLocalSize().X>0.f);
            const double CX=CG.LocalToAbsolute(CG.GetLocalSize()*.5f).X;
            const double VX=VG.LocalToAbsolute(VG.GetLocalSize()*.5f).X;
            UE_LOG(LogTemp,Display,TEXT("CRITICAL_ALIGNMENT width=%.0f %s reference=%.2f critical=%.2f"),Size.X,Name,VX,CX);
            TestTrue(*FString::Printf(TEXT("Critical shares the %s value column"),Name),FMath::Abs(CX-VX)<1.f);
        }
    }
    FeedbackTests::Capture(HUD,TEXT("mercenary-critical-column.png"));
    IssueTests::Click(HUD,TEXT("MenuButton_2"));
    auto* Tab=HUD->GetMonsterTabWidgetForTest();if(!TestNotNull(TEXT("Monster tab"),Tab))return false;
    IssueTests::Dump(Tab);
    auto Window=SNew(SVirtualWindow).Size(FVector2D(1920,1080));
    Window->SetContent(SNew(SOverlay)+SOverlay::Slot()[SlateHUD]+SOverlay::Slot()[Tab->TakeWidget()]);
    auto& App=FSlateApplication::Get();App.RegisterVirtualWindow(Window);
    FWidgetRenderer Renderer(true,true);FHittestGrid Grid;
    auto* Target=FWidgetRenderer::CreateTargetFor(FVector2D(1920,1080),TF_Bilinear,true);
    Renderer.DrawWindow(Target,Grid,Window,1.f,FVector2D(1920,1080),0.f);FlushRenderingCommands();
    for(int32 I=0;I<4;++I)
    {
        auto* Button=HUD->GetWidgetFromName(*FString::Printf(TEXT("MenuButton_%d"),I));
        const auto G=Button->GetCachedGeometry();
        const FVector2D P=G.GetAbsolutePosition()+G.GetAbsoluteSize()*.5;
        FWidgetPath Path(Grid.GetBubblePath(P,0,false));
        TestTrue(*FString::Printf(TEXT("Monster tab leaves menu %d clickable"),I),Path.ContainsWidget(Button->GetCachedWidget().Get()));
    }
    IssueTests::Click(HUD,TEXT("MenuButton_1"));TestFalse(TEXT("Switching to party closes monster tab"),Tab->IsVisible());
    App.UnregisterVirtualWindow(Window);HUD->CloseUI();Tab->RemoveFromParent();
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReportedShopBackIssueTest,"P_RD.UI.ReportedIssues.ShopMapReturn",
 EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FReportedShopBackIssueTest::RunTest(const FString&)
{
    FEditorScriptExecutionGuard Guard;
    auto* World=GEditor->GetEditorWorldContext().World();
    auto* Sub=World->GetSubsystem<UWorldWidgetSubsystem>();
    auto* OriginalHUD=Sub->GetHUD<>();auto* OriginalMap=Sub->GetWorldWidget(EWorldWidgetType::WorldMap);
    auto* HC=LoadClass<UShopUIWidgetBase>(nullptr,TEXT("/Game/UI/Shop/WBP_Shop_FullGenerated.WBP_Shop_FullGenerated_C"));
    auto* Shop=CreateWidget<UShopUIWidgetBase>(World,HC);const auto Slate=Shop->TakeWidget();
    auto* Model=NewObject<UShopUIModel>(Shop);FShopUI Data;Data.mGold=123;Model->SetShop(Data);Shop->BindUIModel(Model);Shop->OpenUI();
    auto* MC=LoadClass<UFrontendMapWidget>(nullptr,TEXT("/Game/UI/WBP_FrontendMap.WBP_FrontendMap_C"));
    auto* Map=CreateWidget<UFrontendMapWidget>(World,MC);const auto MapSlate=Map->TakeWidget();
    auto* HUDProperty=FindFProperty<FObjectPropertyBase>(Sub->GetClass(),TEXT("mHUD"));HUDProperty->SetObjectPropertyValue_InContainer(Sub,Shop);
    Sub->SetWorldWidgetForTest(EWorldWidgetType::WorldMap,Map);
    auto* GC=LoadClass<AShopGameMode>(nullptr,TEXT("/Game/BP/GameMode/BP_ShopGameMode.BP_ShopGameMode_C"));
    auto* Mode=World->SpawnActor<AShopGameMode>(GC);
    for(int32 I=0;I<2;++I)
    {
        IssueTests::Call(Mode,TEXT("HandleLeaveRequested"));Shop->CloseUI();
        TestTrue(TEXT("Leaving connects map Back"),Map->OnCloseRequested.IsBound());
        Map->OnCloseRequested.Broadcast();
        TestFalse(TEXT("Back closes map"),Map->IsVisible());
        TestTrue(TEXT("Back reopens the same shop"),Shop->IsVisible());
        TestEqual(TEXT("Back preserves gold"),Model->GetShop().mGold,123);
    }
    Mode->Destroy();Shop->CloseUI();Map->CloseUI();
    HUDProperty->SetObjectPropertyValue_InContainer(Sub,OriginalHUD);Sub->SetWorldWidgetForTest(EWorldWidgetType::WorldMap,OriginalMap);
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReportedDetailIssueTest,"P_RD.UI.ReportedIssues.DetailCloseGeometry",
 EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FReportedDetailIssueTest::RunTest(const FString&)
{
    auto* World=GEditor->GetEditorWorldContext().World();
    auto* C=LoadClass<UUserWidget>(nullptr,TEXT("/Game/UI/CombatDetail/WBP_CombatDetailOverlay.WBP_CombatDetailOverlay_C"));
    auto* P=NewObject<USkillDetailOverlayPresenter>();P->Initialize(World,C,nullptr,70);
    P->EnsureOverlayWidget();auto* W=P->GetOverlayWidget();const auto Slate=W->TakeWidget();
    FCombatArtifactUI A;A.mName=FText::FromString(TEXT("Artifact"));P->PresentArtifact(A);P->SetNavigationEnabled(true);
    FWidgetRenderer Renderer(true,true);
    for(int32 Frame=0;Frame<3;++Frame)Renderer.DrawWidget(Slate,FVector2D(1920,1080));
    FlushRenderingCommands();
    W->WidgetTree->ForEachWidget([](UWidget* C){if(C->GetName().Contains(TEXT("Close")))UE_LOG(LogTemp,Display,TEXT("ISSUE_CLOSE %s class=%s parent=%s center=%s size=%s"),*C->GetName(),*C->GetClass()->GetName(),*GetNameSafe(C->GetParent()),*(C->GetCachedGeometry().GetAbsolutePosition()+C->GetCachedGeometry().GetAbsoluteSize()*.5).ToString(),*C->GetCachedGeometry().GetAbsoluteSize().ToString());});
    auto* Close=W->GetWidgetFromName(TEXT("DetailCloseButton"));
    TestTrue(TEXT("Close button has rendered area"),Close->GetCachedGeometry().GetAbsoluteSize().X>50.f);
    for(const TCHAR* Name:{TEXT("DetailCloseArt"),TEXT("DetailCloseText")})
    {
        auto* Child=W->GetWidgetFromName(Name);
        if(TestNotNull(Name,Child))
        {
            const auto B=Close->GetCachedGeometry();const auto ChildGeometry=Child->GetCachedGeometry();
            TestTrue(*FString::Printf(TEXT("%s centered in clickable button"),Name),
                (B.GetAbsolutePosition()+B.GetAbsoluteSize()*.5-ChildGeometry.GetAbsolutePosition()-ChildGeometry.GetAbsoluteSize()*.5).Size()<3.f);
        }
    }
    P->Teardown();return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReportedArtifactNavigationTest,"P_RD.UI.ReportedIssues.ArtifactNavigation",
 EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FReportedArtifactNavigationTest::RunTest(const FString&)
{
    auto* World=GEditor->GetEditorWorldContext().World();
    auto* C=LoadClass<UShopUIWidgetBase>(nullptr,TEXT("/Game/UI/Shop/WBP_Shop_FullGenerated.WBP_Shop_FullGenerated_C"));
    auto* Shop=CreateWidget<UShopUIWidgetBase>(World,C);const auto Slate=Shop->TakeWidget();
    auto* Model=NewObject<UShopUIModel>(Shop);FShopUI Data;
    for(int32 I=0;I<3;++I)
    {FShopItemUI Item;Item.mSlotIndex=I;Item.mKind=EShopItemKind::Artifact;Item.mName=FText::FromString(FString::Printf(TEXT("Artifact %d"),I));Data.mItems.Add(Item);}
    Model->SetShop(Data);Shop->BindUIModel(Model);
    auto* Listener=NewObject<UShopUITestListener>(Shop);Listener->DetailModel=Model;
    Model->OnItemDetailRequested.AddDynamic(Listener,&UShopUITestListener::HandleItemDetailRequested);
    IssueTests::Call(Shop,TEXT("HandleArtifactTabClicked"));
    IssueTests::Click(Shop,TEXT("ShopRailButton_2"));
    auto* Property=FindFProperty<FObjectPropertyBase>(Shop->GetClass(),TEXT("mShopSkillDetailPresenter"));
    auto* P=Cast<USkillDetailOverlayPresenter>(Property->GetObjectPropertyValue_InContainer(Shop));
    if(!TestNotNull(TEXT("Artifact opens rich detail presenter"),P))return false;
    const int32 First=Listener->LastSlotIndex;auto* W=P->GetOverlayWidget();
    const auto DetailSlate=W->TakeWidget();
    TestTrue(TEXT("Artifact next arrow visible"),W->GetWidgetFromName(TEXT("DetailNextButton"))->IsVisible());
    IssueTests::Click(W,TEXT("DetailNextButton"));
    TestTrue(TEXT("Next requests another artifact"),Listener->LastSlotIndex!=First);
    TestEqual(TEXT("Detail title updates"),P->GetTitleText()->GetText().ToString(),FString::Printf(TEXT("Artifact %d"),Listener->LastSlotIndex));
    IssueTests::Click(W,TEXT("DetailPreviousButton"));
    TestEqual(TEXT("Previous returns to original artifact"),Listener->LastSlotIndex,First);
    P->Teardown();Shop->CloseUI();return !HasAnyErrors();
}
#endif
