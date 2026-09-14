#include "UI/Combat/CombatSpeedWidget.h"
#include "UI/Combat/CombatUIModel.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "Engine/Texture2D.h"
#include "UObject/ConstructorHelpers.h"

UCombatSpeedWidget::UCombatSpeedWidget(const FObjectInitializer& Initializer) : Super(Initializer)
{
    static ConstructorHelpers::FObjectFinder<UTexture2D> Art(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/KitA/T_KitA_Button_Small_Normal.T_KitA_Button_Small_Normal"));
    Frame = Art.Object;
}

bool UCombatSpeedWidget::Initialize()
{
    const bool Result = Super::Initialize();
    if (!Result || WidgetTree->RootWidget) return Result;
    Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("CombatSpeedButton"));
    Label = WidgetTree->ConstructWidget<UTextBlock>();
    auto Font = FCoreStyle::GetDefaultFontStyle("Bold", 24);
    Font.OutlineSettings.OutlineSize = 2;
    Font.OutlineSettings.OutlineColor = FLinearColor::Black;
    Label->SetFont(Font);
    Label->SetColorAndOpacity(FLinearColor(1.f, .9f, .67f));
    Label->SetShadowOffset(FVector2D(1, 2));
    Label->SetJustification(ETextJustify::Center);
    Label->SetMargin(FMargin(30, 0, 0, 0));
    Label->SetVisibility(ESlateVisibility::HitTestInvisible);
    Button->AddChild(Label);
    WidgetTree->RootWidget = Button;
    FSlateBrush Brush;
    Brush.SetResourceObject(Frame);
    Brush.DrawAs = ESlateBrushDrawType::Box;
    Brush.Margin = FMargin(.2f);
    FButtonStyle ButtonStyle;
    ButtonStyle.SetNormal(Brush);
    Brush.TintColor = FLinearColor(1.15f, 1.05f, .85f); ButtonStyle.SetHovered(Brush);
    Brush.TintColor = FLinearColor(.72f,.65f,.5f); ButtonStyle.SetPressed(Brush);
    Brush.TintColor = FLinearColor(.5f,.5f,.5f); ButtonStyle.SetDisabled(Brush);
    ButtonStyle.SetNormalPadding(FMargin(4)); ButtonStyle.SetPressedPadding(FMargin(4,6,4,2));
    Button->SetStyle(ButtonStyle);
    Button->OnClicked.AddDynamic(this, &UCombatSpeedWidget::Clicked);
    return Result;
}

void UCombatSpeedWidget::Configure(UCombatUIModel* Model)
{
    ViewModel = Model;
    if (!Button) return;
    const int32 Speed = Model ? Model->GetPlaybackSpeed() : 1;
    Label->SetText(FText::FromString(FString::Printf(TEXT("%dx"), Speed)));
    Button->SetIsEnabled(Model && Model->IsPlaybackSpeedAvailable());
    Button->SetBackgroundColor(Speed == 1 ? FLinearColor(.22f,.18f,.12f) : FLinearColor(.4f,.3f,.14f));
    SetToolTipText(FText::Format(NSLOCTEXT("CombatHUD", "PlaybackSpeed", "전투 배속 {0}배 · 누르면 1 → 2 → 3배 전환"), Speed));
}

void UCombatSpeedWidget::Clicked()
{
    if (ViewModel.IsValid()) ViewModel->RequestCyclePlaybackSpeed();
}

int32 UCombatSpeedWidget::NativePaint(const FPaintArgs& Args, const FGeometry& Geometry, const FSlateRect& CullingRect,
    FSlateWindowElementList& Elements, int32 Layer, const FWidgetStyle& Style, bool Enabled) const
{
    const int32 Top = Super::NativePaint(Args, Geometry, CullingRect, Elements, Layer, Style, Enabled);
    const float Y = Geometry.GetLocalSize().Y * .5f;
    // Draw chevrons as geometry: no icon font or uncooked texture dependency.
    const FLinearColor Color = FLinearColor(1.f, .9f, .67f) * Style.GetColorAndOpacityTint();
    for (float X : {18.f, 30.f})
    {
        TArray<FVector2D> Points = { FVector2D(X,Y-9), FVector2D(X+9,Y), FVector2D(X,Y+9) };
        FSlateDrawElement::MakeLines(Elements, Top+1, Geometry.ToPaintGeometry(), Points,
            ESlateDrawEffect::None, Color, true, 3.f);
    }
    return Top+1;
}
