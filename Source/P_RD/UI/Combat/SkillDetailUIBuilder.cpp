#include "UI/Combat/SkillDetailUIBuilder.h"

#include "UI/Combat/CombatUITypes.h"
#include "Frontend/CharacterSelectTypes.h"
#include "DataAsset/ArtifactData/StaticArtifactData.h"
#include "DataAsset/SkillData/StaticSkillData.h"
#include "DataAsset/SkillData/StaticUnitSkillData.h"
#include "DataAsset/SkillData/SkillEffectLayer/SkillEffectLayer_Attack.h"
#include "DataAsset/SkillData/SkillEffectLayer/SkillEffectLayer_GetActionPoint.h"

/* CombatGameMode가 쓰던 변환과 같은 표. 여기가 유일한 기준이다. */

ECombatSkillSelectShapeUI SkillDetailUIBuilder::ToSelectShape(const EAimPattern Pattern)
{
	switch (Pattern)
	{
	case EAimPattern::Single:
		return ECombatSkillSelectShapeUI::Single;
	case EAimPattern::Cross:
		return ECombatSkillSelectShapeUI::Cross;
	case EAimPattern::Star:
		return ECombatSkillSelectShapeUI::Diagonal;
	case EAimPattern::Square:
		return ECombatSkillSelectShapeUI::Square;
	default:
		return ECombatSkillSelectShapeUI::None;
	}
}

ECombatSkillHitShapeUI SkillDetailUIBuilder::ToHitShape(const EEffectPattern Pattern)
{
	switch (Pattern)
	{
	case EEffectPattern::Single:
		return ECombatSkillHitShapeUI::Single;
	case EEffectPattern::Cross:
		return ECombatSkillHitShapeUI::Cross;
	// 게임플레이(TileMapModel::GetEffectTiles)는 Star를 직교+대각 8방향으로
	// 계산한다. Cross로 뭉개면 상세판이 대각 줄을 숨겨 실제와 어긋난다.
	case EEffectPattern::Star:
		return ECombatSkillHitShapeUI::Star;
	case EEffectPattern::Square:
		return ECombatSkillHitShapeUI::Circle;
	default:
		return ECombatSkillHitShapeUI::None;
	}
}

namespace
{
	/** @brief ETargetPattern → UI 타겟 패턴. LineToTarget만 구분하면 된다. */
	ECombatSkillTargetPatternUI ToTargetPattern(const ETargetPattern Pattern)
	{
		return Pattern == ETargetPattern::LineToTarget
			? ECombatSkillTargetPatternUI::LineToTarget
			: ECombatSkillTargetPatternUI::Default;
	}

	/* CombatGameMode 의 GetRarityColor 와 같은 표. 아티팩트 상세도 같은 색을 쓴다. */
	FLinearColor ToRarityColor(ERarityType RarityType)
	{
		switch (RarityType)
		{
		case ERarityType::Rare:
			return FLinearColor(0.55f, 0.72f, 1.0f, 1.0f);
		case ERarityType::Epic:
			return FLinearColor(0.82f, 0.58f, 1.0f, 1.0f);
		case ERarityType::Common:
		default:
			return FLinearColor(0.86f, 0.98f, 0.94f, 1.0f);
		}
	}
}

void SkillDetailUIBuilder::FillFromSkillData(const UStaticSkillData* SkillData,
	FSkillDetailUI& OutDetail)
{
	if (SkillData == nullptr)
	{
		return;
	}

	OutDetail.mName = SkillData->mName;
	OutDetail.mDescription = SkillData->mDescription;
	OutDetail.mIcon = SkillData->mIcon.LoadSynchronous();
	if (const UStaticUnitSkillData* UnitSkill = Cast<UStaticUnitSkillData>(SkillData))
	{
		OutDetail.mActionPointCost = FMath::Max(UnitSkill->mRequiredActionPoint, 0);
	}
	OutDetail.mCooldownTurns = FMath::Max(SkillData->mCooldownDuration, 0);
	for (const FSkillPhaseLayer& MotionLayer : SkillData->mSkillPhaseLayers)
	{
		for (const TInstancedStruct<FSkillEffectLayer>& EffectLayer :
			MotionLayer.mSkillEffectLayers)
		{
			if (const FSkillEffectLayer_Attack* Attack =
				EffectLayer.GetPtr<FSkillEffectLayer_Attack>())
			{
				OutDetail.mDamageMin += Attack->mMinDamage;
				OutDetail.mDamageMax += Attack->mMaxDamage;
			}
			if (const FSkillEffectLayer_GetActionPoint* Gain =
				EffectLayer.GetPtr<FSkillEffectLayer_GetActionPoint>())
			{
				OutDetail.mActionPointGain += Gain->mActionPointGain;
			}
		}
	}
	OutDetail.mCriticalDamage = FMath::RoundToInt(OutDetail.mDamageMax * 1.5f);
	OutDetail.mTargeting.mSelectShape = ToSelectShape(SkillData->mAimPattern);
	OutDetail.mTargeting.mSelectRange = StaticCast<float>(SkillData->mAimRange);
	OutDetail.mTargeting.mHitShape = ToHitShape(SkillData->mEffectPattern);
	OutDetail.mTargeting.mHitRange = StaticCast<float>(SkillData->mEffectArea);
	OutDetail.mTargeting.mAimBlockerMask = SkillData->mAimBlockerMask;
	OutDetail.mTargeting.mEffectBlockerMask = SkillData->mEffectBlockerMask;
	OutDetail.mTargeting.mTargetPattern = ToTargetPattern(SkillData->mTargetPattern);
}

