#include "UI/StageVictory/BossCollapseWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/AudioComponent.h"
#include "Components/Border.h"
#include "Engine/Texture2D.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Rendering/DrawElements.h"
#include "Sound/SoundBase.h"
#include "Styling/CoreStyle.h"

namespace
{
    struct FCollapseTiming { float Detail, LandStart, Land, Rest, Duration; };
    constexpr FCollapseTiming Timings[] = {
        {.34f, .92f, 1.02f, 1.83f, 3.20f},
        {.52f, 1.34f, 1.48f, 2.30f, 3.80f},
        {.62f, 1.64f, 1.80f, 2.55f, 4.20f}};
    float Arrival(float Time, float Start, float Duration)
    {
        return FMath::Pow(1.f - FMath::Clamp((Time-Start)/Duration, 0.f, 1.f), 3.f);
    }
}

bool UBossCollapseWidget::ShouldPlay(bool bWon, bool bBossRoom, int32 Stage)
{
    return bWon && bBossRoom && Stage >= 1 && Stage <= 3;
}

bool UBossCollapseWidget::LoadStage(int32 InStage)
{
    if (!ShouldPlay(true, true, InStage)) return false;
    const FString Base = TEXT("/Game/SVN/OutSideAsset/AICreation/UI/StageVictory/BossCollapse/");
    Atlas = LoadObject<UTexture2D>(nullptr, *FString::Printf(TEXT("%sT_BossCollapse%d.T_BossCollapse%d"), *Base, InStage, InStage));
    Sound = LoadObject<USoundBase>(nullptr, *FString::Printf(TEXT("%sS_BossCollapse%d.S_BossCollapse%d"), *Base, InStage, InStage));
    if (!Atlas || !Sound) return false;
    StageNumber = InStage;
    Panels.Reset();
    // Match the five-pixel separator inset in the approved preview, including odd atlas sizes.
    // GetSizeX/Y can report a 32x32 compilation placeholder on a cold editor/game launch.
    // ImportedSize is stable both before resource compilation and in cooked builds.
    const FIntPoint ImportedSize = Atlas->GetImportedSize();
    if (ImportedSize.X <= 20 || ImportedSize.Y <= 20) return false;
    const float W = ImportedSize.X, H = ImportedSize.Y;
    for (int32 I = 0; I < 4; ++I)
    {
        FSlateBrush Brush;
        Brush.SetResourceObject(Atlas);
        Brush.DrawAs = ESlateBrushDrawType::Image;
        Brush.ImageSize = FVector2D(1280,720);
        const FVector2f Min((I%2*FMath::FloorToInt(W/2)+5)/W, (I/2*FMath::FloorToInt(H/2)+5)/H);
        Brush.SetUVRegion(FBox2f(Min, Min+FVector2f((FMath::FloorToInt(W/2)-10)/W,(FMath::FloorToInt(H/2)-10)/H)));
        Panels.Add(Brush);
    }
    return true;
}

UBossCollapseWidget* UBossCollapseWidget::Show(APlayerController* Player, int32 Stage, FSimpleDelegate OnFinished)
{
    if (!Player) return nullptr;
    auto* Widget = CreateWidget<UBossCollapseWidget>(Player);
    if (!Widget || !Widget->LoadStage(Stage)) return nullptr;
    Widget->Finished = MoveTemp(OnFinished);
    Widget->SetIsFocusable(true);
    Widget->AddToViewport(11000);
    FInputModeUIOnly Input;
    Input.SetWidgetToFocus(Widget->TakeWidget());
    Input.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    Player->SetInputMode(Input);
    Player->SetShowMouseCursor(false);
    Widget->Audio = UGameplayStatics::SpawnSound2D(Widget, Widget->Sound, 1.f, 1.f, 0.f, nullptr, false, true);
    UE_LOG(LogTemp, Display, TEXT("RD_BOSS_COLLAPSE start stage=%d duration=%.2f heroes=0"), Stage, Widget->GetDuration());
    return Widget;
}

TSharedRef<SWidget> UBossCollapseWidget::RebuildWidget()
{
    if (!WidgetTree->RootWidget)
    {
        auto* Root = WidgetTree->ConstructWidget<UBorder>();
        Root->SetBrushColor(FLinearColor::Black);
        Root->SetPadding(FMargin(0));
        WidgetTree->RootWidget = Root;
    }
    return Super::RebuildWidget();
}

float UBossCollapseWidget::GetDuration() const
{
    return StageNumber >= 1 && StageNumber <= 3 ? Timings[StageNumber-1].Duration : 0.f;
}

