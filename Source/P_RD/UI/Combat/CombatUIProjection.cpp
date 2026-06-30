#include "UI/Combat/CombatUIProjection.h"

#include "GameFramework/Actor.h"
#include "ObjectView.h"   // IObjectView 완전형 — UUnitModel::GetView<AActor>()의 Cast<>에 필요

#include "Singleton/WorldSubsystem/SRPGCombatModel.h"
#include "SRPGFramework/SRPGTurnContext.h"

#include "Pawn/UnitModel.h"
#include "Pawn/Player/PlayerUnitModel.h"

#include "Component/AttributeComponent/AttributeSetComponentModel.h"
#include "Component/EquipmentComponent/EquipmentComponentModel.h"
#include "Component/SkillComponent/SkillComponentModel.h"

#include "DataAsset/EquipmentData/StaticEquipmentData.h"
#include "DataAsset/SkillData/StaticSkillData.h"

#include "Dice/DiceModel.h"
#include "Dice/DicePoolModel.h"

#include "AttributeSet/UnitAttributeSet.h"

// 변환에만 쓰는 파일 내부 헬퍼(게임플레이 enum/데이터 -> UI 표현값).
namespace
{
	FLinearColor GetCombatRarityColor(ERarityType RarityType)
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

	ECombatSkillSelectShapeUI ToCombatSkillSelectShape(EAimPattern Pattern)
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

	ECombatSkillHitShapeUI ToCombatSkillHitShape(EEffectPattern Pattern)
	{
		switch (Pattern)
		{
		case EEffectPattern::Single:
			return ECombatSkillHitShapeUI::Single;
		case EEffectPattern::Cross:
		case EEffectPattern::Star:
			return ECombatSkillHitShapeUI::Cross;
		case EEffectPattern::Square:
			return ECombatSkillHitShapeUI::Circle;
		case EEffectPattern::Beam:
			return ECombatSkillHitShapeUI::Single;
		default:
			return ECombatSkillHitShapeUI::None;
		}
	}

	FText GetEquipmentSlotFallbackName(EEquipmentType Slot)
	{
		switch (Slot)
		{
		case EEquipmentType::Weapon:
			return NSLOCTEXT("CombatGameMode", "EquipmentSlotWeapon", "WEAPON");
		case EEquipmentType::Gloves:
			return NSLOCTEXT("CombatGameMode", "EquipmentSlotGloves", "GLOVES");
		case EEquipmentType::Boots:
			return NSLOCTEXT("CombatGameMode", "EquipmentSlotBoots", "BOOTS");
		default:
			return NSLOCTEXT("CombatGameMode", "EquipmentSlotEmpty", "EMPTY");
		}
	}
}

ECombatBuildPhaseUI CombatUIProjection::ToCombatBuildPhaseUI(ESRPGSkillBuildPhase Phase)
{
	switch (Phase)
	{
	case ESRPGSkillBuildPhase::AimSelection:
		return ECombatBuildPhaseUI::AimSelection;
	case ESRPGSkillBuildPhase::Preview:
		return ECombatBuildPhaseUI::Preview;
	case ESRPGSkillBuildPhase::Build:
	case ESRPGSkillBuildPhase::None:
	default:
		return ECombatBuildPhaseUI::None;
	}
}

ECombatBuildPhaseUI CombatUIProjection::ToCombatBuildPhaseUI(ESRPGMoveBuildPhase Phase)
{
	switch (Phase)
	{
	case ESRPGMoveBuildPhase::DestSelection:
		return ECombatBuildPhaseUI::AimSelection;
	case ESRPGMoveBuildPhase::Preview:
		return ECombatBuildPhaseUI::Preview;
	case ESRPGMoveBuildPhase::Build:
	case ESRPGMoveBuildPhase::None:
	default:
		return ECombatBuildPhaseUI::None;
	}
}

