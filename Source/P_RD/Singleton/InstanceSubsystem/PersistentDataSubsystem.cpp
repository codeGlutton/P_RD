#include "Singleton/InstanceSubsystem/PersistentDataSubsystem.h"
#include "GAS/Attribute/UnitAttributeSet.h"

#include "Pawn/Player/PlayerUnit.h"

const FName& UUserPersistData::GetUserName() const
{
	return mUserName;
}

int32 UUserPersistData::GetRunCount() const
{
	return mRunCount;
}

void UPlayerUnitPersistData::RegisterUnit(APlayerUnit* PlayerUnit)
{
	checkf(PlayerUnit != nullptr, TEXT("플레이어 유닛 nullptr"));
	UAbilitySystemComponent* ASC = PlayerUnit->GetAbilitySystemComponent();
	checkf(ASC != nullptr, TEXT("어빌리티 시스템 컴포넌트 nullptr"));

	ApplyPersistData(PlayerUnit);
	BindUnitEvent(PlayerUnit);
}

void UPlayerUnitPersistData::ApplyPersistData(APlayerUnit* PlayerUnit)
{
	checkf(PlayerUnit != nullptr, TEXT("플레이어 유닛 nullptr"));
	UAbilitySystemComponent* ASC = PlayerUnit->GetAbilitySystemComponent();
	checkf(ASC != nullptr, TEXT("어빌리티 시스템 컴포넌트 nullptr"));

	if (mIsNewData == true)
	{
		mIsNewData = false;
		return;
	}

	ASC->ApplyModToAttribute(UPlayerUnitAttributeSet::GetMaxHPAttribute(), EGameplayModOp::Override, mMaxHP);
	ASC->ApplyModToAttribute(UPlayerUnitAttributeSet::GetHPAttribute(), EGameplayModOp::Override, mHP);
	ASC->ApplyModToAttribute(UPlayerUnitAttributeSet::GetLevelAttribute(), EGameplayModOp::Override, mLevel);
	ASC->ApplyModToAttribute(UPlayerUnitAttributeSet::GetExpAttribute(), EGameplayModOp::Override, mExp);
	ASC->ApplyModToAttribute(UPlayerUnitAttributeSet::GetMoneyAttribute(), EGameplayModOp::Override, mMoney);
	for (auto& Pair : mTagCountMap)
	{
		ASC->AddLooseGameplayTag(Pair.Key, Pair.Value);
	}
}

void UPlayerUnitPersistData::BindUnitEvent(APlayerUnit* PlayerUnit)
{
	checkf(PlayerUnit != nullptr, TEXT("플레이어 유닛 nullptr"));
	UAbilitySystemComponent* ASC = PlayerUnit->GetAbilitySystemComponent();
	checkf(ASC != nullptr, TEXT("어빌리티 시스템 컴포넌트 nullptr"));

	ASC->GetGameplayAttributeValueChangeDelegate(UPlayerUnitAttributeSet::GetMaxHPAttribute()).AddLambda([this](const FOnAttributeChangeData& Data) {
		mMaxHP = Data.NewValue;
		});
	ASC->GetGameplayAttributeValueChangeDelegate(UPlayerUnitAttributeSet::GetHPAttribute()).AddLambda([this](const FOnAttributeChangeData& Data) {
		mHP = Data.NewValue;
		});
	ASC->GetGameplayAttributeValueChangeDelegate(UPlayerUnitAttributeSet::GetLevelAttribute()).AddLambda([this](const FOnAttributeChangeData& Data) {
		mLevel = Data.NewValue;
		});
	ASC->GetGameplayAttributeValueChangeDelegate(UPlayerUnitAttributeSet::GetExpAttribute()).AddLambda([this](const FOnAttributeChangeData& Data) {
		mExp = Data.NewValue;
		});
	ASC->GetGameplayAttributeValueChangeDelegate(UPlayerUnitAttributeSet::GetMoneyAttribute()).AddLambda([this](const FOnAttributeChangeData& Data) {
		mMoney = Data.NewValue;
		});
	ASC->RegisterGameplayTagEvent(EffectTags::GameplayEffect_Cost_PassiveStack, EGameplayTagEventType::AnyCountChange).AddLambda([this](const FGameplayTag Tag, int32 Count) {
		mTagCountMap[Tag] = Count;
		if (Count == 0)
		{
			mTagCountMap.Remove(Tag);
		}
		});
}

const FRandomStream& URunPersistData::GetStageBuildStream() const
{
	return mStageBuildStream;
}

const FRandomStream& URunPersistData::GetEventStream() const
{
	return mEventStream;
}

const FRandomStream& URunPersistData::GetCombatStream() const
{
	return mCombatStream;
}

void UPersistentDataSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	mUserPersistData = NewObject<UUserPersistData>(this);
	mRunPersistData = NewObject<URunPersistData>(this);
}

UUserPersistData* UPersistentDataSubsystem::GetUserPersistData()
{
	return mUserPersistData;
}

URunPersistData* UPersistentDataSubsystem::GetRunPersistData()
{
	return mRunPersistData;
}

