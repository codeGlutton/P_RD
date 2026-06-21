#include "Pawn/UnitModel.h"
#include "Setting/GameTeamType.h"

// #include "Pawn/SkillComponent.h"
#include "Component/AttributeComponent/GameplayAttributeComponentModel.h"

UUnitModel::UUnitModel() : mTeamId(EGameTeamType::AllNeutral)
{
	// TODO : 모델로 바꿔야됨

	mAttributeCompModel = CreateDefaultSubobject<UGameplayAttributeComponentModel>(TEXT("AttributeComponentModel"));
	// mSkillComp = CreateDefaultSubobject<USkillComponent>(TEXT("SkillComp"));

	mTileLayerFlags = ETileLayerFlag::Unit;
	mBlockLayerFlags = ETileLayerFlag::Unit | ETileLayerFlag::Obstacle;
	mReplaceLayerFlags = ETileLayerFlag::None;
	mOverlayLayerPriority = 0;
}

void UUnitModel::PostInitializeComponentModels()
{
	Super::PostInitializeComponentModels();

	// TODO : 초기화 로직
	
	//mAbilitySystemComp->InitAbilityActorInfo(this, this);
	// 
	// Difficulty값에 따라 ASC Attribute 초기화
	// auto* AbilitySystemGlobals = IGameplayAbilitiesModule::Get().GetAbilitySystemGlobals();
	// AbilitySystemGlobals->InitGlobalData();
	// AbilitySystemGlobals->GetAttributeSetInitter()->InitAttributeSetDefaults(mAbilitySystemComp, GetUnitKeyName(), GetDifficulty(), true);
}

void UUnitModel::SetGenericTeamId(const FGenericTeamId& TeamID)
{
	mTeamId = TeamID;
}

FGenericTeamId UUnitModel::GetGenericTeamId() const
{
	return mTeamId;
}

UGameplayAttributeComponentModel* UUnitModel::GetAttributeComponentModel() const
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

