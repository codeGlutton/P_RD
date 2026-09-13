#include "UI/Tutorial/GuidedTutorialWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Components/SizeBox.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Button.h"
#include "Engine/Font.h"
#include "Engine/Texture2D.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Brushes/SlateColorBrush.h"
#include "Rendering/DrawElements.h"
#include "UObject/ConstructorHelpers.h"
namespace
{
const FLinearColor Ink(.105f, .063f, .028f, 1), Gold(.9f, .61f, .15f, 1);
bool Holding(EGuidedStage S)
{
	return S == EGuidedStage::HoldSkill || S == EGuidedStage::HoldMonsterSkill;
}
FText Topic(EGuidedStage S)
{
	switch (S)
	{
	case EGuidedStage::ReadTurnOrder: return NSLOCTEXT("FieldGuideTopic", "Order", "턴과 라운드");
	case EGuidedStage::ReadAP: return NSLOCTEXT("FieldGuideTopic", "AP", "행동력 · AP");
	case EGuidedStage::ReadCondition: return NSLOCTEXT("FieldGuideTopic", "Condition", "컨디션");
	case EGuidedStage::ReadStatus: return NSLOCTEXT("FieldGuideTopic", "Status", "상태이상");
	case EGuidedStage::ReadMercenaryStats: return NSLOCTEXT("FieldGuideTopic", "Stats", "용병 능력치");
	case EGuidedStage::ReadSkillCost: return NSLOCTEXT("FieldGuideTopic", "Cost", "스킬 비용");
	case EGuidedStage::ReadSkillCooldown: return NSLOCTEXT("FieldGuideTopic", "Cooldown", "스킬 쿨타임");
	case EGuidedStage::SelectRange: case EGuidedStage::EffectRange: return NSLOCTEXT("FieldGuideTopic", "Range", "사거리와 효과 범위");
	case EGuidedStage::ReadInventory: return NSLOCTEXT("FieldGuideTopic", "Inventory", "인벤토리 · 아티팩트");
	case EGuidedStage::ReadEnemy: case EGuidedStage::ReadEnemySkill: return NSLOCTEXT("FieldGuideTopic", "Enemy", "적 정보");
	default: break;
	}
	if (S == EGuidedStage::OpenSkills || (S >= EGuidedStage::SelectMove && S <= EGuidedStage::ConfirmMove))
		return NSLOCTEXT("TutorialBubble", "Move", "이동");
	if (S < EGuidedStage::EndTurn)
		return NSLOCTEXT("TutorialBubble", "Skill", "스킬");
	if (S == EGuidedStage::EndTurn)
		return NSLOCTEXT("TutorialBubble", "Turn", "턴 종료");
	return NSLOCTEXT("TutorialBubble", "Info", "정보 확인");
}
} // namespace
UGuidedTutorialWidget::UGuidedTutorialWidget(const FObjectInitializer& I) : Super(I)
{
	static ConstructorHelpers::FObjectFinder<UFont> F(
	    TEXT("/Game/SVN/OutSideAsset/Fonts/F_HUD_Oswald.F_HUD_Oswald"));
	Font = F.Object;
	static ConstructorHelpers::FObjectFinder<UTexture2D> P(
	    TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ResultBoards/AtomicV11/"
	         "T_R11_ParchmentSheet.T_R11_ParchmentSheet"));
	Parchment = P.Object;
}
FText UGuidedTutorialWidget::Instruction(EGuidedStage S)
{
	switch (S)
	{
	case EGuidedStage::ReadTurnOrder:
		return NSLOCTEXT("FieldGuide", "TurnOrder", "왼쪽부터 행동할 차례입니다.\nR은 라운드 번호이며, 화살표로 이후 순서를 볼 수 있습니다.");
	case EGuidedStage::ReadAP:
		return NSLOCTEXT("FieldGuide", "ActionPoints", "AP는 행동에 쓰는 자원입니다.\n이동과 스킬에 필요한 AP를 확인하고 사용하세요.");
	case EGuidedStage::ReadCondition:
		return NSLOCTEXT("FieldGuide", "Condition", "표정은 컨디션입니다.\n나쁨, 보통, 좋음, 매우 좋음 순으로 피해량이 증가하며, 매우 좋음 상태에서는 치명타가 적용됩니다.");
	case EGuidedStage::ReadStatus:
		return NSLOCTEXT("FieldGuide", "StatusIcons", "상태이상이 생기면 요약판에 아이콘이 표시됩니다.\n아이콘을 누르면 효과를 확인할 수 있습니다.");
	case EGuidedStage::ReadMercenaryStats:
		return NSLOCTEXT("FieldGuide", "MercenaryStats", "HP는 남은 체력, AP는 행동 자원입니다.\n속도는 행동 순서에 영향을 줍니다.");
	case EGuidedStage::ReadSkillCost:
		return NSLOCTEXT("FieldGuide", "SkillCost", "이 스킬을 쓰는 데 필요한 AP입니다.\n남은 AP가 부족하면 사용할 수 없습니다.");
	case EGuidedStage::ReadSkillCooldown:
		return NSLOCTEXT("FieldGuide", "SkillCooldown", "쿨타임은 다시 쓰기까지의 대기 시간입니다.\n사용 후 스킬 카드에 남은 수가 표시됩니다.");
	case EGuidedStage::SelectRange:
		return NSLOCTEXT("FieldGuide", "SelectRange", "사거리를 눌러 보세요.\n대상을 선택할 수 있는 범위를 표시합니다.");
	case EGuidedStage::EffectRange:
		return NSLOCTEXT("FieldGuide", "EffectRange", "효과 범위를 눌러 보세요.\n선택한 대상 주변의 영향 범위를 표시합니다.");
	case EGuidedStage::ReadInventory:
		return NSLOCTEXT("FieldGuide", "InventoryContents", "획득한 아티팩트는 여기에 모입니다.\n아이콘을 누르면 효과를 볼 수 있습니다. 아직 없다면 다음으로 넘어갑니다.");
	case EGuidedStage::ReadEnemy:
		return NSLOCTEXT("FieldGuide", "EnemyStats", "적의 체력·AP·속도를 확인할 수 있습니다.\n공격 전에 아래 스킬도 살펴보세요.");
	case EGuidedStage::ReadEnemySkill:
		return NSLOCTEXT("FieldGuide", "EnemySkill", "적 스킬도 비용과 범위를 확인할 수 있습니다.\n설명의 피해와 상태이상 효과를 보고 위치를 정하세요.");
	case EGuidedStage::OpenSkills:
		return NSLOCTEXT("FieldGuide", "Open", "행동 카드를 펼치세요.");
	case EGuidedStage::SelectMove:
		return NSLOCTEXT("FieldGuide", "Move", "이동을 선택하세요.");
	case EGuidedStage::MoveTile:
		return NSLOCTEXT("FieldGuide", "Tile", "이동할 칸을 선택하세요.");
	case EGuidedStage::ConfirmMove:
		return NSLOCTEXT("FieldGuide", "MoveConfirm", "경로와 AP를 확인하고 이동하세요.");
	case EGuidedStage::HoldSkill:
		return NSLOCTEXT("FieldGuide", "Hold", "스킬을 길게 눌러\n설명을 확인하세요.");
	case EGuidedStage::CloseSkill:
	case EGuidedStage::CloseMercenarySkill:
	case EGuidedStage::CloseMonsterSkill:
		return NSLOCTEXT("FieldGuide", "Read", "설명을 확인한 후 닫으세요.");
	case EGuidedStage::SelectSkill:
		return NSLOCTEXT("FieldGuide", "Select", "스킬을 짧게 눌러 선택하세요.");
	case EGuidedStage::SkillTarget:
		return NSLOCTEXT("FieldGuide", "Target", "공격할 대상을 선택하세요.");
	case EGuidedStage::ConfirmSkill:
		return NSLOCTEXT("FieldGuide", "Attack", "예상 효과를 확인하고 공격하세요.");
	case EGuidedStage::EndTurn:
		return NSLOCTEXT("FieldGuide", "End", "턴 종료를 누르세요.");
	case EGuidedStage::LessonMenu:
		return NSLOCTEXT("FieldGuide", "Free", "기본 조작 안내가 끝났습니다.");
	case EGuidedStage::OpenMercenary:
		return NSLOCTEXT("FieldGuide", "MercOpen", "용병 정보 버튼을 누르세요.");
	case EGuidedStage::SelectMercenary:
		return NSLOCTEXT("FieldGuide", "MercSelect", "용병을 선택하세요.");
	case EGuidedStage::MercenarySkill:
		return NSLOCTEXT("FieldGuide", "MercSkill", "스킬을 눌러 설명을 확인하세요.");
	case EGuidedStage::OpenInventory:
		return NSLOCTEXT("FieldGuide", "Inventory", "인벤토리를 여세요.");
	case EGuidedStage::Artifact:
		return NSLOCTEXT("FieldGuide", "Artifact", "아티팩트를 눌러 효과를 확인하세요.");
	case EGuidedStage::CloseArtifact:
		return NSLOCTEXT("FieldGuide", "ArtifactClose", "효과를 확인한 후 닫으세요.");
	case EGuidedStage::CloseMercenary:
	case EGuidedStage::CloseMonster:
		return NSLOCTEXT("FieldGuide", "Return", "닫기를 눌러 전투로 돌아가세요.");
	case EGuidedStage::OpenMonster:
		return NSLOCTEXT("FieldGuide", "MonsterOpen", "몬스터 정보 버튼을 누르세요.");
	case EGuidedStage::SelectMonster:
		return NSLOCTEXT("FieldGuide", "MonsterSelect", "몬스터를 선택하세요.");
	case EGuidedStage::HoldMonsterSkill:
		return NSLOCTEXT("FieldGuide", "MonsterHold", "스킬을 길게 눌러\n설명을 확인하세요.");
	default:
		return FText::GetEmpty();
	}
}

