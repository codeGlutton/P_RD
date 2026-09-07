#include "UI/Combat/CombatStatusPresentation.h"

#include "GameplayTagType.h"

namespace
{
	FString StatusLeafName(const FGameplayTag& Tag)
	{
		const FString FullName = Tag.GetTagName().ToString();
		int32 LastDot = INDEX_NONE;
		return FullName.FindLastChar(TEXT('.'), LastDot)
			? FullName.Mid(LastDot + 1)
			: FullName;
	}

	FText LocalizedStatusName(const FString& Leaf)
	{
		static const TMap<FString, FText> Names = {
			{ TEXT("Weakness"),      NSLOCTEXT("CombatStatusUI", "Weakness", "약화") },
			{ TEXT("Vulnerability"), NSLOCTEXT("CombatStatusUI", "Vulnerability", "취약") },
			{ TEXT("Vigor"),         NSLOCTEXT("CombatStatusUI", "Vigor", "활력") },
			{ TEXT("Fortification"), NSLOCTEXT("CombatStatusUI", "Fortification", "강화") },
			{ TEXT("Haste"),         NSLOCTEXT("CombatStatusUI", "Haste", "신속") },
			{ TEXT("Exhaustion"),    NSLOCTEXT("CombatStatusUI", "Exhaustion", "탈진") },
			{ TEXT("Slow"),          NSLOCTEXT("CombatStatusUI", "Slow", "둔화") },
			{ TEXT("Frail"),         NSLOCTEXT("CombatStatusUI", "Frail", "쇠약") },
			{ TEXT("Root"),          NSLOCTEXT("CombatStatusUI", "Root", "속박") },
			{ TEXT("Poison"),        NSLOCTEXT("CombatStatusUI", "Poison", "중독") },
			{ TEXT("Bleed"),         NSLOCTEXT("CombatStatusUI", "Bleed", "출혈") },
			{ TEXT("Stun"),          NSLOCTEXT("CombatStatusUI", "Stun", "기절") },
			{ TEXT("Stealth"),       NSLOCTEXT("CombatStatusUI", "Stealth", "은신") },
			{ TEXT("Strength"),      NSLOCTEXT("CombatStatusUI", "Strength", "완력") },
			{ TEXT("Dexterity"),     NSLOCTEXT("CombatStatusUI", "Dexterity", "재치") },
			{ TEXT("Acumeny"),       NSLOCTEXT("CombatStatusUI", "Acumeny", "예리함") },
			{ TEXT("Pull"),          NSLOCTEXT("CombatStatusUI", "Pull", "끌기") },
			{ TEXT("Push"),          NSLOCTEXT("CombatStatusUI", "Push", "밀치기") },
			{ TEXT("Dead"),          NSLOCTEXT("CombatStatusUI", "Dead", "전투불능") },
		};
		if (const FText* Name = Names.Find(Leaf))
		{
			return *Name;
		}
		return Leaf.IsEmpty()
			? NSLOCTEXT("CombatStatusUI", "Unknown", "이상")
			: FText::FromString(Leaf);
	}

	int32 StatusPriority(const FString& Leaf, const bool bDebuff, const bool bBuff)
	{
		// 조작을 직접 막거나 지속 피해를 주는 상태는 스크롤 첫 화면에 남긴다.
		static const TMap<FString, int32> Priorities = {
			{ TEXT("Stun"), 0 }, { TEXT("Root"), 10 }, { TEXT("Poison"), 20 },
			{ TEXT("Vulnerability"), 30 }, { TEXT("Weakness"), 40 },
			{ TEXT("Frail"), 50 }, { TEXT("Exhaustion"), 60 },
			{ TEXT("Slow"), 70 },
		};
		if (const int32* Priority = Priorities.Find(Leaf))
		{
			return *Priority;
		}
		if (bDebuff)
		{
			return 80;
		}
		if (bBuff)
		{
			return 100;
		}
		return 90;
	}
}

