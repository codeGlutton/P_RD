#include "UI/Reward/RewardConcept03Widget.h"
#include "Components/Image.h"
#include "Editor.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "HAL/FileManager.h"
#include "ImageUtils.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "RHI.h"
#include "ShaderCompiler.h"
#include "Slate/WidgetRenderer.h"
#include "Widgets/Colors/SColorBlock.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SOverlay.h"

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
namespace RewardChestSeparated
{
constexpr int32 Width = 720;
constexpr int32 Height = 600;

bool RenderLayers(UImage& Chest, UImage& Light, const int32 Mode,
    const FLinearColor Background, const FString& Name, TArray<FColor>& Pixels)
{
    TSharedRef<SOverlay> Root = SNew(SOverlay)
        + SOverlay::Slot()[SNew(SColorBlock).Color(Background)];
    if (Mode != 1)
        Root->AddSlot()[SNew(SImage).Image(&Light.GetBrush())
            .ColorAndOpacity(FLinearColor(1.f, 1.f, 1.f, Light.GetRenderOpacity()))];
    if (Mode != 2)
        Root->AddSlot().HAlign(HAlign_Center).VAlign(VAlign_Center)
        [SNew(SBox).WidthOverride(682.f * 390.f / 455.f).HeightOverride(390.f)
            [SNew(SImage).Image(&Chest.GetBrush())]];
    if (GShaderCompilingManager) GShaderCompilingManager->FinishAllCompilation();
    FWidgetRenderer Renderer(true, true);
    for (int32 Pass = 0; Pass < 3; ++Pass)
    {
        Renderer.DrawWidget(Root, FVector2D(Width, Height));
        FlushRenderingCommands();
    }
    UTextureRenderTarget2D* Target = Renderer.DrawWidget(Root, FVector2D(Width, Height));
    FlushRenderingCommands();
    FReadSurfaceDataFlags Flags(RCM_UNorm);
    Flags.SetLinearToGamma(false);
    if (!Target || !Target->GameThread_GetRenderTargetResource()->ReadPixels(Pixels, Flags)) return false;
    TArray64<uint8> Png;
    FImageUtils::PNGCompressImageArray(Width, Height, Pixels, Png);
    const FString Directory = FPaths::ProjectSavedDir() / TEXT("UI/RewardChestSeparated");
    IFileManager::Get().MakeDirectory(*Directory, true);
    return FFileHelper::SaveArrayToFile(Png, *(Directory / (Name + TEXT(".png"))));
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRewardChestSeparatedTest,
    "P_RD.UI.RewardChestSeparated.TimelineAndRender",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRewardChestSeparatedTest::RunTest(const FString& Parameters)
{
    using namespace RewardChestSeparated;
    if (GUsingNullRHI) { AddError(TEXT("Requires a real RHI to verify the chest/light boundary.")); return false; }
    UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
    if (!TestNotNull(TEXT("Capture world"), World)) return false;
    for (const TCHAR* Variant : { TEXT("WBP_RewardConcept03_Frameless"), TEXT("WBP_RewardConcept03_Frameless_NoArtifact") })
    {
        const FString Path = FString::Printf(TEXT("/Game/UI/RewardConcept03New/%s.%s_C"), Variant, Variant);
        UClass* Class = LoadClass<URewardConcept03Widget>(nullptr, *Path);
        if (!TestNotNull(TEXT("Reward variant"), Class)) return false;
        URewardConcept03Widget* Widget = CreateWidget<URewardConcept03Widget>(World, Class);
        Widget->SetRewardPresentationManualTick(true);
        Widget->TakeWidget();
        auto Image = [Widget](const TCHAR* Name) { return Cast<UImage>(Widget->GetWidgetFromName(Name)); };
        UImage* Chest = Image(TEXT("NewChestSequenceImage"));
        UImage* Light = Image(TEXT("NewChestProceduralLight"));
        UImage* GoldChest = Image(TEXT("NewGoldBackgroundChestImage"));
        UImage* GoldLight = Image(TEXT("NewGoldProceduralLight"));
        UWidget* Flash = Widget->GetWidgetFromName(TEXT("NewRewardPresentationFlash"));
        if (!TestNotNull(TEXT("Chest"), Chest) || !TestNotNull(TEXT("Light"), Light)
            || !TestNotNull(TEXT("Gold chest"), GoldChest) || !TestNotNull(TEXT("Gold light"), GoldLight)
            || !TestNotNull(TEXT("Flash"), Flash)) return false;
        UMaterialInstanceDynamic* Cutout = Chest->GetDynamicMaterial();
        if (!TestNotNull(TEXT("Cutout material"), Cutout)) return false;
        for (const TCHAR* Parameter : { TEXT("Atlas"), TEXT("Silhouette") })
        {
            UTexture2D* Texture = Cast<UTexture2D>(Cutout->K2_GetTextureParameterValue(Parameter));
            if (!TestNotNull(Parameter, Texture)) return false;
            Texture->SetForceMipLevelsToBeResident(60.f);
            Texture->WaitForStreaming();
        }
        // Walk real time, including the short interval missed by the old captures.
        Widget->ResetRewardFlow();
        Widget->AdvanceRewardFlow();
        Widget->OpenRewardChest();
        for (int32 Tick = 0; Tick < 112; ++Tick)
        {
            TestEqual(TEXT("No rectangular flash during any opening tick"), Flash->GetRenderOpacity(), 0.f);
            Widget->AdvanceRewardPresentation(1.85f / 111.f);
        }
        TestEqual(TEXT("Opening reaches gold reward"), Widget->GetCurrentStepIndex(), 2);
        Widget->SkipRewardPresentation();
        TestTrue(TEXT("Gold retains its independent soft light"), GoldLight->GetRenderOpacity() > 0.f);
        for (const float Seconds : { 0.f, .65675f, .95f, 1.31f, 1.85f })
        {
            Widget->ResetRewardFlow();
            Widget->AdvanceRewardFlow();
            Widget->OpenRewardChest();
            Widget->AdvanceRewardPresentation(Seconds);
            const bool Gold = Widget->GetCurrentStepIndex() == 2;
            UImage& ActiveChest = *(Gold ? GoldChest : Chest);
            UImage& ActiveLight = *(Gold ? GoldLight : Light);
            for (int32 Mode = 0; Mode < 3; ++Mode)
            {
                TArray<FColor> Pixels;
                const FString Name = FString::Printf(TEXT("%s_%04d_%d"), Variant,
                    FMath::RoundToInt(Seconds * 1000.f), Mode);
                if (!TestTrue(TEXT("Layer capture"), RenderLayers(ActiveChest, ActiveLight, Mode,
                    FLinearColor(.18f, .18f, .18f, 1.f), Name, Pixels))) return false;
                const FColor Background = Pixels[0];
                int32 BoundaryChanges = 0;
                int32 DarkenedByLight = 0;
                int32 LitPixels = 0;
                for (int32 Y = 0; Y < Height; ++Y)
                    for (int32 X = 0; X < Width; ++X)
                    {
                        const FColor P = Pixels[Y * Width + X];
                        const int32 Delta = FMath::Abs(int32(P.R) - Background.R)
                            + FMath::Abs(int32(P.G) - Background.G) + FMath::Abs(int32(P.B) - Background.B);
                        if ((X < 3 || Y < 3 || X >= Width - 3 || Y >= Height - 3) && Delta > 3) ++BoundaryChanges;
                        if (Mode == 2 && (P.R + 1 < Background.R || P.G + 1 < Background.G || P.B + 1 < Background.B)) ++DarkenedByLight;
                        if (Mode == 2 && Delta > 12) ++LitPixels;
                    }
                TestEqual(TEXT("Every outer edge fades fully into background"), BoundaryChanges, 0);
                TestEqual(TEXT("Additive light never paints a dark rectangle"), DarkenedByLight, 0);
                // Regressions inside the source frame: its edges alone were insufficient.
                if (Mode == 1 && (Seconds == 0.f || Seconds == .95f))
                {
                    const int32 SourceY = Seconds == 0.f ? 96 : 350;
                    const int32 Y = FMath::RoundToInt(105.f + SourceY * 390.f / 455.f);
                    const FColor P = Pixels[Y * Width + Width / 2];
                    TestTrue(TEXT("Baked straight glow boundary is absent from chest-only layer"),
                        FMath::Abs(int32(P.R) - Background.R) + FMath::Abs(int32(P.G) - Background.G)
                            + FMath::Abs(int32(P.B) - Background.B) <= 3);
                }
                if (Mode == 2 && Seconds >= .95f) TestTrue(TEXT("Light actually rendered"), LitPixels > 1000);
            }
        }
        Widget->ResetRewardFlow();
        TestEqual(TEXT("Reset clears chest light"), Light->GetRenderOpacity(), 0.f);
        TestEqual(TEXT("Reset clears gold light"), GoldLight->GetRenderOpacity(), 0.f);
    }
    return true;
}
#endif
