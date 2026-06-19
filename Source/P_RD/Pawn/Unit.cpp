#include "Pawn/Unit.h"
#include "GAS/Attribute/UnitAttributeSet.h"
#include "Setting/GameTeamType.h"

#include "DataAsset/UnitSpawnData/StaticUnitSpawnData.h"

#include "Pawn/SkillComponent.h"

AUnit::AUnit() :
	mTeamId(EGameTeamType::AllNeutral)
{
	PrimaryActorTick.bCanEverTick = true;

	mAbilitySystemComp = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComp"));
	mSkillComp = CreateDefaultSubobject<USkillComponent>(TEXT("SkillComp"));
}

void AUnit::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	// ASC 초기화
	mAbilitySystemComp->InitAbilityActorInfo(this, this);

#ifndef WITH_EDITOR
	// Difficulty값에 따라 ASC Attribute 초기화
	auto* AbilitySystemGlobals = IGameplayAbilitiesModule::Get().GetAbilitySystemGlobals();
	AbilitySystemGlobals->InitGlobalData();
	AbilitySystemGlobals->GetAttributeSetInitter()->InitAttributeSetDefaults(mAbilitySystemComp, GetUnitKeyName(), GetDifficulty(), true);
#endif
}

void AUnit::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

#ifdef WITH_EDITOR
	// Difficulty값에 따라 ASC Attribute 초기화 (에디터 환경 내 Difficulty 변경 테스트를 위해 Construction 위치)
	auto* AbilitySystemGlobals = IGameplayAbilitiesModule::Get().GetAbilitySystemGlobals();
	AbilitySystemGlobals->InitGlobalData();
	AbilitySystemGlobals->GetAttributeSetInitter()->InitAttributeSetDefaults(mAbilitySystemComp, GetUnitKeyName(), GetDifficulty(), true);
#endif
}

void AUnit::SetGenericTeamId(const FGenericTeamId& TeamID)
{
	mTeamId = TeamID;
}

FGenericTeamId AUnit::GetGenericTeamId() const
{
	return mTeamId;
}

const FTileTransform& AUnit::GetTileTransform() const
{
	return mTileTransform;
}

ETileLayerFlag AUnit::GetTileLayerFlags() const
{
	return ETileLayerFlag::Unit;
}

ETileLayerFlag AUnit::GetBlockLayerFlags() const
{
	return ETileLayerFlag::Unit;
}

void AUnit::SetTileTransform(const FTileTransform& Transform)
{
	mTileTransform = Transform;
}

UAbilitySystemComponent* AUnit::GetAbilitySystemComponent() const
{
	return mAbilitySystemComp;
}

FTileTargetSnapshotTargetData* AUnit::MakeSnapshotTargetData() const
{
	FTileTargetSnapshotTargetData* TargetData = new FTileTargetSnapshotTargetData();

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

	return TargetData;
}

USkillComponent* AUnit::GetSkillComponent() const
{
	return mSkillComp;
}

void AUnit::OnBeginRoom()
{
}

void AUnit::SetStaticSpawnData(const UStaticUnitSpawnData* StaticUnitSpawnData)
{
	mStaticUnitSpawnData = StaticUnitSpawnData;
}

UUnitAttributeSet* AUnit::GetUnitAttributeSet() const
{
	return mUnitAttributeSet;
}

FName AUnit::GetUnitKeyName() const
{
	checkf(mStaticUnitSpawnData != nullptr, TEXT("유닛 스폰 데이터 nullptr"));

	return mStaticUnitSpawnData->GetKeyName();
}

const FText& AUnit::GetUnitDisplayName() const
{
	checkf(mStaticUnitSpawnData != nullptr, TEXT("유닛 스폰 데이터 nullptr"));

	return mStaticUnitSpawnData->mDisplayName;
}