CombatStatusUI::FPresentation CombatStatusUI::Resolve(
	const FGameplayTag& StatusTag)
{
	FPresentation Result;
	const FString FullName = StatusTag.GetTagName().ToString();
	const FString Leaf = StatusLeafName(StatusTag);
	Result.mDisplayName = LocalizedStatusName(Leaf);
	Result.mIsBuff = FullName.Contains(TEXT(".Buff."));
	Result.mIsDebuff = FullName.Contains(TEXT(".Debuff."));
	Result.mIsRoundDuration = FullName.Contains(TEXT(".RoundDuration."));
	Result.mIsInfinite = FullName.Contains(TEXT(".Infinite."));
	Result.mColor = Result.mIsDebuff
		? EFloatingLogColorType::Debuff
		: (Result.mIsBuff ? EFloatingLogColorType::Buff
			: EFloatingLogColorType::Neutral);
	Result.mSortPriority = StatusPriority(Leaf, Result.mIsDebuff, Result.mIsBuff);

	if (StatusTag.MatchesTag(
		EffectTags::GameplayEffect_StatusEffect_RoundDuration_Buff_Vigor))
	{
		Result.mFloatingIcon = EFloatingLogIconType::Vigor;
	}
	else if (StatusTag.MatchesTag(
		EffectTags::GameplayEffect_StatusEffect_RoundDuration_Buff_Fortification))
	{
		Result.mFloatingIcon = EFloatingLogIconType::Fortification;
	}
	else if (StatusTag.MatchesTag(
		EffectTags::GameplayEffect_StatusEffect_RoundDuration_Debuff_Vulnerability))
	{
		Result.mFloatingIcon = EFloatingLogIconType::Vulnerability;
	}
	else if (StatusTag.MatchesTag(
		EffectTags::GameplayEffect_StatusEffect_RoundDuration_Debuff_Weakness))
	{
		Result.mFloatingIcon = EFloatingLogIconType::Weakness;
	}
	else if (StatusTag.MatchesTag(
		EffectTags::GameplayEffect_StatusEffect_RoundDuration_Debuff_Poison))
	{
		Result.mFloatingIcon = EFloatingLogIconType::Poison;
	}
	else if (StatusTag.MatchesTag(
		EffectTags::GameplayEffect_StatusEffect_RoundDuration_Debuff_Stun))
	{
		Result.mFloatingIcon = EFloatingLogIconType::Stun;
	}
	else if (StatusTag.MatchesTag(EffectTags::GameplayEffect_StatusEffect))
	{
		// 전용 그림이 없는 상태도 이름+색+범용 표식으로 반드시 표시한다.
		Result.mFloatingIcon = EFloatingLogIconType::Status;
	}
	return Result;
}

FText CombatStatusUI::FormatDelta(const FGameplayTag& StatusTag, const int32 Delta)
{
	const FPresentation Presentation = Resolve(StatusTag);
	if (Delta > 0)
	{
		return FText::Format(NSLOCTEXT("CombatStatusUI", "StatusAdded", "{0} +{1}"),
			Presentation.mDisplayName, FText::AsNumber(Delta));
	}
	if (Delta < 0)
	{
		return FText::Format(NSLOCTEXT("CombatStatusUI", "StatusRemoved", "{0} -{1}"),
			Presentation.mDisplayName, FText::AsNumber(FMath::Abs(Delta)));
	}
	return Presentation.mDisplayName;
}

void CombatStatusUI::SortForDisplay(TArray<FStatusEffectUI>& Statuses)
{
	Statuses.StableSort([](const FStatusEffectUI& A, const FStatusEffectUI& B)
	{
		const FPresentation APresentation = Resolve(A.mTag);
		const FPresentation BPresentation = Resolve(B.mTag);
		if (APresentation.mSortPriority != BPresentation.mSortPriority)
		{
			return APresentation.mSortPriority < BPresentation.mSortPriority;
		}
		return A.mTag.GetTagName().LexicalLess(B.mTag.GetTagName());
	});
}