TSharedRef<SWidget> UGuidedTutorialWidget::RebuildWidget()
{
	Canvas = WidgetTree->ConstructWidget<UCanvasPanel>();
	WidgetTree->RootWidget = Canvas;
	Canvas->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	Card = WidgetTree->ConstructWidget<USizeBox>();
	Card->SetWidthOverride(460);
	auto* Plate = WidgetTree->ConstructWidget<UBorder>();
	FSlateBrush Paper;
	Paper.SetResourceObject(Parchment);
	Paper.DrawAs = ESlateBrushDrawType::Image;
	Paper.ImageSize = FVector2D(460, 180);
	Plate->SetBrush(Parchment ? Paper : FSlateRoundedBoxBrush(FLinearColor(.83f, .67f, .40f), 10.f));
	Plate->SetBrushColor(FLinearColor::White);
	Plate->SetPadding(FMargin(30, 18, 28, 24));
	Card->AddChild(Plate);
	auto* VBox = WidgetTree->ConstructWidget<UVerticalBox>();
	Plate->AddChild(VBox);
	auto MakeText = [this](int Size) {
		auto* T = WidgetTree->ConstructWidget<UTextBlock>();
		T->SetFont(FSlateFontInfo(Font, Size));
		T->SetColorAndOpacity(Ink);
		T->SetVisibility(ESlateVisibility::HitTestInvisible);
		return T;
	};
	auto* Header = WidgetTree->ConstructWidget<UHorizontalBox>();
	VBox->AddChildToVerticalBox(Header);
	Counter = MakeText(18);
	auto* TitleSlot = Header->AddChildToHorizontalBox(Counter);
	TitleSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	TitleSlot->SetVerticalAlignment(VAlign_Center);
	Label = MakeText(30);
	Label->SetAutoWrapText(true);
	VBox->AddChildToVerticalBox(Label)->SetPadding(FMargin(0, 0, 0, 4));
	GestureText = MakeText(18);
	GestureText->SetText(NSLOCTEXT("TutorialBubble", "Hold", "길게 누르기"));
	VBox->AddChildToVerticalBox(GestureText)->SetPadding(FMargin(0, 7, 0, 0));
	ContinueButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("GuideContinueButton"));
	FButtonStyle ContinueStyle;
	ContinueStyle.SetNormal(FSlateRoundedBoxBrush(FLinearColor(.22f, .12f, .045f), 5.f));
	ContinueStyle.SetHovered(FSlateRoundedBoxBrush(FLinearColor(.33f, .20f, .08f), 5.f));
	ContinueStyle.SetPressed(FSlateRoundedBoxBrush(FLinearColor(.15f, .08f, .025f), 5.f));
	ContinueStyle.SetNormalPadding(FMargin(24, 9));
	ContinueStyle.SetPressedPadding(FMargin(24, 9));
	ContinueButton->SetStyle(ContinueStyle);
	ContinueLabel = MakeText(22);
	ContinueLabel->SetColorAndOpacity(FLinearColor(.98f, .91f, .73f));
	ContinueLabel->SetText(NSLOCTEXT("FieldGuide", "Continue", "확인"));
	ContinueButton->AddChild(ContinueLabel);
	auto* Footer = WidgetTree->ConstructWidget<UHorizontalBox>();
	DismissButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("GuideDismissButton"));
	DismissButton->SetStyle(ContinueStyle);
	auto* DismissLabel = MakeText(22);
	DismissLabel->SetColorAndOpacity(FLinearColor(.98f, .91f, .73f));
	DismissLabel->SetText(NSLOCTEXT("FieldGuide", "CloseReading", "안내 닫기"));
	DismissButton->AddChild(DismissLabel);
	DismissButton->SetVisibility(ESlateVisibility::Collapsed);
	Footer->AddChildToHorizontalBox(DismissButton)->SetPadding(FMargin(0, 0, 12, 0));
	Footer->AddChildToHorizontalBox(ContinueButton);
	auto* ContinueSlot = VBox->AddChildToVerticalBox(Footer);
	ContinueSlot->SetHorizontalAlignment(HAlign_Right);
	ContinueSlot->SetPadding(FMargin(0, 12, 0, 0));
	auto* CS = Canvas->AddChildToCanvas(Card);
	CS->SetAutoSize(true);
	if (!ReadingTitle.IsEmpty()) PresentReading(ReadingTitle, ReadingDescription, TargetWidget.Get(), bLastReading);
	else if (!EncounterTitle.IsEmpty()) PresentEncounter(EncounterTitle, EncounterDescription);
	else Present(CurrentStage, TargetWidget.Get(), bBoard);
	return Super::RebuildWidget();
}
void UGuidedTutorialWidget::Present(EGuidedStage S, UWidget* Target, bool BoardTarget, bool Retry)
{
	if (CurrentStage != S || TargetWidget.Get() != Target)
		GestureTime = 0;
	CurrentStage = S;
	TargetWidget = Target;
	bBoard = BoardTarget;
	if (!Label)
		return;
	Counter->SetText(Topic(S));
	const bool Reading = FGuidedTutorialProgress::IsReadingStep(S);
	ContinueButton->SetVisibility(Reading ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	Label->SetFont(FSlateFontInfo(Font, Reading ? 26 : 30));
	Label->SetText(Retry ? NSLOCTEXT("TutorialBubble", "Retry", "취소한 후 스킬을 길게 누르세요.")
	                     : Instruction(S));
	GestureText->SetVisibility(Holding(S) && !Retry ? ESlateVisibility::HitTestInvisible
	                                                : ESlateVisibility::Collapsed);
}
void UGuidedTutorialWidget::NativeTick(const FGeometry& G, float Delta)
{
	Super::NativeTick(G, Delta);
	GestureTime += Delta;
	const auto Size = G.GetLocalSize();
	if (!Card || Size.X < 1)
		return;
	double PreferredWidth = 460.;
	// A shop icon can sit in the middle of a short landscape screen. Fit the
	// callout beside that icon instead of clamping a tall card over its spotlight.
	if (!ReadingTitle.IsEmpty() && TargetWidget.IsValid())
	{
		const auto& TG = TargetWidget->GetCachedGeometry();
		const auto TA = G.AbsoluteToLocal(TG.LocalToAbsolute(FVector2D::ZeroVector));
		const auto TB = G.AbsoluteToLocal(TG.LocalToAbsolute(TG.GetLocalSize()));
		const double SideSpace = FMath::Max(TA.X, Size.X - TB.X) - 108. - 28.;
		PreferredWidth = FMath::Clamp(SideSpace, 280., 460.);
	}
	const double Width = FMath::Min(PreferredWidth, Size.X - 48.);
	Card->SetWidthOverride(Width);
	const double Height = FMath::Max(120., Card->GetDesiredSize().Y);
	const double Pad = 24., Gap = 108.;
	FVector2D Pos(Pad, FMath::Max(Pad, Size.Y - Height - 145.));
	bFocus =
	    !bBoard && TargetWidget.IsValid() && !TargetWidget->GetCachedGeometry().GetLocalSize().IsNearlyZero();
	if (bBoard && WorldFocus.Num() == 4)
	{
		FocusA = FVector2D(DBL_MAX, DBL_MAX);
		FocusB = FVector2D(-DBL_MAX, -DBL_MAX);
		for (const auto& Point : WorldFocus)
		{
			const auto Local = G.AbsoluteToLocal(Point);
			FocusA.X = FMath::Min(FocusA.X, Local.X); FocusA.Y = FMath::Min(FocusA.Y, Local.Y);
			FocusB.X = FMath::Max(FocusB.X, Local.X); FocusB.Y = FMath::Max(FocusB.Y, Local.Y);
		}
		bFocus = true;
	}
	if (bFocus)
	{
		if (!bBoard)
		{
		const auto& T = TargetWidget->GetCachedGeometry();
		FocusA = G.AbsoluteToLocal(T.LocalToAbsolute(FVector2D::ZeroVector));
		FocusB = G.AbsoluteToLocal(T.LocalToAbsolute(T.GetLocalSize()));
		}
		const FVector2D Center = (FocusA + FocusB) * .5;
		// Keep the short callout next to its actual control and inside the viewport.
		if (FocusA.X > Width + Gap + Pad)
			Pos = {FocusA.X - Width - Gap, Center.Y - Height * .5};
		else if (Size.X - FocusB.X > Width + Gap + Pad)
			Pos = {FocusB.X + Gap, Center.Y - Height * .5};
		else if (FocusA.Y > Height + Gap + Pad)
			Pos = {Center.X - Width * .5, FocusA.Y - Height - Gap};
		else
			Pos = {Center.X - Width * .5, FocusB.Y + Gap};
	}
	// Keep the tactical diagram visible while the player compares its two range modes.
	if (CurrentStage == EGuidedStage::SelectRange || CurrentStage == EGuidedStage::EffectRange)
		Pos = FVector2D(Size.X - Width - Pad, Size.Y - Height - Pad);
	Pos.X = FMath::Clamp(Pos.X, Pad, FMath::Max(Pad, Size.X - Width - Pad));
	Pos.Y = FMath::Clamp(Pos.Y, Pad, FMath::Max(Pad, Size.Y - Height - Pad));
	Cast<UCanvasPanelSlot>(Card->Slot)->SetPosition(Pos);
	BubblePosition = Pos;
	BubbleSize = {Width, Height};
}
int32 UGuidedTutorialWidget::NativePaint(const FPaintArgs& Args, const FGeometry& G, const FSlateRect& Cull,
                                         FSlateWindowElementList& Out, int32 Layer, const FWidgetStyle& Style,
                                         bool Enabled) const
{
    if (!bFocus)
        return Super::NativePaint(Args, G, Cull, Out, Layer, Style, Enabled);

    const FVector2D ViewSize = G.GetLocalSize();
    const FVector2D Center = (FocusA + FocusB) * .5;
    // A small icon still needs a readable spotlight and a clear arrow landing point.
    const FVector2D HalfSize(FMath::Max(30., (FocusB.X - FocusA.X) * .5 + 12.),
                            FMath::Max(30., (FocusB.Y - FocusA.Y) * .5 + 12.));
    const FVector2D A = Center - HalfSize, B = Center + HalfSize;
    FVector2D HoleA = A - FVector2D(8), HoleB = B + FVector2D(8);
    // Flying enemies extend above their tile. Keep their silhouette visible as well as the tile.
    if (bBoard && CurrentStage == EGuidedStage::SkillTarget) HoleA.Y -= 110.;
    HoleA.X = FMath::Clamp(HoleA.X, 0., ViewSize.X);
    HoleA.Y = FMath::Clamp(HoleA.Y, 0., ViewSize.Y);
    HoleB.X = FMath::Clamp(HoleB.X, HoleA.X, ViewSize.X);
    HoleB.Y = FMath::Clamp(HoleB.Y, HoleA.Y, ViewSize.Y);

    // NativePaint is called after Slate children. Cut out the callout as well as the target,
    // otherwise the screen shade also darkens the instructions and acknowledgement button.
    const auto& CardGeometry = Card->GetCachedGeometry();
    const FVector2D CardA = G.AbsoluteToLocal(CardGeometry.LocalToAbsolute(FVector2D::ZeroVector));
    const FVector2D CardB = G.AbsoluteToLocal(CardGeometry.LocalToAbsolute(CardGeometry.GetLocalSize()));
    TArray<double> Xs = {0., ViewSize.X, HoleA.X, HoleB.X,
        FMath::Clamp(CardA.X, 0., ViewSize.X), FMath::Clamp(CardB.X, 0., ViewSize.X)};
    TArray<double> Ys = {0., ViewSize.Y, HoleA.Y, HoleB.Y,
        FMath::Clamp(CardA.Y, 0., ViewSize.Y), FMath::Clamp(CardB.Y, 0., ViewSize.Y)};
    Xs.Sort(); Ys.Sort();
    auto Inside = [](FVector2D P, FVector2D Min, FVector2D Max)
    { return P.X >= Min.X && P.X <= Max.X && P.Y >= Min.Y && P.Y <= Max.Y; };
    static const FSlateColorBrush DimBrush(FLinearColor::White);
    for (int32 X = 0; X + 1 < Xs.Num(); ++X)
        for (int32 Y = 0; Y + 1 < Ys.Num(); ++Y)
        {
            const FVector2D Position(Xs[X], Ys[Y]), Size(Xs[X+1] - Xs[X], Ys[Y+1] - Ys[Y]);
            const FVector2D Mid = Position + Size * .5;
            if (Size.X <= 0 || Size.Y <= 0 || Inside(Mid, HoleA, HoleB) || Inside(Mid, CardA, CardB)) continue;
            FSlateDrawElement::MakeBox(Out, Layer + 1,
                G.ToPaintGeometry(FVector2f(Size), FSlateLayoutTransform(FVector2f(Position))),
                &DimBrush, ESlateDrawEffect::None, FLinearColor(.015f, .02f, .025f, .68f));
        }

    int32 DrawLayer = Super::NativePaint(Args, G, Cull, Out, Layer + 2, Style, Enabled);
    const FLinearColor Accent(1.f, .78f, .16f, 1.f), Ivory(1.f, .98f, .85f, 1.f);
    const FLinearColor Edge(.015f, .012f, .009f, 1.f);
    auto Line = [&](const TArray<FVector2D>& Points, float Width, FLinearColor Color)
    {
        FSlateDrawElement::MakeLines(Out, ++DrawLayer, G.ToPaintGeometry(), Points,
            ESlateDrawEffect::None, Color, true, Width);
    };
    const float Pulse = .5f + .5f * FMath::Sin(GestureTime * 3.5f);
    auto Outline = [&](const TArray<FVector2D>& Points)
    {
        Line(Points, 22.f + 5.f * Pulse, Accent.CopyWithNewOpacity(.18f + .10f * Pulse));
        Line(Points, 14.f, Edge);
        Line(Points, 8.f, Accent);
        Line(Points, 2.f, Ivory);
    };
    if (WorldFocus.Num() == 4)
    {
        TArray<FVector2D> Diamond;
        for (const auto& Point : WorldFocus) Diamond.Add(G.AbsoluteToLocal(Point));
        const FVector2D FirstPoint = Diamond[0];
        Diamond.Add(FirstPoint);
        Outline(Diamond);
    }
    if (!bBoard)
        Outline({A, {B.X, A.Y}, B, {A.X, B.Y}, A});

    FVector2D Dir = Center - (BubblePosition + BubbleSize * .5);
    if (!Dir.Normalize()) Dir = FVector2D(0, 1);
    // Intersect the rectangle edge; using the smaller half-size lands inside wide controls.
    const double EdgeDistance = FMath::Min(
        FMath::Abs(Dir.X) > .001 ? HalfSize.X / FMath::Abs(Dir.X) : DBL_MAX,
        FMath::Abs(Dir.Y) > .001 ? HalfSize.Y / FMath::Abs(Dir.Y) : DBL_MAX);
    const FVector2D Tip = Center - Dir * (EdgeDistance + 17.);
    const double Travel = 6. * (1. - Pulse);
    const FVector2D ArrowTip = Tip - Dir * Travel;
    const FVector2D Start = ArrowTip - Dir * 64.;
    const FVector2D Side(-Dir.Y, Dir.X);
    const TArray<FVector2D> Arrow = {Start, ArrowTip, ArrowTip - Dir * 25. + Side * 19.,
                                   ArrowTip, ArrowTip - Dir * 25. - Side * 19.};
    Line(Arrow, 17, Edge);
    Line(Arrow, 11, Accent);
    Line(Arrow, 3, Ivory);
	if (Holding(CurrentStage))
	{
		const FVector2D C = Center + FVector2D(18, 18);
		// A larger outlined hand; the ring demonstrates holding, not lesson completion.
		TArray<FVector2D> Hand = {{-7, 21},  {-14, 7}, {-14, 2}, {-10, 0}, {-5, 7},  {-5, -16},
		                          {-3, -20}, {2, -20}, {5, -16}, {5, -2},  {10, -5}, {15, -1},
		                          {20, -2},  {25, 3},  {27, 13}, {22, 27}, {-3, 27}, {-7, 21}};
		for (auto& P : Hand)
			P = P * 1.5 + C;
		Line(Hand, 13, Edge);
		Line(Hand, 7, Ivory);
		const float Phase = FMath::Fmod(GestureTime, 1.7f) / 1.7f;
		TArray<FVector2D> Arc;
		for (int I = 0; I <= 32 * Phase; ++I)
		{
			const float Angle = I / 32.f * 2 * PI - PI * .5;
			Arc.Add(C + FVector2D(0, -28) + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * 43);
		}
		if (Arc.Num() > 1)
		{
			Line(Arc, 12, Edge);
			Line(Arc, 7, Accent);
		}
	}
	return DrawLayer;
}
void UGuidedTutorialWidget::SetNotice(const FText& Text)
{
	if (Label)
		Label->SetText(Text);
}
#if WITH_DEV_AUTOMATION_TESTS
const FGeometry& UGuidedTutorialWidget::GetCalloutGeometryForTest() const { return Card->GetCachedGeometry(); }
#endif

void UGuidedTutorialWidget::PresentEncounter(const FText& Title, const FText& Description)
{
    EncounterTitle = Title;
    EncounterDescription = Description;
    Present(EGuidedStage::ReadEnemy, nullptr, true);
    if (Counter) Counter->SetText(Title);
    SetNotice(Description);
}

void UGuidedTutorialWidget::PresentReading(const FText& Title, const FText& Description, UWidget* Target, bool Last)
{
    ReadingTitle = Title; ReadingDescription = Description; bLastReading = Last;
    Present(EGuidedStage::ReadEnemy, Target);
    if (!Counter) return;
    Counter->SetText(Title);
    SetNotice(Description);
    Counter->SetAutoWrapText(true);
    Label->SetFont(FSlateFontInfo(Font, 24));
    DismissButton->SetVisibility(ESlateVisibility::Visible);
    ContinueLabel->SetText(Last ? NSLOCTEXT("FieldGuide", "ReadingDone", "안내 마침") : NSLOCTEXT("FieldGuide", "ReadingNext", "다음"));
}
