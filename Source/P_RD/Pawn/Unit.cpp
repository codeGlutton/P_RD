#include "Pawn/Unit.h"
#include "GAS/Attribute/UnitAttributeSet.h"
#include "Setting/UnitTeamType.h"

#include "DataAsset/UnitSpawnData/StaticUnitSpawnData.h"

#include "Pawn/SkillComponent.h"

UScriptStruct* FUnitSnapshotTargetData::GetScriptStruct() const
{
	return FUnitSnapshotTargetData::StaticStruct();
}

bool FUnitSnapshotTargetData::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& OutSuccess)
{
	Ar << mMaxHP;
	Ar << mHP;
	Ar << mSkillPoint;
	Ar << mDamagePoint;
	Ar << mDefensePoint;
	Ar << mMovementPoint;
	Ar << mBuffState;
	Ar << mDebuffState;

	return true;
}

AUnit::AUnit() :
	mTeamId(EUnitTeamType::AllNeutral)
{
	PrimaryActorTick.bCanEverTick = true;

	mAbilitySystemComp = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComp"));
	mUnitAttributeSet = CreateDefaultSubobject<UUnitAttributeSet>(TEXT("UnitAttributeSet"));

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

ETileLayerFlag AUnit::GetTileLayer() const
{
	return ETileLayerFlag::Unit;
}

ETileLayerFlag AUnit::GetBlockFlags() const
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

USkillComponent* AUnit::GetSkillComponent() const
{
	return mSkillComp;
}

void AUnit::OnBeginPlayRoom()
{
}

void AUnit::OnEndPlayRoom()
{
}

FUnitSnapshotTargetData* AUnit::MakeSnapshotTargetData() const
{
	FUnitSnapshotTargetData* TargetData = new FUnitSnapshotTargetData();

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

void AUnit::SetStaticSpawnData(const UStaticUnitSpawnData* StaticUnitSpawnData)
{
	mStaticUnitSpawnData = StaticUnitSpawnData;
}

UUnitAttributeSet* AUnit::GetUnitAttributeSet() const
{
	return mUnitAttributeSet;
}

bool AUnit::IsDead() const
{
	return mAbilitySystemComp->HasMatchingGameplayTag(EffectTags::GameplayEffect_ActorState_Dead);
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
