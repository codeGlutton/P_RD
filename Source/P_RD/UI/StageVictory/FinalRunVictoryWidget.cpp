#include "UI/StageVictory/FinalRunVictoryWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/ScaleBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Engine/Font.h"
#include "Engine/Texture2D.h"
#include "UObject/ConstructorHelpers.h"

TSharedRef<SWidget> UFinalRunVictoryWidget::RebuildWidget()
{
    if (!WidgetTree->RootWidget)
    {
        auto *Root = WidgetTree->ConstructWidget<UBorder>();
        Root->SetBrushColor(FLinearColor::Black);
        Root->SetPadding(FMargin(0));
        WidgetTree->RootWidget = Root;
        auto *Scale = WidgetTree->ConstructWidget<UScaleBox>();
        Scale->SetStretch(EStretch::ScaleToFit);
        Root->AddChild(Scale);
        auto *Size = WidgetTree->ConstructWidget<USizeBox>();
        Size->SetWidthOverride(1920);
        Size->SetHeightOverride(1080);
        Scale->AddChild(Size);
        auto *Canvas = WidgetTree->ConstructWidget<UCanvasPanel>();
        Size->AddChild(Canvas);
        auto Place = [Canvas](UWidget *W, FVector2D Pos, FVector2D Extent) {
            auto *S = Canvas->AddChildToCanvas(W);
            S->SetPosition(Pos);
            S->SetSize(Extent);
        };
        auto *Background = WidgetTree->ConstructWidget<UImage>();
        Background->SetBrushFromTexture(LoadObject<UTexture2D>(
            nullptr, TEXT("/Game/SVN/OutSideAsset/AICreation/UI/StageVictory/T_StageReward3.T_StageReward3")));
        Background->SetVisibility(ESlateVisibility::HitTestInvisible);
        Place(Background, {0, 0}, {1920, 1080});
        auto *Shade = WidgetTree->ConstructWidget<UBorder>();
        Shade->SetBrushColor(FLinearColor(0.008f, 0.006f, 0.012f, .68f));
        Place(Shade, {0, 0}, {1920, 1080});
        auto *Edge = WidgetTree->ConstructWidget<UBorder>();
        Edge->SetBrushColor(FLinearColor(.6f, .38f, .12f));
        Place(Edge, {400, 285}, {1120, 510});
        auto *Panel = WidgetTree->ConstructWidget<UBorder>();
        Panel->SetBrushColor(FLinearColor(.027f, .02f, .015f, .97f));
        Place(Panel, {403, 288}, {1114, 504});
        auto *Font =
            LoadObject<UFont>(nullptr, TEXT("/Game/SVN/OutSideAsset/Fonts/GowunBatang/F_GowunBatang.F_GowunBatang"));
        auto Text = [&](const TCHAR *Name, FText Copy, int32 Points, FVector2D Pos, FVector2D Extent,
                        FLinearColor Color) {
            auto *W = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
            W->SetText(Copy);
            W->SetFont(FSlateFontInfo(Font, Points));
            W->SetColorAndOpacity(Color);
            W->SetJustification(ETextJustify::Center);
            W->SetAutoWrapText(true);
            W->SetVisibility(ESlateVisibility::HitTestInvisible);
            Place(W, Pos, Extent);
            return W;
        };
        Text(TEXT("FinalVictoryTitle"), NSLOCTEXT("FinalRunVictory", "Title", "왕국을 구했습니다"), 52, {440, 335},
             {1040, 90}, FLinearColor(1, .78f, .38f));
        Text(TEXT("FinalVictoryMessage"),
             NSLOCTEXT("FinalRunVictory", "Message",
                       "네크로맨서를 쓰러뜨리고 모든 스테이지를 클리어했습니다.\n용병단과 함께해 주셔서 감사합니다."),
             27, {470, 462}, {980, 125}, FLinearColor::White);
        ContinueButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("FinalVictoryContinue"));
        ContinueButton->SetBackgroundColor(FLinearColor(.38f, .24f, .075f));
        Place(ContinueButton, {660, 654}, {600, 92});
        auto *Label =
            WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("FinalVictoryContinueText"));
        Label->SetText(NSLOCTEXT("FinalRunVictory", "Continue", "타이틀로 돌아가기"));
        Label->SetFont(FSlateFontInfo(Font, 30));
        Label->SetJustification(ETextJustify::Center);
        ContinueButton->AddChild(Label);
        ContinueButton->OnClicked.AddDynamic(this, &UFinalRunVictoryWidget::Confirm);
    }
    return Super::RebuildWidget();
}
void UFinalRunVictoryWidget::Confirm()
{
    if (!Continue.IsBound())
        return;
    if (ContinueButton)
        ContinueButton->SetIsEnabled(false);
    FSimpleDelegate Callback = MoveTemp(Continue);
    Continue.Unbind();
    Callback.ExecuteIfBound();
}
