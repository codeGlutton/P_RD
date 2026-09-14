#include "UI/Combat/CombatLayoutHUDWidget.h"
#include "UI/Combat/CombatSpeedWidget.h"
#include "UI/Combat/CombatUIModel.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Button.h"

void UCombatLayoutHUDWidget::EnsurePlaybackButton()
{
    if (!mRootCanvas || mPlaybackWidget) return;
    mPlaybackWidget = CreateWidget<UCombatSpeedWidget>(GetWorld());
    auto* CanvasSlot = mRootCanvas->AddChildToCanvas(mPlaybackWidget);
    CanvasSlot->SetSize(FVector2D(112, 60));
    CanvasSlot->SetZOrder(40);
    mChromeWidgets.AddUnique(mPlaybackWidget);
    BindPressFeedback(mPlaybackWidget->GetSpeedButton(), mPlaybackWidget);
    RefreshPlaybackButton();
}

void UCombatLayoutHUDWidget::RefreshPlaybackButton()
{
    if (mPlaybackWidget)
        mPlaybackWidget->Configure(mUIModel);
}

void UCombatLayoutHUDWidget::PositionPlaybackButton()
{
    if (!mPlaybackWidget || !mRootCanvas || !mMenuButtons.IsValidIndex(0)) return;
    const auto& Root = mRootCanvas->GetCachedGeometry();
    const auto& Menu = mMenuButtons[0]->GetCachedGeometry();
    if (Menu.GetLocalSize().IsNearlyZero()) return;
    const FVector2D Position = Root.AbsoluteToLocal(Menu.LocalToAbsolute(FVector2D(0, Menu.GetLocalSize().Y)));
    if (auto* CanvasSlot = Cast<UCanvasPanelSlot>(mPlaybackWidget->Slot))
        CanvasSlot->SetPosition(FVector2D(FMath::Clamp(Position.X, 0.f, FMath::Max(0.f, Root.GetLocalSize().X - 112.f)), Position.Y + 12.f));
}
