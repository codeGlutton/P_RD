#pragma once

#include "RDMinimal.h"
#include "UI/DiceViewData.h"

class ACombatDiceCaptureActor;

namespace RDCombatHUD
{
	static constexpr int32 MaxCombatDiceCardCount = 8;
	static constexpr int32 CombatSkillSlotCount = 6;
	// 데이터 index 1은 기존 레일 계약상 기본 이동(STEP)이다. UI에서는 별도 MOVE 버튼 대신 이 행이 Move 의도를 보낸다.
	static constexpr int32 CombatMovementSkillDataIndex = 1;
	// 모바일 landscape HUD는 1920x1080 논리 좌표에서 핵심 입력을 112~144px로 유지한다.
	// 720p(스케일 0.666)에서도 약 75~96px이 되어 손가락으로 안정적으로 누를 수 있다.
	static constexpr float MobileSafeInsetX = 64.0f;
	static constexpr float MobileSafeInsetTop = 32.0f;
	static constexpr float MobileWideReferenceWidthPx = 1920.0f;
	static constexpr float MobileCompactReferenceWidthPx = 1440.0f;
	static constexpr float MobileReferenceHeightPx = 1080.0f;
	static constexpr float MobileSkillRailTopPx = 144.0f;
	// 스킬 레일은 탑바의 64px inset을 따라가지 않고 Safe Area의 실제 좌측 끝에 붙인다.
	static constexpr float MobileSkillRailLeftPx = 0.0f;
	static constexpr float MobileSkillRowHeightPx = 112.0f;
	static constexpr float MobileSkillRowGapPx = 8.0f;
	static constexpr float MobileSkillCollapsedWidthPx = 144.0f;
	static constexpr float MobileSkillExpandedWidthPx = 384.0f;
	static constexpr float MobileSkillIconSizePx = 96.0f;
	static constexpr float MobileSkillDrawerHandleWidthPx = 64.0f;
	static constexpr float MobileSkillDrawerHandleHeightPx = 112.0f;
	static constexpr float MobileSkillDrawerAnimationSeconds = 0.20f;
	static constexpr float MobileDiceCellSizePx = 144.0f;
	static constexpr float MobileDiceCellGapPx = 12.0f;
	static constexpr float MobileDiceDockHeightPx = 144.0f;
	static constexpr float MobileBottomEdgeInsetPx = 24.0f;
	static constexpr float MobileBottomGroupGapPx = 24.0f;
	static constexpr float MobileEndTurnWidthPx = 340.0f;
	static constexpr float MobileEndTurnHeightPx = 144.0f;
	static constexpr float MobileCancelActionWidthPx = 260.0f;
	static constexpr float MobileActionButtonGapPx = 24.0f;
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
