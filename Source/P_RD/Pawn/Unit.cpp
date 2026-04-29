#include "Pawn/Unit.h"
#include "GAS/Attribute/UnitAttributeSet.h"
#include "SaveGame/UnitSaveGame.h"
#include "Setting/UnitTeamType.h"

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
	mDifficulty(1),
	mTeamId(EUnitTeamType::AllNeutral)
{
	PrimaryActorTick.bCanEverTick = true;

	mAbilitySystemComp = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComp"));
	mUnitAttributeSet = CreateDefaultSubobject<UUnitAttributeSet>(TEXT("UnitAttributeSet"));
}

void AUnit::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	// ASC 초기화
	mAbilitySystemComp->InitAbilityActorInfo(this, this);

#ifndef WITH_EDITOR
	// Difficulty값에 따라 ASC Attribute 초기화
	IGameplayAbilitiesModule::Get().GetAbilitySystemGlobals()->GetAttributeSetInitter()->InitAttributeSetDefaults(mAbilitySystemComp, GetRowKey(), mLevel, true);
#endif
}

void AUnit::OnConstruction(const FTransform& transform)
{
	Super::OnConstruction(transform);

#ifdef WITH_EDITOR
	// Difficulty값에 따라 ASC Attribute 초기화 (에디터 환경 내 Difficulty 변경 테스트를 위해 Construction 위치)
	IGameplayAbilitiesModule::Get().GetAbilitySystemGlobals()->GetAttributeSetInitter()->InitAttributeSetDefaults(mAbilitySystemComp, GetRowKey(), mDifficulty, true);
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

UAbilitySystemComponent* AUnit::GetAbilitySystemComponent() const
{
	return mAbilitySystemComp;
}

void AUnit::OnBeginStage()
{
}

void AUnit::OnEndStage()
{
}

void AUnit::SaveInfo(UUnitSaveGame* SaveGame) const
{
	checkf(SaveGame != nullptr, TEXT("세이브 오류: SaveGame 객체 nullptr"));

	bool IsFoundAttribute = false;
	SaveGame->mMaxHP = mAbilitySystemComp->GetGameplayAttributeValue(UUnitAttributeSet::GetMaxHPAttribute(), IsFoundAttribute);
	checkf(IsFoundAttribute == true, TEXT("세이브 오류: 찾을 수 없는 속성 값 저장 시도"));
	SaveGame->mHP = mAbilitySystemComp->GetGameplayAttributeValue(UUnitAttributeSet::GetHPAttribute(), IsFoundAttribute);
	checkf(IsFoundAttribute == true, TEXT("세이브 오류: 찾을 수 없는 속성 값 저장 시도"));

	UE_LOG(LogUnitSave, Log, TEXT("Unit 객체 저장 완료"))
}

void AUnit::LoadInfo(const UUnitSaveGame* SaveGame)
{
	checkf(SaveGame != nullptr, TEXT("로드 오류: SaveGame 객체 nullptr"));

	mAbilitySystemComp->ApplyModToAttribute(UUnitAttributeSet::GetMaxHPAttribute(), EGameplayModOp::Override, SaveGame->mMaxHP);
	mAbilitySystemComp->ApplyModToAttribute(UUnitAttributeSet::GetHPAttribute(), EGameplayModOp::Override, SaveGame->mHP);

	UE_LOG(LogUnitSave, Log, TEXT("Unit 객체 로드 완료"))
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

const FText& AUnit::GetDisplayName() const
{
	return mName;
}

FName AUnit::GetRowKey() const
{
	FString Key = mName.ToString();
	Key.RemoveSpacesInline();
	return *Key;
}

int32 AUnit::GetDifficulty() const
{
	return mDifficulty;
}