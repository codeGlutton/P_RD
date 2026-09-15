#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/ScrollBox.h"
#include "Components/ScaleBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "UI/Combat/CombatLayoutHUDWidget.h"
#include "UI/Combat/CombatListEntryAction.h"

namespace
{
void Place(UCanvasPanel *Parent, UWidget *Child, FVector2D Position, FVector2D Size, int32 Z = 0)
{
    auto *CanvasSlot = Parent->AddChildToCanvas(Child);
    CanvasSlot->SetPosition(Position);
    CanvasSlot->SetSize(Size);
    CanvasSlot->SetZOrder(Z);
}
// UMG children are owned by WidgetTree, so duplicating only a panel would share
// its original children. Rebuild the hierarchy and duplicate each leaf explicitly.
UWidget *CloneRow(UWidgetTree *Tree, UWidget *Source, int32 Index)
{
    FString Name = Source->GetName();
    Name.RemoveFromEnd(TEXT("_0"));
    Name += FString::Printf(TEXT("_%d"), Index);
    if (auto *SourcePanel = Cast<UPanelWidget>(Source))
    {
        auto *Copy = Tree->ConstructWidget<UPanelWidget>(Source->GetClass(), *Name);
        Copy->SetVisibility(Source->GetVisibility());
        Copy->SetRenderTransform(Source->GetRenderTransform());
        Copy->SetRenderTransformPivot(Source->GetRenderTransformPivot());
        if (auto* SourceScale=Cast<UScaleBox>(Source))
        {
            auto* Scale=CastChecked<UScaleBox>(Copy);
            Scale->SetStretch(SourceScale->GetStretch());
            Scale->SetStretchDirection(SourceScale->GetStretchDirection());
            Scale->SetUserSpecifiedScale(SourceScale->GetUserSpecifiedScale());
        }
        if (auto *SourceButton = Cast<UButton>(Source))
        {
            auto *Button = CastChecked<UButton>(Copy);
            Button->SetStyle(SourceButton->GetStyle());
            Button->SetBackgroundColor(SourceButton->GetBackgroundColor());
            Button->SetColorAndOpacity(SourceButton->GetColorAndOpacity());
        }
        for (auto *Child : SourcePanel->GetAllChildren())
        {
            auto *ChildCopy = CloneRow(Tree, Child, Index);
            Copy->AddChild(ChildCopy, Child->Slot);
            if (auto *Layout = Cast<UCanvasPanelSlot>(Child->Slot))
            {
                auto *NewLayout = CastChecked<UCanvasPanelSlot>(ChildCopy->Slot);
                NewLayout->SetLayout(Layout->GetLayout());
                NewLayout->SetAutoSize(Layout->GetAutoSize());
                NewLayout->SetZOrder(Layout->GetZOrder());
            }
        }
        return Copy;
    }
    auto *Copy = DuplicateObject<UWidget>(Source, Tree, *Name);
    Copy->Slot = nullptr;
    return Copy;
}
void BindEntry(UButton *Button, UCombatListEntryAction *Action)
{
    Button->OnClicked.Clear();
    Button->OnClicked.AddDynamic(Action, &UCombatListEntryAction::Invoke);
    Button->SetTouchMethod(EButtonTouchMethod::PreciseTap);
    Button->SetClickMethod(EButtonClickMethod::PreciseClick);
}
} // namespace

