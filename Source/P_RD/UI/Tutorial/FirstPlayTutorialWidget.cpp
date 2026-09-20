#include "UI/Tutorial/FirstPlayTutorialWidget.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Engine/Font.h"
#include "Styling/CoreStyle.h"
#include "UObject/ConstructorHelpers.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSafeZone.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
const FSlateRoundedBoxBrush CardBrush(FLinearColor(.045f, .036f, .029f), 12.f, FLinearColor(.67f, .46f, .20f), 2.f);
FSlateFontInfo GuideFont(UFont *Asset, int32 Size)
{
	FSlateFontInfo Font = FCoreStyle::GetDefaultFontStyle("Regular", Size);
	if (Asset)
		Font.FontObject = Asset;
	return Font;
}
} // namespace
UFirstPlayTutorialWidget::UFirstPlayTutorialWidget(const FObjectInitializer &ObjectInitializer)
    : Super(ObjectInitializer)
{
	static ConstructorHelpers::FObjectFinder<UFont> Font(
	    TEXT("/Game/SVN/OutSideAsset/Fonts/F_HUD_Oswald.F_HUD_Oswald"));
	GuideFontAsset = Font.Object;
}
FText UFirstPlayTutorialWidget::Heading(EFirstPlayStep Step)
{
	switch (Step)
	{
	case EFirstPlayStep::MercenaryInfo: return NSLOCTEXT("FirstPlay", "MercenaryInfoTitle", "용병 정보 살펴보기");
 case EFirstPlayStep::MonsterInfo: return NSLOCTEXT("FirstPlay", "MonsterInfoTitle", "몬스터 정보 살펴보기");
 case EFirstPlayStep::InventoryInfo: return NSLOCTEXT("FirstPlay", "InventoryInfoTitle", "인벤토리와 아티팩트");
 case EFirstPlayStep::Welcome:
  return NSLOCTEXT("FirstPlay", "WelcomeTitle", "첫 전투, 세 가지만 익히세요");
	case EFirstPlayStep::Overview:
		return NSLOCTEXT("FirstPlay", "OverviewTitle", "1 / 4   내 차례 확인하기");
	case EFirstPlayStep::Move:
		return NSLOCTEXT("FirstPlay", "MoveTitle", "2 / 4   유닛 이동하기");
	case EFirstPlayStep::Skill:
		return NSLOCTEXT("FirstPlay", "SkillTitle", "3 / 4   스킬 사용하기");
	case EFirstPlayStep::EndTurn:
		return NSLOCTEXT("FirstPlay", "TurnTitle", "4 / 4   다음 차례로 넘기기");
	default:
		return NSLOCTEXT("FirstPlay", "DoneTitle", "기본 전투 안내 완료!");
	}
}
FText UFirstPlayTutorialWidget::Instructions(EFirstPlayStep Step)
{
	switch (Step)
	{
	case EFirstPlayStep::MercenaryInfo:
  return NSLOCTEXT("FirstPlay", "MercenaryInfoBody", "용병 목록에서 한 명을 고르면 능력치와 스킬을 볼 수 있습니다. 스킬 아이콘을 눌러 자세히 확인하세요.\n전투 요약판의 상태이상 아이콘은 길게 누르면 효과 설명이 나옵니다.");
 case EFirstPlayStep::MonsterInfo:
  return NSLOCTEXT("FirstPlay", "MonsterInfoBody", "목록에서 몬스터를 골라 능력치와 스킬을 확인하세요. 스킬 아이콘을 길게 누르면 상세 설명을 볼 수 있습니다.\n공격하기 전에 적의 스킬과 상태이상을 살펴보세요.");
 case EFirstPlayStep::InventoryInfo:
  return NSLOCTEXT("FirstPlay", "InventoryInfoBody", "획득한 아티팩트와 보유 골드를 확인하는 곳입니다. 아티팩트 아이콘을 누르면 효과 설명이 열립니다.\n아직 비어 있다면 모험 중 보상으로 채울 수 있습니다.");
 case EFirstPlayStep::Welcome:
  return NSLOCTEXT("FirstPlay", "WelcomeBody",
		                 "위치 잡기, 스킬을 읽고 공격하기, 다음 차례로 넘기기. "
		                 "첫 실제 전투에서 직접 해보며 익힙니다.\n정보 기능은 필요할 때 따로 살펴볼 수 있어요.");
	case EFirstPlayStep::Overview:
		return NSLOCTEXT("FirstPlay", "OverviewBody",
		                 "위쪽 턴 순서에서 내 차례의 용병을 확인하세요.\n요약판에는 HP와 행동력(AP), 이번 턴의 컨디션 "
		                 "표정이 표시됩니다.");
	case EFirstPlayStep::Move:
		return NSLOCTEXT("FirstPlay", "MoveBody",
		                 "왼쪽 아래 Skill을 눌러 카드를 펼치세요.\n이동 카드와 갈 수 있는 칸을 차례로 누른 뒤 확정하세요.\n이동할 "
		                 "수 없다면 다음 아군 차례에 해도 됩니다.");
	case EFirstPlayStep::Skill:
		return NSLOCTEXT("FirstPlay", "SkillBody",
		                 "Skill에서 스킬 카드를 고르고 대상 칸을 누르세요.\n사거리·남은 AP와 미리보기를 확인한 뒤 "
		                 "확정!\n스킬을 사용하면 다음 안내로 넘어갑니다.");
	case EFirstPlayStep::EndTurn:
		return NSLOCTEXT(
		    "FirstPlay", "TurnBody",
		    "오른쪽 아래 End Turn을 눌러 다음 차례로 넘기세요.\nAP를 모두 써서 자동으로 턴이 끝나도 인정됩니다.");
	default:
		return NSLOCTEXT("FirstPlay", "DoneBody",
		                 "이동, 스킬, 턴 진행을 모두 익혔습니다.\n아티팩트 선택 보상은 3개 중 1개를 고릅니다. 이제 "
		                 "모험을 이어가세요!");
	}
}
void UFirstPlayTutorialWidget::Present(EFirstPlayStep Step, bool TitleContext, bool ReplayOnly)
{
	if (CurrentStep != Step)
		bCompact = false;
	CurrentStep = Step;
	bTitle = TitleContext;
	bReplay = ReplayOnly;
	RefreshCard();
}
TSharedRef<SWidget> UFirstPlayTutorialWidget::RebuildWidget()
{
	SAssignNew(Host, SBox).Visibility(EVisibility::SelfHitTestInvisible);
	RefreshCard();
	return Host.ToSharedRef();
}
void UFirstPlayTutorialWidget::ReleaseSlateResources(bool Children)
{
	Super::ReleaseSlateResources(Children);
	Host.Reset();
}
void UFirstPlayTutorialWidget::RefreshCard()
{
	if (!Host.IsValid())
		return;
	const bool Info = FFirstPlayProgress::InfoBit(CurrentStep) != 0;
 const bool Mini = bReplay || bCompact;
	auto Primary = [this]()
	{
		if (FFirstPlayProgress::InfoBit(CurrentStep) || bReplay || CurrentStep == EFirstPlayStep::Welcome || CurrentStep == EFirstPlayStep::Overview ||
		    CurrentStep == EFirstPlayStep::Finished)
			OnPrimary.ExecuteIfBound();
		else
		{
			bCompact = !bCompact;
			RefreshCard();
		}
		return FReply::Handled();
	};
	if (Mini)
	{
		Host->SetContent(
		    SNew(SSafeZone).Visibility(EVisibility::SelfHitTestInvisible)[SNew(SOverlay).Visibility(EVisibility::SelfHitTestInvisible) +
		                    SOverlay::Slot()
		                        .HAlign(HAlign_Left)
		                        .VAlign(VAlign_Top)
		                        .Padding(220, 132)[SNew(SButton).OnClicked_Lambda(
		                            [this, Primary]()
		                            {
			                            if (bReplay)
				                            return Primary();
			                            bCompact = false;
			                            RefreshCard();
			                            return FReply::Handled();
		                            })[SNew(STextBlock)
		                                   .Font(GuideFont(GuideFontAsset, 22))
		                                   .Text(bReplay ? NSLOCTEXT("FirstPlay", "Replay", "튜토리얼 다시 보기")
		                                                 : NSLOCTEXT("FirstPlay", "Expand", "전투 안내 펼치기"))]]]);
		return;
	}
	FText PrimaryLabel = Info ? NSLOCTEXT("FirstPlay", "InfoRead", "확인했어요") : CurrentStep == EFirstPlayStep::Welcome    ? NSLOCTEXT("FirstPlay", "Begin", "전투에서 배우기")
	                     : CurrentStep == EFirstPlayStep::Overview ? NSLOCTEXT("FirstPlay", "Understood", "확인했어요")
	                     : CurrentStep == EFirstPlayStep::Finished ? NSLOCTEXT("FirstPlay", "Continue", "모험 계속하기")
	                                                               : NSLOCTEXT("FirstPlay", "Minimize", "안내 접기");
	auto Card =
	    SNew(SBox).WidthOverride(
	        Info ? 880.f : bTitle
	            ? 640.f
	            : 520.f)[SNew(SBorder)
	                         .BorderImage(&CardBrush)
	                         .BorderBackgroundColor(FLinearColor(.10f, .07f, .04f))
	                         .Padding(
	                             20)[SNew(SVerticalBox) +
	                                 SVerticalBox::Slot().AutoHeight().Padding(
	                                     0, 0, 0, 12)[SNew(STextBlock)
	                                                      .Font(GuideFont(GuideFontAsset, 27))
	                                                      .ColorAndOpacity(FLinearColor(1, .82f, .45f))
	                                                      .AutoWrapText(true)
	                                                      .Text(Heading(CurrentStep))] +
	                                 SVerticalBox::Slot().AutoHeight().Padding(
	                                     0, 0, 0, 18)[SNew(STextBlock)
	                                                      .Font(GuideFont(GuideFontAsset, 26))
	                                                      .ColorAndOpacity(FLinearColor(.96f, .91f, .80f))
	                                                      .WrapTextAt(Info ? 840.f : bTitle ? 600.f : 480.f)
	                                                      .Text(Instructions(CurrentStep))] +
	                                 SVerticalBox::Slot().AutoHeight()
	                                     [SNew(SHorizontalBox) +
	                                      SHorizontalBox::Slot().FillWidth(1).Padding(
	                                          0, 0, 8,
	                                          0)[SNew(SButton)
	                                                 .ButtonColorAndOpacity(FLinearColor(.45f, .28f, .10f))
	                                                 .ContentPadding(FMargin(12, 10))
	                                                 .OnClicked_Lambda(Primary)[SNew(STextBlock)
	                                                                                .Font(GuideFont(GuideFontAsset, 22))
	                                                                                .Justification(ETextJustify::Center)
	                                                                                .Text(PrimaryLabel)]] +
	                                      SHorizontalBox::Slot().AutoWidth()
	                                          [SNew(SButton)
	                                               .ButtonColorAndOpacity(FLinearColor(.45f, .28f, .10f))
	                                               .ContentPadding(FMargin(10))
	                                               .OnClicked_Lambda(
	                                                   [this]()
	                                                   {
		                                                   OnSkip.ExecuteIfBound();
		                                                   return FReply::Handled();
	                                                   })[SNew(STextBlock)
	                                                          .Font(GuideFont(GuideFontAsset, 18))
	                                                          .Text(Info ? NSLOCTEXT("FirstPlay", "InfoLater", "나중에 보기") : NSLOCTEXT("FirstPlay", "Skip", "안내 종료"))]]]]];
	Host->SetContent(
	    SNew(SSafeZone).Visibility(EVisibility::SelfHitTestInvisible)[SNew(SOverlay).Visibility(EVisibility::SelfHitTestInvisible) + SOverlay::Slot()
	                                         .HAlign((Info || bTitle) ? HAlign_Center : HAlign_Left)
	                                         .VAlign(Info ? VAlign_Bottom : bTitle ? VAlign_Center : VAlign_Top)
	                                         .Padding(Info ? FMargin(20,20,20,32) : bTitle ? FMargin(20) : FMargin(220, 132, 20, 20))[Card]]);
}
