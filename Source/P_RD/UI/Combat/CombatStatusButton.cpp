#include "UI/Combat/CombatStatusButton.h"

void UCombatStatusButton::Configure(const bool bInAlly, const int32 InSlotIndex)
{
	mAlly = bInAlly;
	mSlotIndex = InSlotIndex;

	OnPressed.RemoveDynamic(this, &UCombatStatusButton::ForwardPressed);
	OnClicked.RemoveDynamic(this, &UCombatStatusButton::ForwardClicked);
	OnPressed.AddUniqueDynamic(this, &UCombatStatusButton::ForwardPressed);
	OnClicked.AddUniqueDynamic(this, &UCombatStatusButton::ForwardClicked);
}

void UCombatStatusButton::ForwardPressed()
{
	OnStatusPressed.Broadcast(mAlly, mSlotIndex);
}

void UCombatStatusButton::ForwardClicked()
{
	OnStatusClicked.Broadcast(mAlly, mSlotIndex);
}
