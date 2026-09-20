#include "Misc/AutomationTest.h"
#include "Editor.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/OverlaySlot.h"
#include "Components/ScaleBox.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "AssetCompilingManager.h"
#include "Internationalization/Internationalization.h"
#include "Internationalization/Culture.h"
#include "RHI.h"
#include "Slate/WidgetRenderer.h"
#include "ImageUtils.h"
#include "TextureResource.h"
#include "Misc/FileHelper.h"
#include "Serialization/BufferArchive.h"
#include "UI/CombatResultOverlayWidget.h"

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatDefeatClassicTest,
    "P_RD.UI.CombatDefeat.ClassicBoard",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatDefeatClassicTest::RunTest(const FString&)
{
    UClass* Class = LoadClass<UCombatResultOverlayWidget>(nullptr,
        TEXT("/Game/UI/CombatResult/WBP_CombatDefeat.WBP_CombatDefeat_C"));
    if (!TestNotNull(TEXT("Defeat widget class"), Class)) return false;
    auto* Widget = CreateWidget<UCombatResultOverlayWidget>(GEditor->GetEditorWorldContext().World(), Class);
    if (!TestNotNull(TEXT("Defeat widget instance"), Widget)) return false;
    Widget->SetVisibility(ESlateVisibility::Visible);
    const auto Slate = Widget->TakeWidget();
    Slate->SetCanTick(false);
    UWidgetTree* Tree = Widget->WidgetTree;
    TestNotNull(TEXT("Responsive board"), Cast<UScaleBox>(Tree->FindWidget(TEXT("DefeatResponsiveScale"))));
    for (const TCHAR* Name : {TEXT("DefeatSurvivorValue"), TEXT("DefeatSurvivorLabel"),
        TEXT("mPartyPortrait0"), TEXT("mPartyPortrait1"), TEXT("mPartyPortrait2"),
        TEXT("mGoldText"), TEXT("mRetryButton")})
        TestNull(Name, Tree->FindWidget(FName(Name)));
    int32 Buttons = 0;
    Tree->ForEachWidget([&](UWidget* Child) {
        Buttons += Cast<UButton>(Child) != nullptr ? 1 : 0;
        if (auto* Text = Cast<UTextBlock>(Child))
        {
            auto* Slot = Cast<UOverlaySlot>(Text->Slot);
            if (TestNotNull(TEXT("Centered text slot"), Slot))
                TestEqual(TEXT("Text vertically centered"), Slot->GetVerticalAlignment(), VAlign_Center);
        }
    });
    TestEqual(TEXT("Single title action"), Buttons, 1);
    auto* Location = Cast<UTextBlock>(Tree->FindWidget(TEXT("mLocationText")));
    auto* Round = Cast<UTextBlock>(Tree->FindWidget(TEXT("mRoundText")));
    auto* Enemy = Cast<UTextBlock>(Tree->FindWidget(TEXT("mEnemyText")));
    auto* Button = Cast<UButton>(Tree->FindWidget(TEXT("mTitleButton")));
    if (!Location || !Round || !Enemy || !Button) { AddError(TEXT("Missing required result binding")); return false; }
    FCombatResultUI Result;
    Result.mLocationName = FText::FromString(TEXT("잊힌 성채"));
    Result.mRound = 7;
    Result.mDefeatedMonsterCount = 12;
    int32 Clicks = 0;
    Widget->ShowDefeatResult(Result, FSimpleDelegate::CreateLambda([&]() { ++Clicks; }));
    TestEqual(TEXT("Independent location"), Location->GetText().ToString(), Result.mLocationName.ToString());
    TestEqual(TEXT("Actual round"), Round->GetText().ToString(), FString(TEXT("7")));
    TestEqual(TEXT("Actual kills"), Enemy->GetText().ToString(), FString(TEXT("12")));
    TestTrue(TEXT("Dark ink on parchment retained at runtime"), Round->GetColorAndOpacity().GetSpecifiedColor().R < .1f);
    Button->OnClicked.Broadcast(); Button->OnClicked.Broadcast();
    TestEqual(TEXT("Repeated click invokes title callback once"), Clicks, 1);

    const FString PreviousCulture = FInternationalization::Get().GetCurrentCulture()->GetName();
    if (!GUsingNullRHI)
    {
        FAssetCompilingManager::Get().FinishAllCompilation();
        auto* Art = Cast<UImage>(Tree->FindWidget(TEXT("DefeatArtwork")));
        if (auto* Texture = Art ? Cast<UTexture2D>(Art->GetBrush().GetResourceObject()) : nullptr)
        { Texture->SetForceMipLevelsToBeResident(30.f); Texture->WaitForStreaming(); }
        for (const TCHAR* Culture : {TEXT("ko"), TEXT("en")})
        {
            FInternationalization::Get().SetCurrentCulture(Culture);
            Result.mLocationName = FText::FromString(FString(Culture) == TEXT("ko") ? TEXT("잊힌 성채") : TEXT("Forgotten Citadel"));
            Widget->ShowDefeatResult(Result, FSimpleDelegate());
            auto* Subtitle = Cast<UTextBlock>(Tree->FindWidget(TEXT("DefeatSubtitleText")));
            TestEqual(TEXT("Localized subtitle"), Subtitle->GetText().ToString(), FString(
                FString(Culture) == TEXT("ko") ? TEXT("이번 원정이 종료되었습니다") : TEXT("This expedition has ended")));
            for (const FIntPoint Size : {FIntPoint(1920, 1080), FIntPoint(1400, 1165)})
            {
                FWidgetRenderer Renderer(true, false);
                auto* Target = Renderer.DrawWidget(Slate, FVector2D(Size));
                for (int32 Frame = 0; Frame < 3; ++Frame) Renderer.DrawWidget(Target, Slate, FVector2D(Size), .016f);
                const auto& Geometry = Button->GetCachedGeometry();
                const FVector2D TopLeft = Geometry.LocalToAbsolute(FVector2D::ZeroVector);
                const FVector2D BottomRight = Geometry.LocalToAbsolute(Geometry.GetLocalSize());
                TestTrue(TEXT("Title action stays inside viewport"), TopLeft.X >= 0 && TopLeft.Y >= 0 && BottomRight.X <= Size.X + 1 && BottomRight.Y <= Size.Y + 1);
                TestTrue(TEXT("Title action remains large enough to touch"), BottomRight.Y - TopLeft.Y >= 60);
                TArray<FColor> Pixels;
                FReadSurfaceDataFlags Flags(RCM_UNorm);
                Flags.SetLinearToGamma(false);
                Target->GameThread_GetRenderTargetResource()->ReadPixels(Pixels, Flags);
                TArray64<uint8> Bytes;
                FImageUtils::PNGCompressImageArray(Size.X, Size.Y, Pixels, Bytes);
                const FString Dir = FPaths::ProjectSavedDir() / TEXT("UI/DefeatClassic");
                IFileManager::Get().MakeDirectory(*Dir, true);
                FFileHelper::SaveArrayToFile(Bytes, *(Dir / FString::Printf(TEXT("defeat-%s-%dx%d.png"), Culture, Size.X, Size.Y)));
            }
        }
    }
    FInternationalization::Get().SetCurrentCulture(PreviousCulture);
    Result.mLocationName = FText::GetEmpty(); Result.mRound = 0; Result.mDefeatedMonsterCount = -1;
    Widget->ShowDefeatResult(Result, FSimpleDelegate());
    TestFalse(TEXT("Empty location gets fallback"), Location->GetText().IsEmpty());
    TestEqual(TEXT("First round minimum"), Round->GetText().ToString(), FString(TEXT("1")));
    TestEqual(TEXT("Negative kills clamp to zero"), Enemy->GetText().ToString(), FString(TEXT("0")));
    Result.mRound = 19; Result.mDefeatedMonsterCount = 83;
    Widget->ShowDefeatResult(Result, FSimpleDelegate());
    TestEqual(TEXT("Reused widget receives new run round"), Round->GetText().ToString(), FString(TEXT("19")));
    TestEqual(TEXT("Reused widget receives new run kills"), Enemy->GetText().ToString(), FString(TEXT("83")));
    return true;
}
#endif
