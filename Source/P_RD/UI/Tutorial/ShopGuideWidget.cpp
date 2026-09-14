#include "UI/Tutorial/ShopGuideWidget.h"
#include "UI/Tutorial/GuidedTutorialWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Brushes/SlateColorBrush.h"

#define LOCTEXT_NAMESPACE "FirstShopGuide"
UShopGuideWidget::UShopGuideWidget(const FObjectInitializer& I) : Super(I)
{
    mViewportZOrder = 10030;
    mRemoveFromParentOnClose = true;
}
bool UShopGuideWidget::Initialize()
{
    const bool Result = Super::Initialize();
    if (!Result || WidgetTree->RootWidget) return Result;
    auto* Root = WidgetTree->ConstructWidget<UOverlay>();
    WidgetTree->RootWidget = Root;
    auto* Shield = WidgetTree->ConstructWidget<UButton>();
    FButtonStyle Style;
    Style.SetNormal(FSlateColorBrush(FLinearColor::Transparent));
    Style.SetHovered(Style.Normal); Style.SetPressed(Style.Normal);
    Shield->SetStyle(Style);
    auto* LayerSlot = Root->AddChildToOverlay(Shield);
    LayerSlot->SetHorizontalAlignment(HAlign_Fill); LayerSlot->SetVerticalAlignment(VAlign_Fill);
    Guide = CreateWidget<UGuidedTutorialWidget>(this);
    LayerSlot = Root->AddChildToOverlay(Guide);
    LayerSlot->SetHorizontalAlignment(HAlign_Fill); LayerSlot->SetVerticalAlignment(VAlign_Fill);
    Guide->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
    return Result;
}
void UShopGuideWidget::RefreshPage()
{
    FText Title, Body;
    switch (Page)
    {
    case 0:
        Title = LOCTEXT("GoldTitle", "보유 골드");
        Body = LOCTEXT("GoldBody", "모험에서 모은 골드예요. 상점에서 파티를 정비할 때 사용합니다.\n지금은 설명만 둘러보세요. 아무것도 살 필요는 없어요."); break;
    case 1:
        Title = LOCTEXT("ArtifactTabTitle", "아티팩트 탭");
        Body = LOCTEXT("ArtifactTabBody", "파티에 여러 효과를 더해주는 아티팩트를 파는 곳이에요.\n지금 가진 효과와 어울리는 상품을 찾아볼 수 있어요."); break;
    case 2:
        Title = LOCTEXT("ProductTitle", "상품과 자세한 설명");
        Body = LOCTEXT("ProductBody", "가운데 카드가 선택한 상품이에요. 좌우 화살표로 다른 상품을 볼 수 있어요.\n상품을 길게 누르면 자세한 효과가 나옵니다."); break;
    case 3:
        Title = LOCTEXT("BuyTitle", "구매 버튼");
        Body = LOCTEXT("BuyBody", "선택한 상품을 골드로 사는 버튼이에요. 돈이 부족하거나 품절이면 구매할 수 없어요.\n지금은 구매하지 않고 다음 설명으로 넘어갑니다."); break;
    case 4:
        Title = LOCTEXT("InventoryTitle", "보유 아티팩트");
        Body = LOCTEXT("InventoryBody", "용병 정보 버튼을 열고 인벤토리를 선택하면, 파티가 이미 가진 아티팩트를 볼 수 있어요.\n새 상품과 효과를 비교할 때 이용하면 좋아요."); break;
    case 5:
        Title = LOCTEXT("SkillTabTitle", "스킬 탭");
        Body = LOCTEXT("SkillTabBody", "용병에게 장착할 스킬을 파는 곳이에요.\n직업 전용 스킬은 해당 직업의 용병만 사용할 수 있어요."); break;
    case 6:
        Title = LOCTEXT("SkillUnitTitle", "스킬을 배울 용병");
        Body = LOCTEXT("SkillUnitBody", "이곳에서 스킬을 배울 용병을 선택해요.\n같은 직업의 용병이 여럿이면 직업 카드를 다시 눌러 대상을 바꿀 수 있어요."); break;
    case 7:
        Title = LOCTEXT("SkillSlotTitle", "스킬을 장착할 칸");
        Body = LOCTEXT("SkillSlotBody", "구매할 스킬을 넣을 칸이에요. 이미 스킬이 있다면 교체 확인이 나옵니다.\n교체하면 기존 스킬은 없어져요. 기본 공격과 이동은 교체하지 않아요."); break;
    case 8:
        Title = LOCTEXT("RestTabTitle", "휴식 탭");
        Body = LOCTEXT("RestTabBody", "골드를 내고 파티의 체력을 회복하는 곳이에요.\n회복 전후 HP가 표시되니 얼마나 회복되는지 먼저 살펴보세요."); break;
    case 9:
        Title = LOCTEXT("RestButtonTitle", "휴식 실행");
        Body = LOCTEXT("RestButtonBody", "표시된 비용을 내고 회복하는 버튼이에요. 휴식은 이 상점에서 한 번만 이용할 수 있어요.\n안내 중에는 비용을 쓰지 않습니다."); break;
    case 10:
        Title = LOCTEXT("HireTabTitle", "용병 고용 탭");
        Body = LOCTEXT("HireTabBody", "후보의 능력과 고용 비용을 살펴볼 수 있어요.\n고용할 후보와 파티 자리를 고르는 방식이며, 이미 용병이 있는 자리는 교체됩니다."); break;
    default:
        Title = LOCTEXT("LeaveTitle", "상점 나가기");
        Body = LOCTEXT("LeaveBody", "정비가 끝나면 이 버튼으로 다음 방을 고르면 돼요.\n구매나 휴식 없이, 골드를 아껴두고 바로 떠나도 괜찮아요!"); break;
    }
    UWidget* Target = OnStepChanged.IsBound() ? OnStepChanged.Execute(Page) : nullptr;
    Guide->PresentReading(FText::Format(LOCTEXT("StepTitle", "상점 {0}/{1} · {2}"), Page + 1, PageCount, Title), Body, Target, Page == PageCount - 1);
}
void UShopGuideWidget::Next() { if (bDismissed) return; if (Page == PageCount - 1) Dismiss(); else { ++Page; RefreshPage(); } }
void UShopGuideWidget::Previous() { if (!bDismissed && Page > 0) { --Page; RefreshPage(); } }
void UShopGuideWidget::Dismiss()
{
    if (bDismissed) return;
    bDismissed = true;
    const auto Callback = OnDismissed;
    CloseUI(); Callback.ExecuteIfBound();
}
bool UShopGuideWidget::HandleBackNavigation() { Dismiss(); return true; }
void UShopGuideWidget::ApplyOpenUI()
{
    Super::ApplyOpenUI();
    BeginReading();
    if (GetOwningPlayer()) Guide->GetDismissButton()->SetUserFocus(GetOwningPlayer());
}
void UShopGuideWidget::BeginReading()
{
    Guide->TakeWidget();
    Guide->GetContinueButton()->OnClicked.AddUniqueDynamic(this, &UShopGuideWidget::Next);
    Guide->GetDismissButton()->OnClicked.AddUniqueDynamic(this, &UShopGuideWidget::Dismiss);
    RefreshPage();
}
#undef LOCTEXT_NAMESPACE
