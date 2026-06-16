#pragma once

#include "RDMinimal.h"

namespace RDCombatHUD
{
	static constexpr int32 MaxCombatDiceCardCount = 8;
	static constexpr int32 CombatSkillSlotCount = 6;
	static constexpr float CombatSkillRailLeft = 0.020f;
	static constexpr float CombatSkillRailRight = 0.190f;
	static constexpr float CombatSkillRailTop = 0.144f;
	static constexpr float CombatSkillRailHeight = 0.071f;
	static constexpr float CombatSkillRailGap = 0.012f;
	static constexpr int32 CombatSkillInputZOrder = 1000;
	static constexpr int32 CombatSkillStepIndex = 5;

	inline FText GetCombatSkillLabel(int32 SkillIndex)
	{
		switch (SkillIndex)
		{
		case 0:
			return NSLOCTEXT("CombatTileMapHUDWidget", "CombatSkillBasicAttack", "BASIC");
		case 1:
			return NSLOCTEXT("CombatTileMapHUDWidget", "CombatSkill1", "SKILL 1");
		case 2:
			return NSLOCTEXT("CombatTileMapHUDWidget", "CombatSkill2", "SKILL 2");
		case 3:
			return NSLOCTEXT("CombatTileMapHUDWidget", "CombatSkill3", "SKILL 3");
		case 4:
			return NSLOCTEXT("CombatTileMapHUDWidget", "CombatSkill4", "SKILL 4");
		case 5:
			return NSLOCTEXT("CombatTileMapHUDWidget", "CombatSkillStep", "STEP");
		default:
			return NSLOCTEXT("CombatTileMapHUDWidget", "CombatSkillUnknown", "SKILL");
		}
	}

	inline FText GetCombatSkillRailLabel(int32 SkillIndex)
	{
		// STEP 레일은 "STEP"만 표시(이동 가능 수치는 상단 MOVE x/x로 보여준다).
		return GetCombatSkillLabel(SkillIndex);
	}

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

	inline float EaseOutCubic(float Alpha)
	{
		const float ClampedAlpha = FMath::Clamp(Alpha, 0.0f, 1.0f);
		const float InverseAlpha = 1.0f - ClampedAlpha;
		return 1.0f - InverseAlpha * InverseAlpha * InverseAlpha;
	}

	inline FRotator GetReadableDiceIdleRotation(int32 DiceIndex)
	{
		static const FRotator IdleRotations[] =
		{
			FRotator(24.0f, -38.0f, 16.0f),
			FRotator(18.0f, -20.0f, -10.0f),
			FRotator(28.0f, -52.0f, 8.0f),
			FRotator(16.0f, -30.0f, 18.0f),
		};

		return IdleRotations[DiceIndex % UE_ARRAY_COUNT(IdleRotations)];
	}
}
