#pragma once

#include "RDMinimal.h"
#include "UI/DiceViewData.h"

class ACombatDiceCaptureActor;

namespace RDCombatHUD
{
	static constexpr int32 MaxCombatDiceCardCount = 8;
	// 개별 기술 여섯 칸 대신 전투 중 판단 단위인 행동군만 노출한다.
	// 기본 공격 / 손아귀 / 방해 / 이동의 네 칸이며, 손아귀가 당기기·던지기·교환을 문맥에 맞게 고른다.
	static constexpr int32 CombatSkillSlotCount = 4;
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

	inline int32 GetDiceSettledFaceOrdinal(const FDiceViewData& DiceView)
	{
		if (DiceView.mRolledFaceIndex != INDEX_NONE)
		{
			return DiceView.mRolledFaceIndex + 1;
		}

		const int32 MatchingFaceIndex = DiceView.mFaceValues.Find(DiceView.mResultValue);
		if (MatchingFaceIndex != INDEX_NONE)
		{
			return MatchingFaceIndex + 1;
		}

		return FMath::Clamp(DiceView.mResultValue, 1, FMath::Max(1, DiceView.mFaceCount));
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