void UCombatLayoutHUDWidget::EnsureInventoryScroll(int32 Count)
{
    auto *Page = Cast<UCanvasPanel>(mMercenaryInventoryPage);
    if (!Page || mMercenaryInventoryArtifactFrames.IsEmpty())
        return;
    if (!mInventoryScroll)
    {
        auto *Gold = WidgetTree->FindWidget(TEXT("MercenaryInventoryGoldFrame"));
        auto *GoldSlot = Gold ? Cast<UCanvasPanelSlot>(Gold->Slot) : nullptr;
        if (!GoldSlot)
            return;
        mInventoryGridOrigin = GoldSlot->GetPosition();
        const auto* Cell0 = CastChecked<UCanvasPanelSlot>(mMercenaryInventoryArtifactFrames[0]->Slot);
        const auto* Cell1 = CastChecked<UCanvasPanelSlot>(mMercenaryInventoryArtifactFrames[1]->Slot);
        const auto* Cell2 = CastChecked<UCanvasPanelSlot>(mMercenaryInventoryArtifactFrames[2]->Slot);
        const auto* Cell4 = CastChecked<UCanvasPanelSlot>(mMercenaryInventoryArtifactFrames[4]->Slot);
        mInventoryCellPitch = FVector2D(Cell1->GetPosition().X-Cell0->GetPosition().X, Cell4->GetPosition().Y-Cell0->GetPosition().Y);
        mInventoryFrameSize = Cell0->GetSize();
        mInventoryViewportSize = FVector2D(Cell2->GetPosition().X+Cell2->GetSize().X-mInventoryGridOrigin.X+22.f,
            mInventoryCellPitch.Y+mInventoryFrameSize.Y);
        mInventoryScroll =
            WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("InventoryArtifactScroll"));
        mInventoryScroll->SetScrollBarVisibility(ESlateVisibility::Visible);
        mInventoryScroll->SetAllowOverscroll(false);
        mInventoryScrollSize = WidgetTree->ConstructWidget<USizeBox>();
        mInventoryScrollCanvas = WidgetTree->ConstructWidget<UCanvasPanel>();
        mInventoryScrollCanvas->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
        mInventoryScrollSize->AddChild(mInventoryScrollCanvas);
        mInventoryScroll->AddChild(mInventoryScrollSize);
        // Keep the existing four-column art and gold cell inside the same viewport.
        const auto Children = Page->GetAllChildren();
        for (auto *Child : Children)
        {
            const FString Name = Child->GetName();
            if (!(Name.StartsWith(TEXT("MercenaryInventoryArtifact")) || Name == TEXT("MercenaryInventoryGoldFrame") ||
                  Name == TEXT("MercenaryInventoryGoldIcon") || Name == TEXT("MercenaryInventoryGoldText")))
                continue;
            auto *CanvasSlot = Cast<UCanvasPanelSlot>(Child->Slot);
            if (!CanvasSlot)
                continue;
            const FVector2D Pos = CanvasSlot->GetPosition() - mInventoryGridOrigin, Size = CanvasSlot->GetSize();
            const int32 Z = CanvasSlot->GetZOrder();
            Child->RemoveFromParent();
            Place(mInventoryScrollCanvas, Child, Pos, Size, Z);
        }
        Place(Page, mInventoryScroll, mInventoryGridOrigin, mInventoryViewportSize, 5);
        for (int32 I = 0; I < mMercenaryInventoryArtifactButtons.Num(); ++I)
        {
            auto *Action = NewObject<UCombatListEntryAction>(this);
            mListEntryActions.Add(Action);
            Action->Action =
                FSimpleDelegate::CreateWeakLambda(this, [this, I] { SelectMercenaryInventoryArtifact(I); });
            if (auto *Button = mMercenaryInventoryArtifactButtons[I].Get())
                BindEntry(Button, Action);
        }
    }
    for (int32 I = mMercenaryInventoryArtifactFrames.Num(); I < Count; ++I)
    {
        // Duplicate the authored cell's brushes, typography and inset, not its fixed index.
        const FVector2D Delta((float)((I + 1) % 4 - 1) * mInventoryCellPitch.X, (float)((I + 1) / 4) * mInventoryCellPitch.Y);
        const auto Clone = [&](UWidget *Source, const TCHAR *Prefix) -> UWidget * {
            if (!Source)
                return nullptr;
            auto *SourceSlot = Cast<UCanvasPanelSlot>(Source->Slot);
            if (!SourceSlot)
                return nullptr;
            auto *Copy = DuplicateObject<UWidget>(Source, WidgetTree, *FString::Printf(TEXT("%s_%d"), Prefix, I));
            Copy->Slot = nullptr;
            Place(mInventoryScrollCanvas, Copy, SourceSlot->GetPosition() + Delta, SourceSlot->GetSize(),
                  SourceSlot->GetZOrder());
            return Copy;
        };
        mMercenaryInventoryArtifactFrames.Add(
            Clone(mMercenaryInventoryArtifactFrames[0], TEXT("MercenaryInventoryArtifactFrame")));
        mMercenaryInventoryArtifactIcons.Add(
            Cast<UImage>(Clone(mMercenaryInventoryArtifactIcons[0], TEXT("MercenaryInventoryArtifactIcon"))));
        mMercenaryInventoryArtifactNames.Add(
            Cast<UTextBlock>(Clone(mMercenaryInventoryArtifactNames[0], TEXT("MercenaryInventoryArtifactName"))));
        auto *Button =
            Cast<UButton>(Clone(mMercenaryInventoryArtifactButtons[0], TEXT("MercenaryInventoryArtifactButton")));
        mMercenaryInventoryArtifactButtons.Add(Button);
        auto *Action = NewObject<UCombatListEntryAction>(this);
        mListEntryActions.Add(Action);
        Action->Action = FSimpleDelegate::CreateWeakLambda(this, [this, I] { SelectMercenaryInventoryArtifact(I); });
        BindEntry(Button, Action);
    }
    const int32 Rows = FMath::Max(2, FMath::DivideAndRoundUp(Count + 1, 4));
    mInventoryScrollSize->SetHeightOverride((Rows-1)*mInventoryCellPitch.Y+mInventoryFrameSize.Y);
    mInventoryScrollSize->SetWidthOverride(mInventoryViewportSize.X-22.f);
    for (int32 I = 0; I < mMercenaryInventoryArtifactButtons.Num(); ++I)
        if (auto *Button = mMercenaryInventoryArtifactButtons[I].Get())
            Button->SetVisibility(I < Count ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    mInventoryScroll->SetScrollOffset(
        FMath::Min(mInventoryScroll->GetScrollOffset(), FMath::Max(0.f, (Rows-2)*mInventoryCellPitch.Y)));
}

