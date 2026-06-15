#include "UI/TopMenuBarLayoutPolicy.h"

FLinearColor RDTopMenuBarLayout::GetRuntimeHitAreaColor()
{
	return FLinearColor(1.0f, 1.0f, 1.0f, 0.01f);
}

FAnchors RDTopMenuBarLayout::GetDiceHitAreaAnchors()
{
	return FAnchors(0.815f, 0.105f, 0.870f, 0.170f);
}

FAnchors RDTopMenuBarLayout::GetSkillHitAreaAnchors()
{
	return FAnchors(0.870f, 0.105f, 0.930f, 0.170f);
}

int32 RDTopMenuBarLayout::GetRuntimeHitAreaZOrder()
{
	return 1000;
}