void SkillDetailUIBuilder::FillFromFrontendOption(const FFrontendSkillOption& Option,
	FSkillDetailUI& OutDetail)
{
	OutDetail.mName = Option.mName;
	OutDetail.mDescription = Option.mDescription;
	OutDetail.mIcon = Option.mIcon.LoadSynchronous();
	OutDetail.mActionPointCost = Option.mActionPointCost;
	OutDetail.mActionPointGain = Option.mActionPointGain;
	OutDetail.mCooldownTurns = Option.mCooldownTurns;
	OutDetail.mDamageMin = Option.mDamageMin;
	OutDetail.mDamageMax = Option.mDamageMax;
	OutDetail.mCriticalDamage = Option.mCriticalDamage;
	OutDetail.mTargeting.mSelectShape = ToSelectShape(Option.mAimPattern);
	OutDetail.mTargeting.mSelectRange = StaticCast<float>(Option.mAimRange);
	OutDetail.mTargeting.mHitShape = ToHitShape(Option.mEffectPattern);
	OutDetail.mTargeting.mHitRange = StaticCast<float>(Option.mEffectArea);
	OutDetail.mTargeting.mAimBlockerMask = Option.mAimBlockerMask;
	OutDetail.mTargeting.mEffectBlockerMask = Option.mEffectBlockerMask;
	OutDetail.mTargeting.mTargetPattern = ToTargetPattern(Option.mTargetPattern);
}

void SkillDetailUIBuilder::FillFromArtifactData(const UStaticArtifactData* ArtifactData,
	FCombatArtifactUI& OutDetail)
{
	if (ArtifactData == nullptr)
	{
		return;
	}

	// 이름 미설정 DA(placeholder)는 에셋 이름으로 폴백한다 -- 상점 목록과 같은 규칙.
	OutDetail.mName = ArtifactData->mName.IsEmpty()
		? FText::FromName(ArtifactData->GetFName()) : ArtifactData->mName;
	OutDetail.mIcon = ArtifactData->mIcon.LoadSynchronous();
	OutDetail.mPrice = ArtifactData->mPrice;
	OutDetail.mRarityColor = ToRarityColor(ArtifactData->mRarityType);
	OutDetail.mRarityName = StaticEnum<ERarityType>() != nullptr
		? StaticEnum<ERarityType>()->GetDisplayNameTextByValue(
			StaticCast<int64>(ArtifactData->mRarityType))
		: FText::GetEmpty();
	OutDetail.mRarityLevel = StaticCast<int32>(ArtifactData->mRarityType);

	// 아티팩트 자체에는 설명 필드가 없다. 붙은 패시브의 mDescription 을 모아
	// 효과 줄로 쓴다 -- 전투 HUD/상점 상세 카드가 하던 조립과 같은 규칙.
	OutDetail.mEffectDescriptions.Reset();
	for (const TSoftObjectPtr<UStaticPassiveData>& PassiveSoft
		: ArtifactData->mStaticPassiveData)
	{
		if (const UStaticPassiveData* Passive = PassiveSoft.LoadSynchronous();
			Passive != nullptr && Passive->mDescription.IsEmpty() == false)
		{
			OutDetail.mEffectDescriptions.Add(Passive->mDescription);
		}
	}
	// 설명 없는 순수 스탯 아티팩트는 기본 문구 한 줄 -- 보상 상세와 같은 폴백.
	if (OutDetail.mEffectDescriptions.IsEmpty()
		&& ArtifactData->mStatModifiers.IsEmpty() == false)
	{
		OutDetail.mEffectDescriptions.Add(
			NSLOCTEXT("CombatLayoutHUD", "ArtifactStatOnlyEffect",
				"파티 전체 능력치를 강화합니다."));
	}
}
