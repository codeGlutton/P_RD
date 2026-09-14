#include "Misc/AutomationTest.h"
#include "UI/Tutorial/ShopGuideWidget.h"
#include "Singleton/InstanceSubsystem/PersistentData.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Editor.h"
#include "Slate/WidgetRenderer.h"
#include "Engine/TextureRenderTarget2D.h"
#include "TextureCompiler.h"
#include "ImageUtils.h"
#include "Misc/FileHelper.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShopGuideSaveTest, "P_RD.Tutorial.Shop.Save", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FShopGuideSaveTest::RunTest(const FString&)
{
    auto* Profile = NewObject<UUserPersistData>();
    TestFalse(TEXT("Existing and new profiles can see their first shop guide"), Profile->SeenShopWalkthrough);
    Profile->SeenShopWalkthrough = true;
    TArray<uint8> Bytes;
    { FMemoryWriter W(Bytes); FObjectAndNameAsStringProxyArchive A(W, false); A.ArIsSaveGame = true; A.ArNoDelta = true; Profile->Serialize(A); }
    auto* Reloaded = NewObject<UUserPersistData>();
    { FMemoryReader R(Bytes); FObjectAndNameAsStringProxyArchive A(R, false); A.ArIsSaveGame = true; A.ArNoDelta = true; Reloaded->Serialize(A); }
    TestTrue(TEXT("Dismissal survives a profile save/load"), Reloaded->SeenShopWalkthrough);
    TestFalse(TEXT("Shop history does not start the combat tutorial"), Reloaded->GuidedTutorial.Enrolled);
    TestTrue(TEXT("Shop history does not acknowledge traps"), Reloaded->SeenTrapHints.IsEmpty());
    Reloaded->ClearUser();
    TestFalse(TEXT("Only a profile reset clears the shop history"), Reloaded->SeenShopWalkthrough);
    return !HasAnyErrors();
}


