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
		return NSLOCTEXT("FieldGuide", "Condition", "표정은 컨디션입니다.\n나쁨·보통·좋음 순으로 피해가 높아지며, 최상은 치명타도 적용됩니다.");
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
	auto* ContinueLabel = MakeText(22);
	ContinueLabel->SetColorAndOpacity(FLinearColor(.98f, .91f, .73f));
	ContinueLabel->SetText(NSLOCTEXT("FieldGuide", "Continue", "확인"));
	ContinueButton->AddChild(ContinueLabel);
	auto* ContinueSlot = VBox->AddChildToVerticalBox(ContinueButton);
	ContinueSlot->SetHorizontalAlignment(HAlign_Right);
	ContinueSlot->SetPadding(FMargin(0, 12, 0, 0));
	auto* CS = Canvas->AddChildToCanvas(Card);
	CS->SetAutoSize(true);
	Present(CurrentStage, TargetWidget.Get(), bBoard);
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
	const double Width = FMath::Min(460., Size.X - 48.);
	Card->SetWidthOverride(Width);
	const double Height = FMath::Max(120., Card->GetDesiredSize().Y);
	const double Pad = 24., Gap = 36.;
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
	const int32 Top = Super::NativePaint(Args, G, Cull, Out, Layer, Style, Enabled);
	if (!bFocus)
		return Top;
	int32 DrawLayer = Top;
	auto Line = [&](const TArray<FVector2D>& P, float W, FLinearColor C) {
		FSlateDrawElement::MakeLines(Out, ++DrawLayer, G.ToPaintGeometry(), P, ESlateDrawEffect::None, C,
		                             true, W);
	};
	const FVector2D A = FocusA - FVector2D(5), B = FocusB + FVector2D(5), Center = (A + B) * .5;
	const double L = FMath::Min(22., FMath::Min(B.X - A.X, B.Y - A.Y) * .25);
	const float Pulse = .7f + .3f * FMath::Sin(GestureTime * 4);
	if (WorldFocus.Num() == 4)
	{
		TArray<FVector2D> Diamond;
		for (const auto& Point : WorldFocus) Diamond.Add(G.AbsoluteToLocal(Point));
		// TArray::Add rejects references into itself, even if there is spare capacity.
		const FVector2D FirstPoint = Diamond[0];
		Diamond.Add(FirstPoint);
		Line(Diamond, 12, Ink.CopyWithNewOpacity(.85f));
		Line(Diamond, 6, Gold.CopyWithNewOpacity(Pulse));
		Line(Diamond, 2, FLinearColor(1, 1, .8f));
	}
	if (!bBoard)
	for (FVector2D Corner : {A, FVector2D(B.X, A.Y), B, FVector2D(A.X, B.Y)})
	{
		const double SX = Corner.X == A.X ? 1 : -1, SY = Corner.Y == A.Y ? 1 : -1;
		TArray<FVector2D> P = {Corner + FVector2D(SX * L, 0), Corner, Corner + FVector2D(0, SY * L)};
		Line(P, 6, Ink);
		Line(P, 3, Gold.CopyWithNewOpacity(Pulse));
	}
	FVector2D Dir = Center - (BubblePosition + BubbleSize * .5);
	Dir.Normalize();
	const FVector2D Tip = Center - Dir * FMath::Min((B.X - A.X) * .5, (B.Y - A.Y) * .5);
	const FVector2D Start = Tip - Dir * (26. + 4. * FMath::Sin(GestureTime * 4));
	const FVector2D Side(-Dir.Y, Dir.X);
	TArray<FVector2D> Arrow = {Start, Tip, Tip - Dir * 12. + Side * 9., Tip, Tip - Dir * 12. - Side * 9.};
	Line(Arrow, 7, Ink);
	Line(Arrow, 4, Gold);
	if (Holding(CurrentStage))
	{
		const FVector2D C = Center + FVector2D(18, 18);
		// A small outlined pointing hand; the ring demonstrates holding, not lesson completion.
		TArray<FVector2D> Hand = {{-7, 21},  {-14, 7}, {-14, 2}, {-10, 0}, {-5, 7},  {-5, -16},
		                          {-3, -20}, {2, -20}, {5, -16}, {5, -2},  {10, -5}, {15, -1},
		                          {20, -2},  {25, 3},  {27, 13}, {22, 27}, {-3, 27}, {-7, 21}};
		for (auto& P : Hand)
			P += C;
		Line(Hand, 8, Ink);
		Line(Hand, 4, FLinearColor(1.f, .95f, .8f));
		const float Phase = FMath::Fmod(GestureTime, 1.7f) / 1.7f;
		TArray<FVector2D> Arc;
		for (int I = 0; I <= 32 * Phase; ++I)
		{
			const float Angle = I / 32.f * 2 * PI - PI * .5;
			Arc.Add(C + FVector2D(0, -19) + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * 29);
		}
		if (Arc.Num() > 1)
		{
			Line(Arc, 7, Ink);
			Line(Arc, 3, Gold);
		}
	}
	return DrawLayer;
}
void UGuidedTutorialWidget::SetNotice(const FText& Text)
{
	if (Label)
		Label->SetText(Text);
}
