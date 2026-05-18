#include "Singleton/InstanceSubsystem/PersistentDataSubsystem.h"
#include "GAS/Attribute/UnitAttributeSet.h"
#include "Pawn/Player/PlayerUnit.h"

#include "Setting/GlobalGameBalanceSettings.h"
#include "Engine/AssetManager.h"
#include "PCGStage/StageBuilder.h"

#include "FunctionLibrary/RandomStreamFunctionLibrary.h"

void UUserPersistData::MakeNewUser(FName Name)
{
	mUserName = Name;
	mRunCount = 0;
}

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

int32 UPlayerUnitPersistData::GetPlayerLevel() const
{
	return mPlayerLevel;
}

int32 UPlayerUnitPersistData::GetDifficulty() const
{
	return mDifficulty;
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

void URunPersistData::MakeNewRun(int32 Difficulty)
{
	ClearRunData();

	mStageBuildStream.Initialize(FMath::Rand32());
	mEventStream.Initialize(FMath::Rand32());

	mIsNewData = true;
	mDifficulty = Difficulty;
}

void URunPersistData::MakeStageAsync(EStageLevelType Type, FOnCreateStage OnCreateStage)
{
	UAssetManager* AssetManager = UAssetManager::GetIfInitialized();
	checkf(AssetManager != nullptr, TEXT("에셋 매니저 nullptr"));

	const UGlobalGameBalanceSettings* GlobalGameBalanceSetting = GetDefault<UGlobalGameBalanceSettings>();
	checkf(GlobalGameBalanceSetting != nullptr, TEXT("글로벌 밸런스 세팅 객체 nullptr"));

	GlobalGameBalanceSetting->mStageBuildSettingTable.LoadAsync(FLoadSoftObjectPathAsyncDelegate::CreateLambda([this, Type, OnCreateStage = MoveTemp(OnCreateStage), GlobalGameBalanceSetting](const FSoftObjectPath& Path, UObject* Object) {
		
		const UDataTable* BalanceSetting = Cast<UDataTable>(Object);
		checkf(BalanceSetting != nullptr, TEXT("스테이지 빌드 세팅 미존재 nullptr"));

		const FStageBuilderParams& BuilderParams = *BalanceSetting->FindRow<FStageBuilderParams>(*EnumToString(Type), TEXT("밸런스 세팅 테이블 탐색 에러"));
		const FRandomStream& BuildStream = URandomStreamFunctionLibrary::GetStageBuildStream(this);

		mStage = FStageBuilder::Make(BuildStream, GlobalGameBalanceSetting->mGlobalStageBuildSetting, BuilderParams).Build();
		OnCreateStage.ExecuteIfBound(mStage);

		}));
}

const FRandomStream& URunPersistData::GetStageBuildStream() const
{
	return mStageBuildStream;
}

const FRandomStream& URunPersistData::GetEventStream() const
{
	return mEventStream;
}

const FStage& URunPersistData::GetStage() const
{
	return mStage;
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

void URunPersistData::ClearRunData()
{
	mPlayerLevel = 1;
	mDifficulty = 1;
	mIsNewData = true;

	mTagCountMap.Empty();
	mEquipmentIds.Empty();
}
