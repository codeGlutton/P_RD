#include "UI/Combat/CombatStatusPresentation.h"

#include "GameplayTagType.h"
#include "Engine/Texture2D.h"
#include "UObject/StrongObjectPtr.h"

namespace
{
	struct FStatusArt
	{
		const TCHAR* Leaf;
		EFloatingLogIconType Icon;
		const TCHAR* Asset;
	};
	const FStatusArt StatusArt[] = {
		{TEXT("Vigor"), EFloatingLogIconType::Vigor, TEXT("Agility")},
		{TEXT("Fortification"), EFloatingLogIconType::Fortification, TEXT("Fortification")},
		{TEXT("Weakness"), EFloatingLogIconType::Weakness, TEXT("Weakness")},
		{TEXT("Vulnerability"), EFloatingLogIconType::Vulnerability, TEXT("Vulnerability")},
		{TEXT("Poison"), EFloatingLogIconType::Poison, TEXT("Poison")},
		{TEXT("Stun"), EFloatingLogIconType::Stun, TEXT("Stun")},
		{TEXT("Strength"), EFloatingLogIconType::Strength, TEXT("Strength")},
		{TEXT("Dexterity"), EFloatingLogIconType::Dexterity, TEXT("Dexterity")},
		{TEXT("Acumeny"), EFloatingLogIconType::Acumeny, TEXT("Acumeny")},
		{TEXT("Haste"), EFloatingLogIconType::Haste, TEXT("Haste")},
		{TEXT("Exhaustion"), EFloatingLogIconType::Exhaustion, TEXT("Exhaustion")},
		{TEXT("Slow"), EFloatingLogIconType::Slow, TEXT("Slow")},
		{TEXT("Frail"), EFloatingLogIconType::Frail, TEXT("Frail")},
		{TEXT("Root"), EFloatingLogIconType::Root, TEXT("Root")},
		{TEXT("ControlImmunity"), EFloatingLogIconType::ControlImmunity, TEXT("ControlImmunity")},
		{TEXT("WeakeningImmunity"), EFloatingLogIconType::WeakeningImmunity, TEXT("WeakeningImmunity")},
		{TEXT("ForcedMovementImmunity"), EFloatingLogIconType::ForcedMovementImmunity, TEXT("ForcedMovementImmunity")},
		{TEXT("Bleed"), EFloatingLogIconType::Status, TEXT("Bleed")},
		{TEXT("Stealth"), EFloatingLogIconType::Status, TEXT("Stealth")},
		{TEXT("Pull"), EFloatingLogIconType::Status, TEXT("GetMove")},
		{TEXT("Push"), EFloatingLogIconType::Status, TEXT("GetMove")},
	};