#include "UI/Tutorial/GuidedTutorialWidget.h"
#include "UI/Shop/ShopUIWidgetBase.h"
#include "UI/Shop/ShopUIModel.h"
#include "DataAsset/ArtifactData/StaticArtifactData.h"
#include "Widgets/SOverlay.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShopGuideReadingTest, "P_RD.Tutorial.Shop.ReadingAndRender", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FShopGuideReadingTest::RunTest(const FString&)
{
    if (GUsingNullRHI) { AddError(TEXT("Rendering required")); return false; }
    auto* World = GEditor->GetEditorWorldContext().World();
    auto* ShopClass = LoadClass<UShopUIWidgetBase>(nullptr, TEXT("/Game/UI/Shop/WBP_Shop_FullGenerated.WBP_Shop_FullGenerated_C"));
    auto* Shop = CreateWidget<UShopUIWidgetBase>(World, ShopClass);
    const auto ShopSlate = Shop->TakeWidget();
    auto* Model = NewObject<UShopUIModel>(Shop);
    FShopUI Data; Data.mGold = 100;
    auto* Artifact = LoadObject<UStaticArtifactData>(nullptr, TEXT("/Game/BP/DataAsset/Artifact/DA_Artifact_A039_GiantsHeart"));
    auto* Icon = Artifact->mIcon.LoadSynchronous();
    TestNotNull(TEXT("The reported Giant's Heart icon is restored"), Icon);
    for (int32 I = 0; I < 3; ++I)
    {
        auto& Item = Data.mItems.AddDefaulted_GetRef(); Item.mSlotIndex = I;
        Item.mName = Artifact->mName; Item.mIcon = Icon; Item.mPrice = 200;
        Item.mKind = I == 2 ? EShopItemKind::Skill : EShopItemKind::Artifact;
        Item.mRequiredJobType = EUnitJobType::Knight;
    }
    FShopOwnedUnitUI Unit; Unit.mUnitIndex = 0; Unit.mJobType = EUnitJobType::Knight;
    Unit.mSkillSlots.SetNum(5); Data.mOwnedUnits.Add(Unit); Data.mSkillTargetUnits.Add(Unit);
    Data.mRest.mPrice = 100; Data.mRest.mIsAffordable = true;
    Model->SetShop(Data); Shop->BindUIModel(Model); Shop->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
    const FString Folder = FPaths::ProjectSavedDir() / TEXT("UI/ShopWalkthrough");
    IFileManager::Get().MakeDirectory(*Folder, true);
    for (const FVector2D Size : {FVector2D(1400, 1165), FVector2D(1100, 700)})
    {
        auto* Widget = CreateWidget<UShopGuideWidget>(World);
        const auto Slate = Widget->TakeWidget();
        Widget->SetVisibility(ESlateVisibility::Visible);
        auto Composite = SNew(SOverlay) + SOverlay::Slot()[ShopSlate] + SOverlay::Slot()[Slate];
        int32 Dismissals = 0; UWidget* Focus = nullptr;
        Widget->OnDismissed.BindLambda([&] { ++Dismissals; });
        Widget->OnStepChanged.BindLambda([&](int32 Step) { Focus = Shop->ShowShopGuideStepForTest(Step); return Focus; });
        Widget->BeginReading();
        for (int32 Page = 0; Page < UShopGuideWidget::PageCount; ++Page)
        {
            TestEqual(TEXT("Next explains one actual control at a time"), Widget->GetPage(), Page);
            TestEqual(TEXT("Reading alone does not acknowledge the guide"), Dismissals, 0);
            TestTrue(*FString::Printf(TEXT("Step %d points to a visible shop control"), Page + 1), Focus && Focus->IsVisible());
            FTextureCompilingManager::Get().FinishAllCompilation();
            FWidgetRenderer Renderer(true, true); Renderer.SetIsPrepassNeeded(true);
            for (int I = 0; I < 4; ++I) { Renderer.DrawWidget(Composite, Size); Widget->GetGuide()->UpdateLayoutForTest(); }
            auto* Target = Renderer.DrawWidget(Composite, Size); FlushRenderingCommands();
            for (auto* Button : {Widget->GetGuide()->GetDismissButton(), Widget->GetGuide()->GetContinueButton()})
            {
                TestTrue(TEXT("Closing and advancing are always available"), Button && Button->IsVisible() && Button->GetIsEnabled());
                const auto& G = Button->GetCachedGeometry();
                const auto Min = G.GetAbsolutePosition(), Max = Min + G.GetAbsoluteSize();
                TestTrue(TEXT("Guide buttons stay on screen"), Min.X >= 0 && Min.Y >= 0 && Max.X <= Size.X + 1 && Max.Y <= Size.Y + 1);
            }
            if (Focus)
            {
                const auto& FG = Focus->GetCachedGeometry();
                const auto& CG = Widget->GetGuide()->GetCalloutGeometryForTest();
                const auto FA = FG.GetAbsolutePosition(), FB = FA + FG.GetAbsoluteSize();
                const auto CA = CG.GetAbsolutePosition(), CB = CA + CG.GetAbsoluteSize();
                TestFalse(*FString::Printf(TEXT("Step %d callout does not cover its target"), Page + 1),
                    CA.X < FB.X && CB.X > FA.X && CA.Y < FB.Y && CB.Y > FA.Y);
            }
            TestEqual(TEXT("No gold spent by changing guide steps"), Model->GetShop().mGold, 100);
            TestFalse(TEXT("Guide never purchases the artifact"), Model->GetShop().mItems[0].mIsSoldOut);
            TestFalse(TEXT("Guide never uses the rest service"), Model->GetShop().mRest.mIsUsed);
            TArray<FColor> Pixels; Target->GameThread_GetRenderTargetResource()->ReadPixels(Pixels);
            TArray64<uint8> PNG; FImageUtils::PNGCompressImageArray(Size.X, Size.Y, Pixels, PNG);
            FFileHelper::SaveArrayToFile(PNG, *(Folder / FString::Printf(TEXT("%dx%d-step%02d.png"), int32(Size.X), int32(Size.Y), Page + 1)));
            Widget->Next();
        }
        TestEqual(TEXT("Last explanation completes without transacting"), Dismissals, 1);
        Widget->Dismiss(); TestEqual(TEXT("Double dismissal is safe"), Dismissals, 1);
    }
    for (bool Back : {false, true})
    {
        auto* Widget = CreateWidget<UShopGuideWidget>(World);
        int32 Dismissals = 0; Widget->OnDismissed.BindLambda([&] { ++Dismissals; });
        if (Back) Widget->HandleBackNavigation(); else Widget->Dismiss();
        TestEqual(TEXT("Can immediately skip all explanations"), Dismissals, 1);
    }
    return !HasAnyErrors();
}
