#pragma once

#include "RDMinimal.h"
namespace RDCombatHUD
{
	static constexpr int32 CombatSkillSlotCount = 6;
	static constexpr float CombatSkillRailLeft = 0.020f;
	static constexpr float CombatSkillRailRight = 0.190f;
	static constexpr float CombatSkillRailTop = 0.144f;
	static constexpr float CombatSkillRailHeight = 0.071f;
	static constexpr float CombatSkillRailGap = 0.012f;
	static constexpr float CombatSkillDetailSafeGap = 0.016f;
	static constexpr int32 CombatSkillInputZOrder = 1000;
	static constexpr int32 CombatSkillDetailDismissZOrder = 148;
	static constexpr int32 CombatSkillDetailBackdropZOrder = 149;
	static constexpr int32 CombatSkillDetailPanelZOrder = 150;
	static constexpr int32 CombatSkillDetailRailZOrder = 170;

	inline FLinearColor GetTransparentInputButtonColor()
	{
		return FLinearColor(1.0f, 1.0f, 1.0f, 0.01f);
	}

	inline FMargin GetCombatSkillRailPadding()
	{
		return FMargin(8.0f, 4.0f);
	}

	inline FLinearColor GetCombatSkillRailBrushColor(bool bSelected)
	{
		return bSelected
			? FLinearColor(0.34f, 0.74f, 0.68f, 0.94f)
			: FLinearColor(0.18f, 0.32f, 0.32f, 0.74f);
	}

	inline FLinearColor GetCombatSkillRailTextColor(bool bSelected)
	{
		return bSelected
			? FLinearColor(1.0f, 0.94f, 0.58f, 1.0f)
			: FLinearColor(0.78f, 1.0f, 0.94f, 0.96f);
	}

	inline FVector2D GetCombatSkillRailScale(bool bSelected)
	{
		return bSelected ? FVector2D(1.035f, 1.035f) : FVector2D::UnitVector;
	}
}