void UCombatLayoutHUDWidget::EnsureMonsterScroll(int32 Count)
{
    auto *Tree = mMonsterTabWidget->WidgetTree.Get();
    auto *First = Tree->FindWidget(TEXT("MonsterRow_0"));
    if (!First)
        return;
    if (!mMonsterScroll)
    {
        auto *Parent = Cast<UCanvasPanel>(First->GetParent());
        auto *FirstSlot = Cast<UCanvasPanelSlot>(First->Slot);
        auto *Last = Tree->FindWidget(TEXT("MonsterRow_2"));
        auto *LastSlot = Last ? Cast<UCanvasPanelSlot>(Last->Slot) : nullptr;
        if (!Parent || !FirstSlot || !LastSlot)
            return;
        const FVector2D Origin = FirstSlot->GetPosition();
        const FVector2D Bounds = LastSlot->GetPosition() + LastSlot->GetSize() - Origin;
        mMonsterScroll = Tree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("MonsterListScroll"));
        mMonsterScroll->SetScrollBarVisibility(ESlateVisibility::Visible);
        mMonsterScroll->SetAllowOverscroll(false);
        auto *Size = Tree->ConstructWidget<USizeBox>();
        auto *Canvas = Tree->ConstructWidget<UCanvasPanel>();
        Size->AddChild(Canvas);
        mMonsterScroll->AddChild(Size);
        Size->SetWidthOverride(Bounds.X);
        Size->SetHeightOverride(Bounds.Y);
        for (int32 I = 0; I < 3; ++I)
        {
            auto *Row = Tree->FindWidget(*FString::Printf(TEXT("MonsterRow_%d"), I));
            auto *CanvasSlot = Cast<UCanvasPanelSlot>(Row->Slot);
            const FVector2D Pos = CanvasSlot->GetPosition() - Origin, RowSize = CanvasSlot->GetSize();
            Row->RemoveFromParent();
            Place(Canvas, Row, Pos, RowSize);
        }
        Place(Parent, mMonsterScroll, Origin, FVector2D(Bounds.X + 20, Bounds.Y), 10);
    }
    auto *Size = CastChecked<USizeBox>(mMonsterScroll->GetChildAt(0));
    auto *Canvas = CastChecked<UCanvasPanel>(Size->GetChildAt(0));
    auto *Second = Tree->FindWidget(TEXT("MonsterRow_1"));
    const float Pitch = CastChecked<UCanvasPanelSlot>(Second->Slot)->GetPosition().Y;
    const FVector2D RowSize = CastChecked<UCanvasPanelSlot>(First->Slot)->GetSize();
    for (int32 I = mMonsterRowCount; I < Count; ++I)
    {
        auto *Row = CloneRow(Tree, First, I);
        Place(Canvas, Row, FVector2D(0, I * Pitch), RowSize);
    }
    mMonsterRowCount = FMath::Max(mMonsterRowCount, Count);
    // Rebinding uses stable list indices, including rows beyond the original three.
    for (int32 I = mMonsterBoundRowCount; I < mMonsterRowCount; ++I)
    {
        auto *Button = Cast<UButton>(Tree->FindWidget(*FString::Printf(TEXT("MonsterRowButton_%d"), I)));
        if (!Button)
            continue;
        auto *Action = NewObject<UCombatListEntryAction>(this);
        mListEntryActions.Add(Action);
        Action->Action = FSimpleDelegate::CreateWeakLambda(this, [this, I] { HandleMonsterTabRowClicked(I); });
        BindEntry(Button, Action);
    }
    mMonsterBoundRowCount = mMonsterRowCount;
    Size->SetHeightOverride(FMath::Max(0, Count - 1) * Pitch + RowSize.Y);
}