void UBossCollapseWidget::Advance(float Delta)
{
    if (bFinished || StageNumber == 0) return;
    Elapsed += FMath::Max(0.f, Delta);
    Invalidate(EInvalidateWidgetReason::Paint);
    if (Elapsed >= GetDuration())
    {
        bFinished = true;
        if (Audio) Audio->Stop();
        FSimpleDelegate Callback = MoveTemp(Finished);
        Finished.Unbind();
        UE_LOG(LogTemp, Display, TEXT("RD_BOSS_COLLAPSE finished stage=%d"), StageNumber);
        // Open rewards while the final comic frame still covers the battlefield.
        Callback.ExecuteIfBound();
        RemoveFromParent();
    }
}
void UBossCollapseWidget::NativeTick(const FGeometry& G, float Delta)
{
    Super::NativeTick(G,Delta);
    Advance(Delta);
}
void UBossCollapseWidget::Cancel()
{
    bFinished = true;
    Finished.Unbind();
    if (Audio) Audio->Stop();
    RemoveFromParent();
}
void UBossCollapseWidget::NativeDestruct()
{
    bFinished = true;
    Finished.Unbind();
    if (Audio) Audio->Stop();
    Super::NativeDestruct();
}
FReply UBossCollapseWidget::NativeOnKeyDown(const FGeometry&, const FKeyEvent&) { return FReply::Handled(); }
FReply UBossCollapseWidget::NativeOnMouseButtonDown(const FGeometry&, const FPointerEvent&) { return FReply::Handled(); }
FReply UBossCollapseWidget::NativeOnTouchStarted(const FGeometry&, const FPointerEvent&) { return FReply::Handled(); }

int32 UBossCollapseWidget::NativePaint(const FPaintArgs& Args, const FGeometry& G, const FSlateRect& Cull,
    FSlateWindowElementList& Out, int32 Layer, const FWidgetStyle& Style, bool Enabled) const
{
    Layer = Super::NativePaint(Args,G,Cull,Out,Layer,Style,Enabled)+1;
    if (Panels.Num()!=4 || StageNumber<1 || StageNumber>3) return Layer;
    const auto& T = Timings[StageNumber-1];
    const FVector2D View = G.GetLocalSize();
    const float Scale = FMath::Max(View.X/1280.f,View.Y/720.f);
    if (Scale <= 0.f) return Layer;
    FVector2D Shake = FVector2D::ZeroVector;
    auto AddImpact = [&](float At, float Amplitude)
    {
        if (Elapsed < At) return;
        const float Dt=Elapsed-At, Env=FMath::Exp(-15.f*Dt);
        Shake += FVector2D(.65f*Amplitude*Env*FMath::Sin(99.f*Dt),Amplitude*Env*FMath::Cos(87.f*Dt));
    };
    if (StageNumber==1) { AddImpact(.42f,6); AddImpact(T.Land,17); }
    if (StageNumber==2) { AddImpact(.10f,9); AddImpact(.62f,5); AddImpact(T.Land,22); }
    if (StageNumber==3) { AddImpact(1.12f,3); AddImpact(T.Land,8); }
    const float SceneScale = Scale*1.05f;
    const FVector2D Origin = (View-FVector2D(1280,720)*SceneScale)*.5f-Shake*Scale;
    const FSlateBrush* White = FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"));
    Out.PushClip(FSlateClippingZone(G));
    auto Draw = [&](int32 Index, FVector2D Pos, FVector2D Size, float Angle, bool Border)
    {
        const auto Geometry=G.ToPaintGeometry(Size*SceneScale,FSlateLayoutTransform(Origin+Pos*SceneScale));
        if (Border)
            FSlateDrawElement::MakeRotatedBox(Out,Layer++,Geometry,White,ESlateDrawEffect::None,Angle,TOptional<FVector2D>(),FSlateDrawElement::RelativeToElement,FLinearColor(.025f,.03f,.02f));
        const FVector2D Inset = Border ? FVector2D(9,9) : FVector2D::ZeroVector;
        FSlateBrush Brush = Panels[Index];
        // Crop to the panel's aspect ratio rather than stretching the boss.
        const FVector2D Inner = Size-Inset*2;
        FBox2f UV=Brush.GetUVRegion();
        const float Ratio=Inner.X/Inner.Y;
        if (Ratio>1280.f/720.f) { const float Remove=(UV.Max.Y-UV.Min.Y)*(1.f-(1280.f/720.f)/Ratio)*.5f; UV.Min.Y+=Remove;UV.Max.Y-=Remove; }
        else { const float Remove=(UV.Max.X-UV.Min.X)*(1.f-Ratio/(1280.f/720.f))*.5f; UV.Min.X+=Remove;UV.Max.X-=Remove; }
        Brush.SetUVRegion(UV);
        FSlateDrawElement::MakeRotatedBox(Out,Layer++,G.ToPaintGeometry(Inner*SceneScale,FSlateLayoutTransform(Origin+(Pos+Inset)*SceneScale)),&Brush,ESlateDrawEffect::None,Angle,TOptional<FVector2D>(),FSlateDrawElement::RelativeToElement,FLinearColor::White);
    };
    Draw(0,{0,0},{1280,720},0,false);
    if (Elapsed>=T.Detail && Elapsed<T.Land)
    {
        if (StageNumber==1) Draw(1,{195,195-850*Arrival(Elapsed,T.Detail,.08f)},{1110,570},-.055f,true);
        if (StageNumber==2) Draw(1,{200+1450*Arrival(Elapsed,T.Detail,.10f),20},{1090,595},.045f,true);
        if (StageNumber==3) Draw(1,{175+1450*Arrival(Elapsed,T.Detail,.10f),150},{1130,615},-.075f,true);
    }
    if (Elapsed>=T.LandStart && Elapsed<T.Rest)
        Draw(2,{0,-850*Arrival(Elapsed,T.LandStart,T.Land-T.LandStart)},{1280,720},0,false);
    if (Elapsed>=T.Rest) Draw(3,{0,0},{1280,720},0,false);
    Out.PopClip();
    return Layer;
}