TArray<FUnitUI> CombatUIProjection::BuildUnitUIs(USRPGCombatModel* CombatModel)
{
	TArray<FUnitUI> UnitUIs;
	if (CombatModel == nullptr)
	{
		return UnitUIs;
	}

	for (const TObjectPtr<UUnitModel>& Unit : CombatModel->GetUnits())
	{
		if (Unit == nullptr)
		{
			continue;
		}

		FUnitUI UnitUI;
		UnitUI.mUnitId = Unit->GetModelId();
		UnitUI.mIsPlayer = Unit->IsPlayerUnitModel();
		UnitUI.mTile = Unit->GetTileTransform().mIndex;

		if (AActor* UnitActor = Unit->GetView<AActor>())
		{
			UnitUI.mWorldLocation = UnitActor->GetActorLocation();
		}

		if (UAttributeSetComponentModel* AttributeComponentModel = Unit->GetAttributeComponentModel())
		{
			UnitUI.mHP = AttributeComponentModel->GetAttributeCurrentValue(UUnitAttributeSet::GetHPAttribute());
			UnitUI.mMaxHP = AttributeComponentModel->GetAttributeCurrentValue(UUnitAttributeSet::GetMaxHPAttribute());
			// [216 통합] UnitUI.mDamagePoint는 의도적으로 미할당(기본값 0) — 215가 DamagePoint 속성을 제거함.
			//            TODO: AttackPoint/AttackFactor 중 정책 확정 후 mDamagePoint 매핑.
			UnitUI.mDefensePoint = AttributeComponentModel->GetAttributeCurrentValue(UUnitAttributeSet::GetDefensePointAttribute());
			UnitUI.mMovementPoint = AttributeComponentModel->GetAttributeCurrentValue(UUnitAttributeSet::GetMovementPointAttribute());
			UnitUI.mMaxMovementPoint = UnitUI.mMovementPoint;
			UnitUI.mSkillPoint = AttributeComponentModel->GetAttributeCurrentValue(UUnitAttributeSet::GetAttackPointAttribute());
			UnitUI.mStatusTags = AttributeComponentModel->GetOwnedGameplayTags();
		}

		UnitUIs.Add(UnitUI);
	}

	return UnitUIs;
}

TArray<FDiceSlotUI> CombatUIProjection::BuildDiceUIs(UDicePoolModel* DicePoolModel)
{
	TArray<FDiceSlotUI> DiceUIs;
	if (DicePoolModel == nullptr)
	{
		return DiceUIs;
	}

	const TArray<TObjectPtr<UDiceModel>>& Dices = DicePoolModel->GetDices();
	DiceUIs.Reserve(Dices.Num());

	for (int32 DiceIndex = 0; DiceIndex < Dices.Num(); ++DiceIndex)
	{
		const UDiceModel* Dice = Dices[DiceIndex];
		if (Dice == nullptr)
		{
			continue;
		}

		FDiceSlotUI DiceUI;
		DiceUI.mDiceId = Dice->GetSourceDiceId();
		DiceUI.mResultValue = Dice->IsRolled() ? Dice->GetCurrentValue() : 0;
		DiceUI.mRolledFaceIndex = Dice->GetRolledFaceIndex();
		DiceUI.mIsRolled = Dice->IsRolled();
		DiceUI.mIsSelected = DicePoolModel->IsSelectedDice(DiceIndex);
		DiceUI.mIsUsed = Dice->IsUsed();
		DiceUI.mFaceCount = Dice->GetFaceCount();
		DiceUI.mFaceValues = Dice->GetFaceValues();
		DiceUI.mFaceTextures = Dice->GetFaceTextures();

		DiceUIs.Add(DiceUI);
	}

	return DiceUIs;
}

TArray<int32> CombatUIProjection::BuildSelectedDiceIndices(UDicePoolModel* DicePoolModel)
{
	TArray<int32> SelectedDiceIndices;
	if (DicePoolModel == nullptr)
	{
		return SelectedDiceIndices;
	}

	const TArray<TObjectPtr<UDiceModel>>& Dices = DicePoolModel->GetDices();
	for (int32 DiceIndex = 0; DiceIndex < Dices.Num(); ++DiceIndex)
	{
		if (DicePoolModel->IsSelectedDice(DiceIndex))
		{
			SelectedDiceIndices.Add(DiceIndex);
		}
	}

	return SelectedDiceIndices;
}

FTurnUI CombatUIProjection::BuildTurnUI(USRPGCombatModel* CombatModel)
{
	FTurnUI TurnUI;
	if (CombatModel == nullptr)
	{
		return TurnUI;
	}

	if (USRPGTurnContext* CurrentTurnContext = CombatModel->GetCurrentTurnContext())
	{
		if (UUnitModel* TurnOwner = CurrentTurnContext->GetOwner())
		{
			TurnUI.mCurrentUnitId = TurnOwner->GetModelId();
		}
	}

	return TurnUI;
}

