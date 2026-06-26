#include "Pawn/UnitModel.h"
#include "Setting/GameTeamType.h"

// #include "Pawn/SkillComponent.h"
#include "Component/AttributeComponent/AttributeSetComponentModel.h"

#include "Singleton/WorldSubsystem/TacticalFrameworkModel.h"
#include "Singleton/WorldSubsystem/TacticalFrameworkSubsystem.h"

UUnitModel::UUnitModel() : mTeamId(EGameTeamType::AllNeutral)
{
	// TODO : 모델로 바꿔야됨

	mAttributeCompModel = CreateDefaultSubobject<UAttributeSetComponentModel>(TEXT("AttributeSetComponentModel"));
	// mSkillComp = CreateDefaultSubobject<USkillComponent>(TEXT("SkillComp"));

	mTileLayerFlags = StaticCast<int32>(ETileLayerFlag::Unit);
	mBlockLayerFlags = StaticCast<int32>(ETileLayerFlag::Unit | ETileLayerFlag::Obstacle);
	mReplaceLayerFlags = StaticCast<int32>(ETileLayerFlag::None);
	mOverlayLayerPriority = 0;
}

void UUnitModel::PostInitializeComponentModels()
{
	Super::PostInitializeComponentModels();

	UTacticalFrameworkSubsystem* TacticalFrameworkSubsystem = GetWorld()->GetSubsystem<UTacticalFrameworkSubsystem>();
	checkf(TacticalFrameworkSubsystem != nullptr, TEXT("전략 프레임워크 서브시스템 nullptr"));

	UTacticalFrameworkModel* TacticalFrameworkModel = TacticalFrameworkSubsystem->GetModel<UTacticalFrameworkModel>();
	checkf(TacticalFrameworkModel != nullptr, TEXT("전략 프레임워크 모델 nullptr"));

	TacticalFrameworkModel->GetAttributeSetInitter()->InitAttributeSetDefaults(GetAttributeComponentModel(), GetBoardActorKeyName(), GetDifficulty(), true);
}

void UUnitModel::SetGenericTeamId(const FGenericTeamId& TeamID)
{
	mTeamId = TeamID;
}

FGenericTeamId UUnitModel::GetGenericTeamId() const
{
	return mTeamId;
}

UAttributeSetComponentModel* UUnitModel::GetAttributeComponentModel() const
{
	return mAttributeCompModel;
}

FBoardCombatTargetSnapshotData* UUnitModel::MakeSnapshotData() const
{
	/*FTileTargetSnapshotTargetData* TargetData = new FTileTargetSnapshotTargetData();

	bool IsFoundAttribute = false;
	TargetData->mMaxHP = mAbilitySystemComp->GetGameplayAttributeValue(UUnitAttributeSet::GetMaxHPAttribute(), IsFoundAttribute);
	checkf(IsFoundAttribute == true, TEXT("스냅샷 오류: 찾을 수 없는 속성 값 저장 시도"));
	TargetData->mHP = mAbilitySystemComp->GetGameplayAttributeValue(UUnitAttributeSet::GetHPAttribute(), IsFoundAttribute);
	checkf(IsFoundAttribute == true, TEXT("스냅샷 오류: 찾을 수 없는 속성 값 저장 시도"));
	TargetData->mSkillPoint = mAbilitySystemComp->GetGameplayAttributeValue(UUnitAttributeSet::GetSkillPointAttribute(), IsFoundAttribute);
	checkf(IsFoundAttribute == true, TEXT("스냅샷 오류: 찾을 수 없는 속성 값 저장 시도"));
	TargetData->mDamagePoint = mAbilitySystemComp->GetGameplayAttributeValue(UUnitAttributeSet::GetDamagePointAttribute(), IsFoundAttribute);
	checkf(IsFoundAttribute == true, TEXT("스냅샷 오류: 찾을 수 없는 속성 값 저장 시도"));
	TargetData->mDefensePoint = mAbilitySystemComp->GetGameplayAttributeValue(UUnitAttributeSet::GetDefensePointAttribute(), IsFoundAttribute);
	checkf(IsFoundAttribute == true, TEXT("스냅샷 오류: 찾을 수 없는 속성 값 저장 시도"));
	TargetData->mMovementPoint = mAbilitySystemComp->GetGameplayAttributeValue(UUnitAttributeSet::GetMovementPointAttribute(), IsFoundAttribute);
	checkf(IsFoundAttribute == true, TEXT("스냅샷 오류: 찾을 수 없는 속성 값 저장 시도"));

	return TargetData;*/

	return nullptr;
}

//USkillComponent* UUnitModel::GetSkillComponent() const
//{
//	return mSkillComp;
//}

