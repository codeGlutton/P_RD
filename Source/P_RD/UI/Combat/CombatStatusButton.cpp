#include "UI/Combat/CombatStatusButton.h"

void UCombatStatusButton::Configure(const bool bInAlly, const int32 InSlotIndex)
{
	mAlly = bInAlly;
	mSlotIndex = InSlotIndex;

	OnPressed.RemoveDynamic(this, &UCombatStatusButton::ForwardPressed);
	OnReleased.RemoveDynamic(this, &UCombatStatusButton::ForwardReleased);
	OnPressed.AddUniqueDynamic(this, &UCombatStatusButton::ForwardPressed);
	OnReleased.AddUniqueDynamic(this, &UCombatStatusButton::ForwardReleased);
}

void UCombatStatusButton::ForwardPressed()
{
	OnStatusPressed.Broadcast(mAlly, mSlotIndex);
}

void UCombatStatusButton::ForwardReleased()
{
	OnStatusReleased.Broadcast(mAlly, mSlotIndex);
}