TArray<FSkillUI> CombatUIProjection::BuildSkillUIs(USkillComponentModel* SkillComponentModel)
{
	TArray<FSkillUI> SkillUIs;
	if (SkillComponentModel == nullptr)
	{
		return SkillUIs;
	}

	const int32 SkillCount = SkillComponentModel->GetSkillNum();
	SkillUIs.Reserve(SkillCount);

	for (int32 SkillIndex = 0; SkillIndex < SkillCount; ++SkillIndex)
	{
		FSkillUI SkillUI;
		SkillUI.mSkillIndex = SkillIndex;
		SkillUI.mIsUsable = false;

		// [216 통합] 215의 새 스킬 API. FSkillEntry.mData는 이미 로드된 UStaticSkillData.
		FSkillEntry* SkillEntry = SkillComponentModel->GetSkill(SkillIndex);
		UStaticSkillData* StaticSkillData = (SkillEntry != nullptr) ? SkillEntry->mData : nullptr;
		if (StaticSkillData == nullptr)
		{
			SkillUIs.Add(SkillUI);
			continue;
		}

		SkillUI.mName = StaticSkillData->mName;
		SkillUI.mDescription = StaticSkillData->mDescription;
		SkillUI.mIcon = StaticSkillData->mIcon.LoadSynchronous();
		SkillUI.mDiceCost = StaticSkillData->mRequiredDiceCount;
		SkillUI.mIsUsable = true;
		SkillUI.mTargeting.mSelectShape = ToCombatSkillSelectShape(StaticSkillData->mAimPattern);
		SkillUI.mTargeting.mSelectRange = StaticCast<float>(StaticSkillData->mAimRangeDefaultValue);
		SkillUI.mTargeting.mSelectRangeRatio = StaticSkillData->mAimRangeRatio;
		SkillUI.mTargeting.mHitShape = ToCombatSkillHitShape(StaticSkillData->mEffectPattern);
		SkillUI.mTargeting.mHitRange = StaticCast<float>(StaticSkillData->mEffectAreaDefaultValue);
		SkillUI.mTargeting.mHitRangeRatio = StaticSkillData->mEffectAreaRatio;
		SkillUI.mTargeting.mIsIndirect = StaticSkillData->mIsIndirect;
		SkillUI.mTargeting.mIsPenetration = StaticSkillData->mIsPenetration;
		SkillUIs.Add(SkillUI);
	}

	return SkillUIs;
}

TArray<FEquipmentUI> CombatUIProjection::BuildEquipmentUIs(UEquipmentComponentModel* EquipmentComponentModel)
{
	TArray<FEquipmentUI> EquipmentUIs;
	if (EquipmentComponentModel == nullptr)
	{
		return EquipmentUIs;
	}

	const int32 SlotCount = StaticCast<int32>(EEquipmentType::Count);
	EquipmentUIs.Reserve(SlotCount);

	for (int32 SlotIndex = 0; SlotIndex < SlotCount; ++SlotIndex)
	{
		const EEquipmentType Slot = StaticCast<EEquipmentType>(SlotIndex);
		const FEquippedEntry* EquippedEntry = EquipmentComponentModel->GetEquipped(Slot);
		const UStaticEquipmentData* StaticEquipmentData = EquippedEntry != nullptr ? EquippedEntry->mData.Get() : nullptr;

		FEquipmentUI EquipmentUI;
		EquipmentUI.mSlotIndex = SlotIndex;
		EquipmentUI.mName = GetEquipmentSlotFallbackName(Slot);

		if (StaticEquipmentData != nullptr)
		{
			EquipmentUI.mItemId = StaticEquipmentData->GetPrimaryAssetId();
			EquipmentUI.mName = StaticEquipmentData->mName.IsEmpty() ? EquipmentUI.mName : StaticEquipmentData->mName;
			EquipmentUI.mIcon = StaticEquipmentData->mIcon.LoadSynchronous();
			EquipmentUI.mIsEquipped = true;
			EquipmentUI.mRarityColor = GetCombatRarityColor(StaticEquipmentData->mRarityType);
		}

		EquipmentUIs.Add(EquipmentUI);
	}

	return EquipmentUIs;
}

FPlayerMetaUI CombatUIProjection::BuildPlayerMetaUI(UPlayerUnitModel* PlayerUnit)
{
	FPlayerMetaUI MetaUI;
	if (PlayerUnit == nullptr)
	{
		return MetaUI;
	}

	MetaUI.mLevel = PlayerUnit->GetPlayerLevel();

	if (UAttributeSetComponentModel* AttributeComponentModel = PlayerUnit->GetAttributeComponentModel())
	{
		MetaUI.mGold = FMath::RoundToInt(AttributeComponentModel->GetAttributeCurrentValue(UPlayerUnitAttributeSet::GetMoneyAttribute()));
		MetaUI.mExp = AttributeComponentModel->GetAttributeCurrentValue(UPlayerUnitAttributeSet::GetExpAttribute());
		MetaUI.mMaxExp = AttributeComponentModel->GetAttributeCurrentValue(UPlayerUnitAttributeSet::GetMaxExpAttribute());
	}

	return MetaUI;
}
