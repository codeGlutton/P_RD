#include "Misc/AutomationTest.h"
#include "UI/Combat/CombatWorldWidgetPosition.h"
#include "UI/RDHUDScalingRule.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Slate/WidgetRenderer.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SDPIScaler.h"
#include "RHI.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatWorldWidgetPositionTest,
    "P_RD.UI.CombatHUD.WorldPositionSafeAreaAndDPI",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatWorldWidgetPositionTest::RunTest(const FString&)
{
    const auto* Rule = GetDefault<URDHUDScalingRule>();
    int32 Cases = 0;
    for (const FIntPoint Resolution : {FIntPoint(1280,720), FIntPoint(1920,1080),
        FIntPoint(2400,1080), FIntPoint(3200,1440), FIntPoint(2560,1920), FIntPoint(1296,1080)})
    for (float ApplicationScale : {1.f, 1.5f, 2.f})
    for (const FVector2D InsetPixels : {FVector2D::ZeroVector, FVector2D(96,0), FVector2D(0,64), FVector2D(96,48)})
    for (float ParentScale : {1.f, .8f})
    {
        const float Dpi = Rule->GetDPIScaleBasedOnSize(Resolution) * ApplicationScale;
        const FVector2D Size(Resolution.X, Resolution.Y);
        const FVector2D DesktopOrigin(157,91); // Windowed/high-DPI desktop as well as phones.
        const FGeometry Viewport = FGeometry::MakeRoot(Size / Dpi,
            FSlateLayoutTransform(Dpi, DesktopOrigin));
        const FGeometry Canvas = Viewport.MakeChild((Size-InsetPixels) / (Dpi*ParentScale),
            FSlateLayoutTransform(ParentScale, InsetPixels / Dpi));
        for (const FVector2D Fraction : {FVector2D(.2,.3), FVector2D(.5,.5), FVector2D(.8,.7)})
        {
            const FVector2D Pixel = Size * Fraction;
            const FVector2D ProjectedWidget = Pixel / Dpi;
            const FVector2D Fixed = CombatWorldWidgetPosition::ViewportToCanvas(ProjectedWidget, Viewport, Canvas);
            const FVector2D ExpectedLocal = (Pixel-InsetPixels) / (Dpi*ParentScale);
            TestTrue(TEXT("Projection is converted to the canvas's local space"), Fixed.Equals(ExpectedLocal, .01));
            TestTrue(TEXT("Placed anchor stays above the same unit pixel"),
                Canvas.LocalToAbsolute(Fixed).Equals(DesktopOrigin+Pixel, .01));
            if (ParentScale == 1.f && !InsetPixels.IsNearlyZero())
            {
                const FVector2D OldError = Canvas.LocalToAbsolute(ProjectedWidget) - (DesktopOrigin+Pixel);
                TestTrue(TEXT("Old direct placement reproduces the safe-area displacement"), OldError.Equals(InsetPixels,.01));
            }
            ++Cases;
        }
    }
    AddInfo(FString::Printf(TEXT("Checked %d placements across 6 resolutions, DPI scales, safe-area insets and parent scales."),Cases));
    return !HasAnyErrors();
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatWorldHpCanvasLayoutTest,
    "P_RD.UI.CombatHUD.WorldHpCanvasLayout",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatWorldHpCanvasLayoutTest::RunTest(const FString&)
{
    if (GUsingNullRHI) { AddWarning(TEXT("Actual Slate arrangement needs RHI")); return true; }
    const auto* Rule = GetDefault<URDHUDScalingRule>();
    for (const FIntPoint Resolution : {FIntPoint(1280,720), FIntPoint(1920,1080),
        FIntPoint(2400,1080), FIntPoint(3200,1440), FIntPoint(2560,1920), FIntPoint(1296,1080)})
    {
        const float Dpi = Rule->GetDPIScaleBasedOnSize(Resolution);
        const FVector2D Size(Resolution.X,Resolution.Y);
        const FVector2D Inset(96,48);
        auto* Canvas = NewObject<UCanvasPanel>();
        auto* Bar = NewObject<UBorder>(Canvas);
        auto* Slot = Canvas->AddChildToCanvas(Bar);
        Slot->SetSize(FVector2D(360,68));
        Slot->SetAlignment(FVector2D(.5,1));
        Bar->SetRenderTransformPivot(FVector2D(.5,1));
        Bar->SetRenderScale(FVector2D(.52));
        auto Slate = SNew(SDPIScaler).DPIScale(Dpi)
            [SNew(SBorder).Padding(FMargin(Inset.X/Dpi,Inset.Y/Dpi,0,0))
                [Canvas->TakeWidget()]];
        const FVector2D UnitPixel = Size * FVector2D(.6,.55);
        const FVector2D ProjectedWidget = UnitPixel / Dpi;
        const FGeometry Viewport = FGeometry::MakeRoot(Size/Dpi,FSlateLayoutTransform(Dpi));
        Slot->SetPosition(ProjectedWidget + FVector2D(0,-64));
        FWidgetRenderer Renderer(true,true);
        Renderer.DrawWidget(Slate,Size);
        FlushRenderingCommands();
        const FVector2D ExpectedAnchor = UnitPixel+FVector2D(0,-64*Dpi);
        const FVector2D OldAnchor = Bar->GetCachedGeometry().LocalToAbsolute(FVector2D(180,68));
        TestTrue(TEXT("Real canvas reproduces the old displaced bar"),
            (OldAnchor-ExpectedAnchor).Equals(Inset,.1));
        Slot->SetPosition(CombatWorldWidgetPosition::ViewportToCanvas(ProjectedWidget,
            Viewport,Canvas->GetCachedGeometry())+FVector2D(0,-64));
        Renderer.DrawWidget(Slate,Size);
        FlushRenderingCommands();
        const FVector2D FixedAnchor = Bar->GetCachedGeometry().LocalToAbsolute(FVector2D(180,68));
        TestTrue(TEXT("Rendered HP plate anchor aligns after safe-area conversion"),FixedAnchor.Equals(ExpectedAnchor,.1));
    }
    return !HasAnyErrors();
}
#endif