	UTexture2D* LoadStatusArt(const FStatusArt& Art)
	{
		// Static UObject pointers alone are not GC roots. Keep cached UI art alive.
		static TMap<FString, TStrongObjectPtr<UTexture2D>> Cache;
		const FString Path = FString::Printf(TEXT("%sT_Status_%s.T_Status_%s"),
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/CombatHUD/StatusIcons/"),
			Art.Asset, Art.Asset);
		if (const TStrongObjectPtr<UTexture2D>* Existing = Cache.Find(Path))
		{
			return Existing->Get();
		}
		UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, *Path);
		if (Texture)
		{
			Cache.Add(Path, TStrongObjectPtr<UTexture2D>(Texture));
		}
		return Texture;
	}

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
			{ TEXT("Weakness"),      			NSLOCTEXT("CombatStatusUI", "Weakness", "약화") },
			{ TEXT("Vulnerability"), 			NSLOCTEXT("CombatStatusUI", "Vulnerability", "취약") },
			{ TEXT("Vigor"),         			NSLOCTEXT("CombatStatusUI", "Vigor", "활력") },
			{ TEXT("Fortification"), 			NSLOCTEXT("CombatStatusUI", "Fortification", "강화") },
			{ TEXT("Haste"),         			NSLOCTEXT("CombatStatusUI", "Haste", "신속") },
			{ TEXT("Exhaustion"),    			NSLOCTEXT("CombatStatusUI", "Exhaustion", "탈진") },
			{ TEXT("Slow"),          			NSLOCTEXT("CombatStatusUI", "Slow", "둔화") },
			{ TEXT("Frail"),         			NSLOCTEXT("CombatStatusUI", "Frail", "쇠약") },
			{ TEXT("Root"),          			NSLOCTEXT("CombatStatusUI", "Root", "속박") },
			{ TEXT("Poison"),        			NSLOCTEXT("CombatStatusUI", "Poison", "중독") },
			{ TEXT("Bleed"),         			NSLOCTEXT("CombatStatusUI", "Bleed", "출혈") },
			{ TEXT("Stun"),          			NSLOCTEXT("CombatStatusUI", "Stun", "기절") },
			{ TEXT("Stealth"),       			NSLOCTEXT("CombatStatusUI", "Stealth", "은신") },
			{ TEXT("Strength"),      			NSLOCTEXT("CombatStatusUI", "Strength", "완력") },
			{ TEXT("Dexterity"),     			NSLOCTEXT("CombatStatusUI", "Dexterity", "재치") },
			{ TEXT("Acumeny"),       			NSLOCTEXT("CombatStatusUI", "Acumeny", "예리함") },
			{ TEXT("Pull"),          			NSLOCTEXT("CombatStatusUI", "Pull", "끌기") },
			{ TEXT("Push"),          			NSLOCTEXT("CombatStatusUI", "Push", "밀치기") },
			{ TEXT("ControlImmunity"), 			NSLOCTEXT("CombatStatusUI", "ControlImmunity", "억제 면역") },
			{ TEXT("WeakeningImmunity"), 		NSLOCTEXT("CombatStatusUI", "WeakeningImmunity", "쇠약 면역") },
			{ TEXT("ForcedMovementImmunity"), 	NSLOCTEXT("CombatStatusUI", "ForcedMovementImmunity", "강제 이동 면역") },
			{ TEXT("Dead"),          			NSLOCTEXT("CombatStatusUI", "Dead", "전투불능") },
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

	if (StatusTag.MatchesTag(EffectTags::GameplayEffect_StatusEffect))
	{
		Result.mFloatingIcon = EFloatingLogIconType::Status;
		for (const FStatusArt& Art : StatusArt)
		{
			if (Leaf == Art.Leaf)
			{
				Result.mFloatingIcon = Art.Icon;
				break;
			}
		}
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

UTexture2D* CombatStatusUI::ResolveIcon(const FGameplayTag& StatusTag)
{
	if (!StatusTag.MatchesTag(EffectTags::GameplayEffect_StatusEffect))
	{
		return nullptr;
	}
	const FString Leaf = StatusLeafName(StatusTag);
	for (const FStatusArt& Art : StatusArt)
	{
		if (Leaf == Art.Leaf)
		{
			if (UTexture2D* Texture = LoadStatusArt(Art))
			{
				return Texture;
			}
		}
	}
	return nullptr;
}

UTexture2D* CombatStatusUI::ResolveIcon(const EFloatingLogIconType IconType)
{
	if (IconType == EFloatingLogIconType::Status || IconType == EFloatingLogIconType::None)
	{
		return nullptr;
	}
	for (const FStatusArt& Art : StatusArt)
	{
		if (Art.Icon == IconType)
		{
			return LoadStatusArt(Art);
		}
	}
	return nullptr;
}

namespace
{
const TMap<FString, FText>& StatusDescriptions()
{
    static const TMap<FString, FText> Descriptions = {
		{ TEXT("Strength"),      NSLOCTEXT("CombatLayoutHUD", "StatusDescStrength", "공격 관련 능력이 강화된다.") },
		{ TEXT("Dexterity"),     NSLOCTEXT("CombatLayoutHUD", "StatusDescDexterity", "기교 관련 능력이 강화된다.") },
		{ TEXT("Acumeny"),       NSLOCTEXT("CombatLayoutHUD", "StatusDescAcumeny", "판단 관련 능력이 강화된다.") },
		{ TEXT("Fortification"), NSLOCTEXT("CombatLayoutHUD", "StatusDescFortification", "방어도를 얻을 때 받는 양이 늘어납니다.") },
		{ TEXT("Vulnerability"), NSLOCTEXT("CombatLayoutHUD", "StatusDescVulnerability", "받는 피해가 늘어난다.") },
		{ TEXT("Weakness"),      NSLOCTEXT("CombatLayoutHUD", "StatusDescWeakness", "주는 피해가 줄어든다.") },
		{ TEXT("Vigor"),         NSLOCTEXT("CombatLayoutHUD", "StatusDescVigor", "행동력(AP)을 회복하거나 얻을 때 받는 양이 늘어납니다.") },
		{ TEXT("Haste"),         NSLOCTEXT("CombatLayoutHUD", "StatusDescHaste", "속도가 올라간다.") },
		{ TEXT("Exhaustion"),    NSLOCTEXT("CombatLayoutHUD", "StatusDescExhaustion", "행동력(AP)을 회복하거나 얻을 때 받는 양이 줄어듭니다. 한 턴에 이동하거나 스킬을 사용할 여유가 줄어들어요.") },
		{ TEXT("Slow"),          NSLOCTEXT("CombatLayoutHUD", "StatusDescSlow", "속도가 내려간다.") },
		{ TEXT("Frail"),         NSLOCTEXT("CombatLayoutHUD", "StatusDescFrail", "방어도를 얻을 때 받는 양이 줄어듭니다.") },
		{ TEXT("Root"),          NSLOCTEXT("CombatLayoutHUD", "StatusDescRoot", "이동할 수 없다.") },
		{ TEXT("Poison"),        NSLOCTEXT("CombatLayoutHUD", "StatusDescPoison", "라운드가 지날 때 피해를 입습니다.") },
		{ TEXT("Bleed"),         NSLOCTEXT("CombatLayoutHUD", "StatusDescBleed", "라운드가 지날 때 피해를 입습니다.") },
		{ TEXT("Stun"),                   NSLOCTEXT("CombatLayoutHUD", "StatusDescStun", "이동과 스킬을 사용할 수 없다.") },
		{ TEXT("Stealth"),                NSLOCTEXT("CombatLayoutHUD", "StatusDescStealth", "적의 대상이 되지 않는다.") },
		{ TEXT("ControlImmunity"),        NSLOCTEXT("CombatLayoutHUD", "StatusDescControlImmunity", "기절과 속박에 걸리는 것을 막아줍니다.") },
		{ TEXT("WeakeningImmunity"),      NSLOCTEXT("CombatLayoutHUD", "StatusDescWeakeningImmunity", "탈진, 둔화, 약화 같은 능력 저하 효과에 걸리는 것을 막아줍니다.") },
		{ TEXT("ForcedMovementImmunity"), NSLOCTEXT("CombatLayoutHUD", "StatusDescForcedMovementImmunity", "밀치기나 끌어당기기로 위치가 바뀌는 것을 막아줍니다.") },
		{ TEXT("Pull"),                   NSLOCTEXT("CombatLayoutHUD", "StatusDescPull", "대상을 시전자 쪽으로 끌어당깁니다.") },
		{ TEXT("Push"),                   NSLOCTEXT("CombatLayoutHUD", "StatusDescPush", "대상을 반대 방향으로 밀어냅니다.") },
	};
    return Descriptions;
}
}
bool CombatStatusUI::HasDescription(const FGameplayTag& Tag)
{
    return StatusDescriptions().Contains(StatusLeafName(Tag));
}
FText CombatStatusUI::Describe(const FGameplayTag& Tag)
{
    const FText* Description = StatusDescriptions().Find(StatusLeafName(Tag));
    const auto Info = Resolve(Tag);
    const FText Duration = Info.mIsInfinite
        ? NSLOCTEXT("CombatLayoutHUD", "StatusDurationInfinite", "전투가 끝날 때까지 지속된다.")
        : Info.mIsRoundDuration
            ? NSLOCTEXT("CombatLayoutHUD", "StatusDurationRound", "라운드가 지나면 1중첩씩 사라진다.")
            : NSLOCTEXT("CombatLayoutHUD", "StatusDurationOther", "효과 조건이 끝날 때까지 지속된다.");
    return FText::Format(NSLOCTEXT("CombatLayoutHUD", "StatusDescBodyFmt", "{0}\n{1}"),
        Description ? *Description : NSLOCTEXT("CombatLayoutHUD", "StatusDescMissing", "효과 설명이 아직 없다."), Duration);
}
