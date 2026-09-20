#include "Misc/AutomationTest.h"
#include "Editor.h"
#include "UI/Combat/SkillDetailOverlayPresenter.h"
#include "UI/Combat/StatusDescriptionText.h"
#include "UI/Combat/CombatStatusPresentation.h"
#include "UI/Shop/ShopUIWidgetBase.h"
#include "UI/Shop/ShopUIModel.h"
#include "UI/Tutorial/ShopGuideWidget.h"
#include "UI/Tutorial/GuidedTutorialWidget.h"
#include "UI/StageVictory/BossEntranceWidget.h"
#include "GameplayTagType.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "Slate/WidgetRenderer.h"
#include "Widgets/SOverlay.h"
#include "Engine/TextureRenderTarget2D.h"
#include "ImageUtils.h"
#include "Misc/FileHelper.h"
#include "Serialization/BufferArchive.h"
#include "Internationalization/Internationalization.h"
#include "Internationalization/Culture.h"

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
namespace FeedbackTests { void Capture(UUserWidget* Widget, const TCHAR* Name); }

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRewardDetailFeedbackTest, "P_RD.UI.Feedback.SkillGlossaryAndNavigation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRewardDetailFeedbackTest::RunTest(const FString&)
{
    const FString Culture = FInternationalization::Get().GetCurrentCulture()->GetName();
    FInternationalization::Get().SetCurrentCulture(TEXT("ko"));
    auto* World = GEditor->GetEditorWorldContext().World();
    auto* Class = LoadClass<UUserWidget>(nullptr, TEXT("/Game/UI/CombatDetail/WBP_CombatDetailOverlay.WBP_CombatDetailOverlay_C"));
    auto* Presenter = NewObject<USkillDetailOverlayPresenter>();
    Presenter->Initialize(World, Class, nullptr, 70);
    FSkillDetailUI Skill;
    Skill.mName = FText::FromString(TEXT("상태 설명 확인"));
    Skill.mDescription = FText::FromString(TEXT("대상에게 탈진 태그를 부여합니다. 탈진을 누르면 효과를 자세히 볼 수 있습니다."));
    Presenter->Present(Skill);
    Presenter->SetNavigationEnabled(true);
    auto* Overlay = Presenter->GetOverlayWidget();
    if (!TestNotNull(TEXT("Actual detail WBP"), Overlay)) return false;
    const auto OverlaySlate = Overlay->TakeWidget();
    int32 Moves = 0;
    Presenter->OnNextRequested.BindLambda([&] { ++Moves; });
    Presenter->OnPreviousRequested.BindLambda([&] { --Moves; });
    auto* Next = Cast<UButton>(Overlay->GetWidgetFromName(TEXT("DetailNextButton")));
    auto* Prev = Cast<UButton>(Overlay->GetWidgetFromName(TEXT("DetailPreviousButton")));
    if (TestNotNull(TEXT("Detail has its own next button above the input shield"), Next))
    {
        Next->OnClicked.Broadcast(); TestEqual(TEXT("Next dispatches once"), Moves, 1);
    }
    if (TestNotNull(TEXT("Detail previous button"), Prev))
    {
        Prev->OnClicked.Broadcast(); TestEqual(TEXT("Previous reverses navigation"), Moves, 0);
    }
    const FString Markup = UStatusDescriptionText::MakeStatusMarkup(Skill.mDescription.ToString());
    TestTrue(TEXT("Exhaustion in the paragraph becomes an actual hyperlink"), Markup.Contains(TEXT("id=\"status\"")));
    TestTrue(TEXT("Literal angle brackets are escaped"), UStatusDescriptionText::MakeStatusMarkup(TEXT("<test>")).Contains(TEXT("&lt;test&gt;")));
    FeedbackTests::Capture(Overlay, TEXT("skill-status-links.png"));
    auto* Close = Cast<UButton>(Overlay->GetWidgetFromName(TEXT("DetailCloseButton")));
    auto* CloseText = Cast<UTextBlock>(Overlay->GetWidgetFromName(TEXT("DetailCloseText")));
    if (TestNotNull(TEXT("Close button"), Close) && TestNotNull(TEXT("Close text"), CloseText))
    {
        const auto& B = Close->GetCachedGeometry(); const auto& T = CloseText->GetCachedGeometry();
        TestTrue(TEXT("Close caption is centered inside button"),
            (B.GetAbsolutePosition() + B.GetAbsoluteSize()*.5 - T.GetAbsolutePosition() - T.GetAbsoluteSize()*.5).Size() < 3.f);
    }
    Presenter->OpenStatusGlossary(EffectTags::GameplayEffect_StatusEffect_RoundDuration_Debuff_Exhaustion);
    TestFalse(TEXT("Glossary hides candidate navigation"), Next && Next->IsVisible());
    TestTrue(TEXT("Glossary title identifies exhaustion"), Presenter->GetTitleText()->GetText().EqualTo(
        CombatStatusUI::Resolve(EffectTags::GameplayEffect_StatusEffect_RoundDuration_Debuff_Exhaustion).mDisplayName));
    FeedbackTests::Capture(Overlay, TEXT("skill-exhaustion-detail.png"));
    if (Close) Close->OnClicked.Broadcast();
    TestTrue(TEXT("Close restores the same skill without losing selection"), Presenter->GetTitleText()->GetText().EqualTo(Skill.mName));
    TestTrue(TEXT("Comparison navigation is restored"), Next && Next->IsVisible());
    if (Close) Close->OnClicked.Broadcast();
    TestFalse(TEXT("Second close dismisses detail"), Presenter->IsShowing());
    Presenter->Teardown();
    FInternationalization::Get().SetCurrentCulture(Culture);
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLevelRewardGuideTest, "P_RD.UI.Feedback.LevelRewardWalkthrough",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLevelRewardGuideTest::RunTest(const FString&)
{
    auto* World = GEditor->GetEditorWorldContext().World();
    auto* Class = LoadClass<UShopUIWidgetBase>(nullptr, TEXT("/Game/UI/Shop/WBP_Shop_FullGenerated.WBP_Shop_FullGenerated_C"));
    auto* Shop = CreateWidget<UShopUIWidgetBase>(World, Class);
    const auto ShopSlate = Shop->TakeWidget();
    auto* Model = NewObject<UShopUIModel>(Shop);
    FShopUI Data; Data.mIsLevelUpReward = true; Data.mGold = 123; Data.mRewardUnitIndex = 0;
    Data.mRewardTitle = FText::FromString(TEXT("Knight Lv. 2"));
    FShopOwnedUnitUI Unit; Unit.mUnitIndex = 0; Unit.mJobType = EUnitJobType::Knight; Unit.mSkillSlots.SetNum(6);
    Data.mOwnedUnits.Add(Unit); Data.mSkillTargetUnits.Add(Unit);
    for (int32 I = 0; I < 3; ++I)
    {
        auto& Item = Data.mItems.AddDefaulted_GetRef(); Item.mSlotIndex = I;
        Item.mKind = EShopItemKind::Skill; Item.mRequiredJobType = EUnitJobType::Knight;
        Item.mName = FText::FromString(FString::Printf(TEXT("Skill %d"), I));
        Item.mIcon = LoadObject<UTexture2D>(nullptr, TEXT("/Game/SVN/OutSideAsset/AICreation/UI/CombatHUD/SkillIcons/T_SkillIcon_Slash.T_SkillIcon_Slash"));
    }
    Model->SetShop(Data); Shop->BindUIModel(Model); Shop->OpenUI();
    auto* Guide = CreateWidget<UShopGuideWidget>(World);
    Guide->SetLevelUpMode(true);
    Guide->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
    const auto GuideSlate = Guide->TakeWidget();
    auto Composite = SNew(SOverlay) + SOverlay::Slot()[ShopSlate] + SOverlay::Slot()[GuideSlate];
    UWidget* Focus = nullptr; int32 Dismissals = 0;
    Guide->OnStepChanged.BindLambda([&](int32 Step) { Focus = Shop->ShowShopGuideStepForTest(Step); return Focus; });
    Guide->OnDismissed.BindLambda([&] { ++Dismissals; });
    Guide->BeginReading();
    for (int32 I = 0; I < Guide->GetPageCount(); ++I)
    {
        TestEqual(TEXT("Each actual control gets its own step"), Guide->GetPage(), I);
        TestTrue(*FString::Printf(TEXT("Level-up target %d is visible"), I), Focus && Focus->IsVisible());
                for (const FVector2D Size : {FVector2D(1920, 1080), FVector2D(1400, 1165)})
        {
            FWidgetRenderer Renderer(true, true);
            for (int32 Frame = 0; Frame < 4; ++Frame)
            {
                Renderer.DrawWidget(Composite, Size);
                Guide->GetGuide()->UpdateLayoutForTest();
            }
            auto* Image = Renderer.DrawWidget(Composite, Size);
            if (Focus)
            {
                const auto& FG = Focus->GetCachedGeometry();
                const auto& CG = Guide->GetGuide()->GetCalloutGeometryForTest();
                const auto A = FG.GetAbsolutePosition(), B = A + FG.GetAbsoluteSize();
                const auto C = CG.GetAbsolutePosition(), D = C + CG.GetAbsoluteSize();
                TestFalse(TEXT("Level guide leaves its highlighted control uncovered"),
                    A.X < D.X && B.X > C.X && A.Y < D.Y && B.Y > C.Y);
            }
            FBufferArchive Bytes; FImageUtils::ExportRenderTarget2DAsPNG(Image, Bytes);
            FFileHelper::SaveArrayToFile(Bytes, *(FPaths::ProjectSavedDir() / TEXT("UI/Feedback") /
                FString::Printf(TEXT("level-guide-%d-%d.png"), I, int32(Size.X))));
        }
        Guide->GetGuide()->GetContinueButton()->OnClicked.Broadcast();
    }
    TestEqual(TEXT("Only reading next finishes the guide"), Dismissals, 1);
    TestEqual(TEXT("No gold spent"), Model->GetShop().mGold, 123);
    for (const auto& Item : Model->GetShop().mItems) TestFalse(TEXT("No skill acquired by guide"), Item.mIsSoldOut);
    Shop->CloseUI(); Guide->RemoveFromParent();
    auto* Boss = CreateWidget<UBossEntranceWidget>(World);
    Boss->SetIgnoreCombatPlayback(true);
    TestFalse(TEXT("Boss cinema refuses combat acceleration"), Boss->SetCinematicPlaybackRate(3.f));
    TestFalse(TEXT("Boss cinema also refuses maximum combat speed"), Boss->SetCinematicPlaybackRate(8.f));
    return !HasAnyErrors();
}
#endif
