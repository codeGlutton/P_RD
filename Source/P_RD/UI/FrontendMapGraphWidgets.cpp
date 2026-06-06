#include "UI/FrontendMapGraphWidgets.h"

#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"

void UFrontendMapNodeButton::SetNodeCoordinates(int32 InRowIndex, int32 InColumnIndex)
{
	RowIndex = InRowIndex;
	ColumnIndex = InColumnIndex;

	if (!bClickBound)
	{
		OnClicked.AddUniqueDynamic(this, &UFrontendMapNodeButton::HandleClicked);
		bClickBound = true;
	}
}

void UFrontendMapNodeButton::HandleClicked()
{
	OnMapNodeClicked.Broadcast(RowIndex, ColumnIndex);
}

void UFrontendMapLineWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (LinePanel == nullptr)
	{
		UE_LOG(LogRD, Warning, TEXT("FrontendMapLineWidget: LinePanel is not connected. WBP_FrontendMapLine must provide a Border named LinePanel."));
	}
}

void UFrontendMapLineWidget::SetLineColor(const FLinearColor& InColor)
{
	if (LinePanel != nullptr)
	{
		LinePanel->SetBrushColor(InColor);
	}
}

void UFrontendMapNodeWidget::SetNodeVisual(
	int32 InRowIndex,
	int32 InColumnIndex,
	const FText& Label,
	const FText& Badge,
	const FLinearColor& PanelColor,
	const FLinearColor& TypeStripeColor,
	const FSlateColor& LabelColor,
	const FSlateColor& BadgeColor)
{
	RowIndex = InRowIndex;
	ColumnIndex = InColumnIndex;

	if (NodePanel != nullptr)
	{
		NodePanel->SetBrushColor(PanelColor);
	}
	if (NodeTypeStripe != nullptr)
	{
		NodeTypeStripe->SetBrushColor(TypeStripeColor);
	}
	if (NodeLabelText != nullptr)
	{
		NodeLabelText->SetText(Label);
		NodeLabelText->SetColorAndOpacity(LabelColor);
	}
	if (NodeBadgeText != nullptr)
	{
		NodeBadgeText->SetText(FText::GetEmpty());
		NodeBadgeText->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UFrontendMapNodeWidget::SetNodeEnabled(bool bEnabled) const
{
	if (NodeButton != nullptr)
	{
		NodeButton->SetIsEnabled(bEnabled);
	}
}

void UFrontendMapNodeWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (NodeButton != nullptr)
	{
		NodeButton->OnClicked.AddUniqueDynamic(this, &UFrontendMapNodeWidget::HandleNodeButtonClicked);
	}
	else
	{
		UE_LOG(LogRD, Warning, TEXT("FrontendMapNodeWidget: NodeButton is not connected. WBP_FrontendMapNode must provide a Button named NodeButton."));
	}
}

void UFrontendMapNodeWidget::NativeDestruct()
{
	if (NodeButton != nullptr)
	{
		NodeButton->OnClicked.RemoveDynamic(this, &UFrontendMapNodeWidget::HandleNodeButtonClicked);
	}

	Super::NativeDestruct();
}

void UFrontendMapNodeWidget::HandleNodeButtonClicked()
{
	OnMapNodeClicked.Broadcast(RowIndex, ColumnIndex);
}
