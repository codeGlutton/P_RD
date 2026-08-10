#include "Singleton/InstanceSubsystem/PersistentData.h"

#include "Containers/Ticker.h"
#include "AttributeSet/PartyAttributeSet.h"
#include "AttributeSet/UnitAttributeSet.h"

#include "Singleton/WorldSubsystem/TacticalFrameworkModel.h"

#include "Actor/Party/PartyModel.h"
#include "Pawn/Player/PlayerUnitModel.h"

#include "Component/AttributeComponent/AttributeSetComponentModel.h"
#include "Component/SkillComponent/SkillComponentModel.h"
#include "Component/ArtifactComponent/PartyArtifactComponentModel.h"
#include "AttributeSet/LevelAttributeSet.h"

#include "Setting/GameBalanceSettings.h"
#include "Engine/AssetManager.h"
#include "PCGStage/StageBuilder.h"

#include "DataAsset/UnitSpawnData/StaticPlayerUnitSpawnData.h"
#include "DataAsset/SkillData/StaticSkillData.h"
#include "DataAsset/ArtifactData/StaticArtifactData.h"

#include "FunctionLibrary/RandomStreamFunctionLibrary.h"

#include "Setting/GamePlaySettings.h"
#include "HAL/IConsoleManager.h"
#include "Internationalization/TextLocalizationManager.h"
#include "Sound/SoundClass.h"

#if !UE_BUILD_SHIPPING

namespace
{
	int32 GFixedStageBuildSeedForDebugging = INDEX_NONE;
	int32 GFixedEventSeedForDebugging = INDEX_NONE;

	FAutoConsoleVariableRef CFixedStageBuildSeedForDebugging(
		TEXT("Seed.FixedStageBuildSeed"),
		GFixedStageBuildSeedForDebugging,
		TEXT("디버깅을 위해서 스테이지 빌드 시드를 고정"),
		ECVF_Default
	);
	FAutoConsoleVariableRef CFixedEventSeedForDebugging(
		TEXT("Seed.FixedEventSeed"),
		GFixedEventSeedForDebugging,
		TEXT("디버깅을 위해서 이벤트 시드를 고정"),
		ECVF_Default
	);
}

#endif

void FRunLog::Clear()
{
	mUseCountPerUnit.Empty();
	mKilledEnemyUnits.Empty();
	mAcquiredSkills.Empty();
	mAcquiredEquipment.Empty();
	mAcquiredDices.Empty();
}

void FUserLog::Clear()
{
	mRunCount = 0;
	mUseCountPerUnit.Empty();
	mKnownEnemyUnitIds.Empty();
	mKnownSkillIds.Empty();
	mKnownEquipmentIds.Empty();
	mKnownDiceIds.Empty();
}

void UPlayerUnitPersistData::MakeUnit(const FPrimaryAssetId& PlayerUnitId)
{
	UAssetManager* AssetManager = UAssetManager::GetIfInitialized();
	checkf(AssetManager != nullptr, TEXT("에셋 매니저 nullptr"));

	const UStaticPlayerUnitSpawnData* PlayerData = AssetManager->GetPrimaryAssetObject<UStaticPlayerUnitSpawnData>(PlayerUnitId);
	checkf(PlayerData != nullptr, TEXT("플레이어 데이터 nullptr"));

	ClearUnit();

	// 플레이어 기본 데이터 세팅
	{
		mPlayerUnitId = PlayerUnitId;
		mPlayerLevel = 1;
	}

	// 플레이어 기본 속성 세팅
	{
		mMaxHP = PlayerData->GetDefaultAttributeValue(GetWorld(), UPlayerUnitAttributeSet::StaticClass(), UPlayerUnitAttributeSet::GetMaxHPAttribute(), mPlayerLevel);
		mHP = PlayerData->GetDefaultAttributeValue(GetWorld(), UPlayerUnitAttributeSet::StaticClass(), UPlayerUnitAttributeSet::GetHPAttribute(), mPlayerLevel);
		mHP = mHP > 0.f ? FMath::Min(mHP, mMaxHP) : mMaxHP;
		mExp = 0.f;
	}

	// 스킬 기본 값 세팅
	{
		constexpr int32 PlayerSkillSlot = 6;
		mSkillIds.Init(FPrimaryAssetId(), PlayerSkillSlot);

		const int32 StaticSkillCount = PlayerData->mSkillDatas.Num();
		for (int32 i = 0; i < StaticSkillCount; ++i)
		{
			const TSoftObjectPtr<UStaticSkillData>& SkillSoft = PlayerData->mSkillDatas[i];
			if (const UStaticSkillData* SkillData = SkillSoft.LoadSynchronous())
			{
				mSkillIds[i] = SkillData->GetPrimaryAssetId();
			}
		}
	}
}

void UPlayerUnitPersistData::MakeUnit(UPlayerUnitModel* PlayerUnit)
{
	checkf(PlayerUnit != nullptr, TEXT("플레이어 유닛 모델 nullptr"));
	UAttributeSetComponentModel* AttributeSetComponentModel = PlayerUnit->GetAttributeComponentModel();
	checkf(AttributeSetComponentModel != nullptr, TEXT("어빌리티 시스템 컴포넌트 nullptr"));
	USkillComponentModel* SkillComponentModel = PlayerUnit->GetSkillComponentModel();
	checkf(SkillComponentModel != nullptr, TEXT("플레이어 스킬 컴포넌트 nullptr"));

	ClearUnit();

	// 플레이어 데이터 세팅
	{
		mPlayerUnitId = PlayerUnit->GetStaticSpawnDataId();
		mPlayerLevel = PlayerUnit->GetPlayerLevel();
	}

	// 플레이어 속성 세팅
	{
		mMaxHP = AttributeSetComponentModel->GetAttributeCurrentValue(UPlayerUnitAttributeSet::GetMaxHPAttribute());
		mHP = AttributeSetComponentModel->GetAttributeCurrentValue(UPlayerUnitAttributeSet::GetHPAttribute());
		mExp = AttributeSetComponentModel->GetAttributeCurrentValue(UPlayerUnitAttributeSet::GetExpAttribute());
	}

	// 스킬 값 세팅
	{
		constexpr int32 PlayerSkillSlot = 6;
		mSkillIds.Init(FPrimaryAssetId(), PlayerSkillSlot);

		const TArray<FSkillEntry>& SkillEntries = SkillComponentModel->GetSkills();
		const int32 SkillEntryCount = SkillEntries.Num();
		for (int32 i = 0; i < SkillEntryCount; ++i)
		{
			const FSkillEntry& SkillEntry = SkillEntries[i];
			if (SkillEntry.IsValid() == true)
			{
				mSkillIds[i] = SkillEntry.mData->GetPrimaryAssetId();
			}
		}
	}
}

void UPlayerUnitPersistData::ClearUnit()
{
	mPlayerUnitId = FPrimaryAssetId();
	mPlayerLevel = 1;

	mTagCountMap.Empty();
	mSkillIds.Empty();
}

void UPlayerUnitPersistData::RegisterPlayerUnit(UPlayerUnitModel* PlayerUnit)
{
	checkf(PlayerUnit != nullptr, TEXT("플레이어 유닛 nullptr"));
	
	SyncPlayerPersistData(PlayerUnit);
	BindPlayerUnitEvent(PlayerUnit);
}

void UPlayerUnitPersistData::UnregisterPlayerUnit(UPlayerUnitModel* PlayerUnit)
{
	checkf(PlayerUnit != nullptr, TEXT("플레이어 유닛 nullptr"));

	UnbindPlayerUnitEvent(PlayerUnit);
}

const FPrimaryAssetId& UPlayerUnitPersistData::GetPlayerUnitId() const
{
	return mPlayerUnitId;
}

int32 UPlayerUnitPersistData::GetPlayerLevel() const
{
	return mPlayerLevel;
}

const TArray<FPrimaryAssetId>& UPlayerUnitPersistData::GetSkillIds() const
{
	return mSkillIds;
}

void UPlayerUnitPersistData::SyncPlayerPersistData(UPlayerUnitModel* PlayerUnit)
{
	checkf(PlayerUnit != nullptr, TEXT("플레이어 유닛 nullptr"));
	UAttributeSetComponentModel* AttributeSetComponentModel = PlayerUnit->GetAttributeComponentModel();
	checkf(AttributeSetComponentModel != nullptr, TEXT("어빌리티 시스템 컴포넌트 nullptr"));
	USkillComponentModel* SkillComponentModel = PlayerUnit->GetSkillComponentModel();
	checkf(SkillComponentModel != nullptr, TEXT("플레이어 스킬 컴포넌트 nullptr"));

	// 플레이어 기본 데이터 동기화
	PlayerUnit->SetPlayerLevel(mPlayerLevel);
	AttributeSetComponentModel->ApplyModToAttribute(UPlayerUnitAttributeSet::GetMaxHPAttribute(), ETacticalModOp::Override, mMaxHP);
	AttributeSetComponentModel->ApplyModToAttribute(UPlayerUnitAttributeSet::GetHPAttribute(), ETacticalModOp::Override, mHP);
	AttributeSetComponentModel->ApplyModToAttribute(UPlayerUnitAttributeSet::GetExpAttribute(), ETacticalModOp::Override, mExp);

	// 플레이어 태그 동기화
	for (auto& Pair : mTagCountMap)
	{
		AttributeSetComponentModel->AddLooseGameplayTag(Pair.Key, Pair.Value);
	}

	// 플레이어 스킬 동기화
	SkillComponentModel->SetSkillFrom(GetSkillIds());
}

void UPlayerUnitPersistData::BindPlayerUnitEvent(UPlayerUnitModel* PlayerUnit)
{
	 checkf(PlayerUnit != nullptr, TEXT("플레이어 유닛 nullptr"));
	 UAttributeSetComponentModel* AttributeSetComponentModel = PlayerUnit->GetAttributeComponentModel();
	 checkf(AttributeSetComponentModel != nullptr, TEXT("어빌리티 시스템 컴포넌트 nullptr"));
	 USkillComponentModel* SkillComponentModel = PlayerUnit->GetSkillComponentModel();
	 checkf(SkillComponentModel != nullptr, TEXT("플레이어 스킬 컴포넌트 nullptr"));

	 /* 죽음으로 인한 파티 해제 처리 */

	 AttributeSetComponentModel->RegisterTacticalTagEvent(EffectTags::GameplayEffect_ActorState_Dead, ETacticalTagEventType::NewOrRemoved).AddWeakLambda(
		 PlayerUnit, [PlayerUnit](const FGameplayTag Tag, int32 Count) {
			 if (Count <= 0)
			 {
				 return;
			 }

			 UPartyModel* OwnerParty = PlayerUnit->GetOwnerParty();
			 if (OwnerParty != nullptr)
			 {
				 OwnerParty->RemovePlayerUnitModel(PlayerUnit);
			 }
		 }
	 );

	 // 레벨 추적
	 PlayerUnit->OnChangePlayerLevel.AddWeakLambda(this, [this](UPlayerUnitModel* Model, int32 PlayerLevel) {
		 mPlayerLevel = PlayerLevel;
		 });

	 // 스텟 추적
	 AttributeSetComponentModel->GetTacticalAttributeValueChangeDelegate(UPlayerUnitAttributeSet::GetMaxHPAttribute()).AddWeakLambda(this, [this](const FTacticalAttributeChangeData& Data) {
	 	mMaxHP = Data.mNewValue;
	 	});
	 AttributeSetComponentModel->GetTacticalAttributeValueChangeDelegate(UPlayerUnitAttributeSet::GetHPAttribute()).AddWeakLambda(this, [this](const FTacticalAttributeChangeData& Data) {
	 	mHP = Data.mNewValue;
	 	});
	 AttributeSetComponentModel->GetTacticalAttributeValueChangeDelegate(UPlayerUnitAttributeSet::GetExpAttribute()).AddWeakLambda(this, [this](const FTacticalAttributeChangeData& Data) {
	 	mExp = Data.mNewValue;
	 	});

	 // 패시브 스택 비용 태그의 개수 변화를 추적
	 AttributeSetComponentModel->RegisterTacticalTagEvent(EffectTags::GameplayEffect_Cost_PassiveStack, ETacticalTagEventType::AnyCountChange).AddWeakLambda(this, [this](const FGameplayTag Tag, int32 Count) {
	 	mTagCountMap[Tag] = Count;
	 	if (Count == 0)
	 	{
	 		mTagCountMap.Remove(Tag);
	 	}
	 	});

	 // 플레이어 스킬 추적
	 SkillComponentModel->OnChangeSkillUI.AddWeakLambda(this, [this](int32 SkillIndex, const UStaticSkillData* PreSkillData, const UStaticSkillData* NewSkillData)
		 {
			 // 음수 인덱스 방어
			 if (SkillIndex < 0)
			 {
				 return;
			 }

			 // 슬롯이 부족하면 확장 후 기록
			 if (SkillIndex >= mSkillIds.Num())
			 {
				 mSkillIds.SetNum(SkillIndex + 1);
			 }

			 FPrimaryAssetId NextSkillId;
			 if (NewSkillData != nullptr)
			 {
				 NextSkillId = NewSkillData->GetPrimaryAssetId();
			 }
			 mSkillIds[SkillIndex] = NextSkillId;
		 });
}

void UPlayerUnitPersistData::UnbindPlayerUnitEvent(UPlayerUnitModel* PlayerUnit)
{
	checkf(PlayerUnit != nullptr, TEXT("플레이어 유닛 nullptr"));
	UAttributeSetComponentModel* AttributeSetComponentModel = PlayerUnit->GetAttributeComponentModel();
	checkf(AttributeSetComponentModel != nullptr, TEXT("어빌리티 시스템 컴포넌트 nullptr"));
	USkillComponentModel* SkillComponentModel = PlayerUnit->GetSkillComponentModel();
	checkf(SkillComponentModel != nullptr, TEXT("플레이어 스킬 컴포넌트 nullptr"));

	PlayerUnit->OnChangePlayerLevel.RemoveAll(this);
	AttributeSetComponentModel->GetTacticalAttributeValueChangeDelegate(UPlayerUnitAttributeSet::GetMaxHPAttribute()).RemoveAll(this);
	AttributeSetComponentModel->GetTacticalAttributeValueChangeDelegate(UPlayerUnitAttributeSet::GetHPAttribute()).RemoveAll(this);
	AttributeSetComponentModel->GetTacticalAttributeValueChangeDelegate(UPlayerUnitAttributeSet::GetExpAttribute()).RemoveAll(this);
	AttributeSetComponentModel->RegisterTacticalTagEvent(EffectTags::GameplayEffect_Cost_PassiveStack, ETacticalTagEventType::AnyCountChange).RemoveAll(this);
	SkillComponentModel->OnChangeSkillUI.RemoveAll(this);
}

void UPlayerUnitPersistData::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);
}

UPartyPersistData::UPartyPersistData()
{
	mPartyPlayers.Init(nullptr, 3);

	mPartyPlayers[0] = CreateDefaultSubobject<UPlayerUnitPersistData>(TEXT("PartyPlayer00"));
	mPartyPlayers[1] = CreateDefaultSubobject<UPlayerUnitPersistData>(TEXT("PartyPlayer01"));
	mPartyPlayers[2] = CreateDefaultSubobject<UPlayerUnitPersistData>(TEXT("PartyPlayer02"));
}

void UPartyPersistData::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	if (Ar.IsSaveGame() == true)
	{
		/* 파티 멤버 유닛 내부 데이터 직렬화 */
		const int32 PlayerMaxNum = mPartyPlayers.Num();
		for (int32 PlayerIndex = 0; PlayerIndex < PlayerMaxNum; ++PlayerIndex)
		{
			mPartyPlayers[PlayerIndex]->Serialize(Ar);
		}
	}
}

void UPartyPersistData::RegisterParty(UPartyModel* Party, TArray<TObjectPtr<UPlayerUnitModel>>& Players)
{
	checkf(Party != nullptr, TEXT("파티 nullptr"));
	checkf(Players.Num() == mPartyPlayers.Num(), TEXT("파티 멤버 부족"));

	SyncPartyPersistData(Party, Players);
	BindPartyEvent(Party, Players);

	const int32 PlayerMaxNum = mPartyPlayers.Num();
	for (int32 PlayerIndex = 0; PlayerIndex < PlayerMaxNum; ++PlayerIndex)
	{
		if (Players[PlayerIndex] == nullptr)
		{
			continue;
		}

		mPartyPlayers[PlayerIndex]->RegisterPlayerUnit(Players[PlayerIndex]);
	}
}

TArray<FPrimaryAssetId> UPartyPersistData::GetPlayerUnitIds() const
{
	TArray<FPrimaryAssetId> PlayerUnitIds;
	for (const TObjectPtr<UPlayerUnitPersistData>& PartyPlayer : mPartyPlayers)
	{
		PlayerUnitIds.Add(PartyPlayer->GetPlayerUnitId());
	}
	return PlayerUnitIds;
}

int32 UPartyPersistData::GetDifficulty() const
{
	return mDifficulty;
}

const TArray<FPrimaryAssetId>& UPartyPersistData::GetArtifactIds() const
{
	return mArtifactIds;
}

void UPartyPersistData::SyncPartyPersistData(UPartyModel* Party, TArray<TObjectPtr<UPlayerUnitModel>>& Players)
{
	checkf(Party != nullptr, TEXT("플레이어 유닛 nullptr"));
	UAttributeSetComponentModel* AttributeSetComponentModel = Party->GetAttributeComponentModel();
	checkf(AttributeSetComponentModel != nullptr, TEXT("어빌리티 시스템 컴포넌트 nullptr"));
	UPartyArtifactComponentModel* PartyArtifactComponentModel = Party->GetPartyArtifactComponentModel();
	checkf(PartyArtifactComponentModel != nullptr, TEXT("파티 아티팩트 컴포넌트 nullptr"));

	// 플레이어 등록
	const int32 PlayerMaxNum = mPartyPlayers.Num();
	for (int32 PlayerIndex = 0; PlayerIndex < PlayerMaxNum; ++PlayerIndex)
	{
		if (Players[PlayerIndex] == nullptr)
		{
			continue;
		}

		Party->SetPlayerUnitModel(PlayerIndex, Players[PlayerIndex]);
	}

	// 플레이어 기본 데이터 동기화
	Party->SetDifficulty(mDifficulty);
	AttributeSetComponentModel->ApplyModToAttribute(UPartyAttributeSet::GetMoneyAttribute(), ETacticalModOp::Override, mMoney);

	// 아티팩트 동기화
	PartyArtifactComponentModel->RestoreFrom(GetArtifactIds());
}

void UPartyPersistData::BindPartyEvent(UPartyModel* Party, TArray<TObjectPtr<UPlayerUnitModel>>& Players)
{
	checkf(Party != nullptr, TEXT("파티 nullptr"));
	UAttributeSetComponentModel* AttributeSetComponentModel = Party->GetAttributeComponentModel();
	checkf(AttributeSetComponentModel != nullptr, TEXT("어빌리티 시스템 컴포넌트 nullptr"));
	UPartyArtifactComponentModel* PartyArtifactComponentModel = Party->GetPartyArtifactComponentModel();
	checkf(PartyArtifactComponentModel != nullptr, TEXT("파티 아티팩트 컴포넌트 nullptr"));

	// 플레이어 추적
	Party->OnChangePartyPlayer.AddLambda([this](int32 PlayerIndex, UPlayerUnitModel* PreModel, UPlayerUnitModel* NextModel) {
		if (PreModel != nullptr)
		{
			mPartyPlayers[PlayerIndex]->UnregisterPlayerUnit(PreModel);
			mPartyPlayers[PlayerIndex]->ClearUnit();
		}
		if (NextModel != nullptr)
		{
			mPartyPlayers[PlayerIndex]->MakeUnit(NextModel);
			mPartyPlayers[PlayerIndex]->RegisterPlayerUnit(NextModel);
		}
		});

	// 스텟 추적
	AttributeSetComponentModel->GetTacticalAttributeValueChangeDelegate(UPartyAttributeSet::GetMoneyAttribute()).AddLambda([this](const FTacticalAttributeChangeData& Data) {
		mMoney = Data.mNewValue;
		});

	// 아티팩트 추적
	PartyArtifactComponentModel->OnChangeArtifact.AddLambda([this](const TArray<TObjectPtr<UStaticArtifactData>>& PartyArtifacts)
		{
			mArtifactIds.Empty(PartyArtifacts.Num());
			for (const TObjectPtr<UStaticArtifactData>& PartyArtifact : PartyArtifacts)
			{
				mArtifactIds.Add(PartyArtifact->GetPrimaryAssetId());
			}
		});
}

void URunPersistData::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	if (Ar.IsSaveGame() == true)
	{
		if (Ar.IsSaving() == true)
		{
			int32 CurrnetStageSeed = mStageBuildStream.GetCurrentSeed();
			Ar << CurrnetStageSeed;

			int32 CurrnetEventSeed = mEventStream.GetCurrentSeed();
			Ar << CurrnetEventSeed;
		}
		else if (Ar.IsLoading() == true)
		{
			int32 CurrnetStageSeed = INDEX_NONE;
			Ar << OUT CurrnetStageSeed;
			mStageBuildStream.Initialize(CurrnetStageSeed);

			int32 CurrnetEventSeed = INDEX_NONE;
			Ar << OUT CurrnetEventSeed;
			mEventStream.Initialize(CurrnetEventSeed);
		}
	}
}

void URunPersistData::MakeCaches()
{
	const UGamePlaySettings* GamePlaySettings = GetDefault<UGamePlaySettings>();

	const auto& EffectVFXMapping = GamePlaySettings->mGlobalStatusEffectVFXSetting.mEffectVFXs;
	mRunPersistDataCache.mEffectVFXs.Reserve(EffectVFXMapping.Num());
	for (auto& EffectVFXPair : EffectVFXMapping)
	{
		for (const FSoftNiagaraSpawnData& NiagaraSpawnData : EffectVFXPair.Value.mNiagaraSpawnDatas)
		{
			mRunPersistDataCache.mEffectVFXs.Add(NiagaraSpawnData.mNiagaraSystem.LoadSynchronous());
		}
	}
	for (const FSoftNiagaraSpawnData& NiagaraSpawnData : GamePlaySettings->mCombatTargetRemoveVFX.mNiagaraSpawnDatas)
	{
		mRunPersistDataCache.mEffectVFXs.Add(NiagaraSpawnData.mNiagaraSystem.LoadSynchronous());
	}

	for (const FCombatTargetVFXTimelineSetting& TimelineSetting : GamePlaySettings->mCombatTargetVFXTimelineSettings)
	{
		mRunPersistDataCache.mEffectCurves.Add(TimelineSetting.mTimelineCurve.LoadSynchronous());
	}
}

void URunPersistData::StartRun(const TArray<FPrimaryAssetId>& PlayerUnitIds, int32 Difficulty)
{
	checkf(PlayerUnitIds.Num() == mPartyPlayers.Num(), TEXT("파티 멤버 부족"));

	ClearRun();

	// 파티 멤버들 생성
	const int32 PlayerMaxNum = mPartyPlayers.Num();
	for (int32 PlayerIndex = 0; PlayerIndex < PlayerMaxNum; ++PlayerIndex)
	{
		if (PlayerUnitIds[PlayerIndex].IsValid() == false)
		{
			continue;
		}
		mPartyPlayers[PlayerIndex]->MakeUnit(PlayerUnitIds[PlayerIndex]);
	}

	// 시드 초기 세팅
	{
#if !UE_BUILD_SHIPPING
		const int32 StageBuildStream = GFixedStageBuildSeedForDebugging != INDEX_NONE ? GFixedStageBuildSeedForDebugging : FMath::Rand32();
		const int32 EventStream = GFixedEventSeedForDebugging != INDEX_NONE ? GFixedEventSeedForDebugging : FMath::Rand32();
#else
		const int32 StageBuildStream = FMath::Rand32();
		const int32 EventStream = FMath::Rand32();
#endif
		mStageBuildStream.Initialize(StageBuildStream);
		mEventStream.Initialize(EventStream);
	}

	// 파티 기본 데이터 세팅
	{
		mDifficulty = Difficulty;
	}

	// 파티 기본 속성 세팅
	{
		UTacticalFrameworkModel* TacticalFrameworkModel = GetWorldSubsystemModel<UTacticalFrameworkModel>(GetWorld());
		checkf(TacticalFrameworkModel != nullptr, TEXT("전략 프레임워크 모델 nullptr"));

		mMoney = TacticalFrameworkModel->GetAttributeSetInitter()->GetAttributeSetValue(
			UPartyAttributeSet::StaticClass(),
			UPartyAttributeSet::GetMoneyAttribute().GetUProperty(),
			UPartyAttributeSet::KEY_NAME,
			mDifficulty
		);
	}
}

void URunPersistData::ClearRun()
{
	mDifficulty = 1;
	mMoney = 0;

	mArtifactIds.Empty();
	mRewardSkillIds.Empty();
	mRewardEquipmentIds.Empty();
	mStage.Reset();

	mRunLog.Clear();

	for (const TObjectPtr<UPlayerUnitPersistData>& PartyPlayer : mPartyPlayers)
	{
		PartyPlayer->ClearUnit();
	}
}

bool URunPersistData::AddRewardSkill(const FPrimaryAssetId& SkillId)
{
	if (SkillId.IsValid() == false)
	{
		return false;
	}

	mRewardSkillIds.Add(SkillId);
	++mRunLog.mAcquiredSkills.FindOrAdd(SkillId);
	return true;
}

bool URunPersistData::AddRewardEquipment(const FPrimaryAssetId& EquipmentId)
{
	if (EquipmentId.IsValid() == false)
	{
		return false;
	}

	mRewardEquipmentIds.Add(EquipmentId);
	++mRunLog.mAcquiredEquipment.FindOrAdd(EquipmentId);
	return true;
}

void URunPersistData::MakeStageAsync(EStageLevelType Type, FOnCreateStage OnCreateStage)
{
	UAssetManager* AssetManager = UAssetManager::GetIfInitialized();
	checkf(AssetManager != nullptr, TEXT("에셋 매니저 nullptr"));

	const UGameBalanceSettings* GameBalanceSetting = GetDefault<UGameBalanceSettings>();
	checkf(GameBalanceSetting != nullptr, TEXT("밸런스 세팅 객체 nullptr"));

	GameBalanceSetting->mStageBuildSettingTable.LoadAsync(FLoadSoftObjectPathAsyncDelegate::CreateLambda([this, Type, OnCreateStage = MoveTemp(OnCreateStage), GameBalanceSetting](const FSoftObjectPath& Path, UObject* Object) {
		
		const UDataTable* BalanceSetting = Cast<UDataTable>(Object);
		checkf(BalanceSetting != nullptr, TEXT("스테이지 빌드 세팅 미존재 nullptr"));

		const FStageBuilderParams& BuilderParams = *BalanceSetting->FindRow<FStageBuilderParams>(*EnumToString(Type), TEXT("밸런스 세팅 테이블 탐색 에러"));
		const FRandomStream& BuildStream = URandomStreamFunctionLibrary::GetStageBuildStream(this);
		const FLevelAttributeCache LevelAttributeCache = ULevelAttributeSet::MakeCache(this);

		mStage.InitializeAs<FStage>(
			FStageBuilder::Make(BuildStream, GameBalanceSetting->mGlobalStageBuildSetting, LevelAttributeCache).SetParams(BuilderParams).Build()
		);
		OnCreateStage.ExecuteIfBound(mStage.Get());

		}));
}

void URunPersistData::SetCurrentRoomIndex(int32 RowIndex, int32 ColumnIndex)
{
	mStage.GetMutable().SetCurrentRoom(RowIndex, ColumnIndex);
}

void URunPersistData::SetRoomClearData(const FRoomClearData& ClearData)
{
	mStage.GetMutable().SetRoomClearData(ClearData);
}

void URunPersistData::CollectAssetIds(int32 RowIndex, int32 ColumnIndex, OUT TArray<FPrimaryAssetId>& PlayerIds, OUT FPrimaryAssetId& StageId, OUT FPrimaryAssetId& RoomId, OUT TArray<FPrimaryAssetId>& AdditionalAssetIds) const
{
	for (const TObjectPtr<UPlayerUnitPersistData>& PartyPlayer : mPartyPlayers)
	{
		PlayerIds.Add(PartyPlayer->GetPlayerUnitId());
		PlayerIds.Append(PartyPlayer->GetSkillIds());
	}
	PlayerIds.Append(GetArtifactIds());
	PlayerIds.Append(GetRewardSkillIds());
	PlayerIds.Append(GetRewardEquipmentIds());

	StageId = GetStage().mStaticStageSpawnDataId;
	GetRoom(RowIndex, ColumnIndex).CollectAssetIds(RoomId, AdditionalAssetIds);
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
	return mStage.Get();
}

const FRoom& URunPersistData::GetRoom(int32 RowIndex, int32 ColumnIndex) const
{
	return mStage.Get().GetRoom(RowIndex, ColumnIndex);
}

const FRoom& URunPersistData::GetStartRoom() const
{
	return mStage.Get().GetStartRoom();
}

const FRoom& URunPersistData::GetCurrentRoom() const
{
	return mStage.Get().GetCurrentRoom();
}

void URunPersistData::GetCurrentRoomIndex(OUT int32& RowIndex, OUT int32& ColumnIndex) const
{
	RowIndex = mStage.Get().mCurRow;
	ColumnIndex = mStage.Get().mCurColumn;
}

const TArray<FPrimaryAssetId>& URunPersistData::GetRewardSkillIds() const
{
	return mRewardSkillIds;
}

const TArray<FPrimaryAssetId>& URunPersistData::GetRewardEquipmentIds() const
{
	return mRewardEquipmentIds;
}

const FRunLog& URunPersistData::GetRunLog() const
{
	return mRunLog;
}

bool URunPersistData::IsActive() const
{
	return mStage.IsValid();
}

void UUserPersistData::MakeUser(const FText& Name)
{
	ClearUser();

	mUserName = Name;
}

void UUserPersistData::ClearUser()
{
	mUserName = FText();
	mUserLog.Clear();
}

void UUserPersistData::UpdateLog(const FRunLog& RunLog)
{
	++mUserLog.mRunCount;

	for (auto& Pair : RunLog.mUseCountPerUnit)
	{
		mUserLog.mUseCountPerUnit.FindOrAdd(Pair.Key) += Pair.Value;
	}

	for (auto& UnitPair : RunLog.mKilledEnemyUnits)
	{
		mUserLog.mKnownEnemyUnitIds.Add(UnitPair.Key);
	}
	for (auto& SkillPair : RunLog.mAcquiredSkills)
	{
		mUserLog.mKnownSkillIds.Add(SkillPair.Key);
	}
	for (auto& EquipmentPair : RunLog.mAcquiredEquipment)
	{
		mUserLog.mKnownEquipmentIds.Add(EquipmentPair.Key);
	}
	for (auto& DicePair : RunLog.mAcquiredDices)
	{
		mUserLog.mKnownDiceIds.Add(DicePair.Key);
	}
}

const FText& UUserPersistData::GetUserName() const
{
	return mUserName;
}

bool UUserPersistData::IsActive() const
{
	return mUserName.IsEmpty() == false;
}

const FUserLog& UUserPersistData::GetUserLog() const
{
	return mUserLog;
}

UOptionPersistData::UOptionPersistData()
{
	mVolumes.Init(1.f, StaticCast<int32>(EGameVolumeType::Count));
	mOptionPersistDataCache.mSoundClassObjects.Init(nullptr, StaticCast<int32>(EGameVolumeType::Count));
}

void UOptionPersistData::MakeCaches()
{
	// 이미 로드되어 있기 때문에, 대기 없음
	const UAudioSettings* AudioSettings = GetDefault<UAudioSettings>();
	mOptionPersistDataCache.mSoundMixObject = Cast<USoundMix>(AudioSettings->DefaultBaseSoundMix.TryLoad());

	const UGamePlaySettings* GamePlaySettings = GetDefault<UGamePlaySettings>();
	const int32 MaxGameVolumeType = StaticCast<int32>(EGameVolumeType::Count);
	for (int32 i = 0; i < MaxGameVolumeType; ++i)
	{
		mOptionPersistDataCache.mSoundClassObjects[i] = Cast<USoundClass>(GamePlaySettings->mSoundClasses[i].ToSoftObjectPath().TryLoad());
	}

	FViewport::ViewportResizedEvent.AddUObject(this, &UOptionPersistData::OnResizeViewport);
}

void UOptionPersistData::MakeOption()
{
	ClearOption();
}

void UOptionPersistData::ClearOption()
{
	const UOptionPersistData* CDO = GetDefault<UOptionPersistData>();
	mVolumes = CDO->mVolumes;
	mLanguageType = CDO->mLanguageType;
	mOverallQuality = CDO->mOverallQuality;
	mFpsLimit = CDO->mFpsLimit;
	mCameraShakeEnabled = CDO->mCameraShakeEnabled;
	mEffectVFXEnabled = CDO->mEffectVFXEnabled;

	ApplyCurrentOptions();
}

void UOptionPersistData::SetVolume(EGameVolumeType VolumeType, float Volume)
{
	Volume = FMath::Clamp(Volume, 0.f, 1.f);

	mVolumes[StaticCast<int32>(VolumeType)] = Volume;
	if (mOptionPersistDataCache.mSoundMixObject == nullptr
		|| mOptionPersistDataCache.mSoundClassObjects.IsValidIndex(StaticCast<int32>(VolumeType)) == false
		|| mOptionPersistDataCache.mSoundClassObjects[StaticCast<int32>(VolumeType)] == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("OptionPersistData: sound option asset missing for volume type %d"), StaticCast<int32>(VolumeType));
		return;
	}
	UGameplayStatics::SetSoundMixClassOverride(
		GetWorld(),																	// 월드 컨텍스트
		mOptionPersistDataCache.mSoundMixObject,									// 적용할 SoundMix
		mOptionPersistDataCache.mSoundClassObjects[StaticCast<int32>(VolumeType)],	// 대상 SoundClass
		Volume,																		// 볼륨 (0.0 ~ 1.0)
		1.0f,																		// 피치 (기본값 1.0)
		0.5f																		// 페이드 시간 (초 단위)
	);
}

void UOptionPersistData::SetLanguage(ELanguageType LanguageType)
{
	FString LanguageStr;
	switch (LanguageType)
	{
	case ELanguageType::KOREAN:
		LanguageStr = TEXT("ko");
		break;
	case ELanguageType::ENGLISH:
		LanguageStr = TEXT("en");
		break;
	}

	if (LanguageStr.IsEmpty() == true)
	{
		return;
	}

	mLanguageType = LanguageType;
	FInternationalization::Get().SetCurrentCulture(LanguageStr);

	FTextLocalizationManager& LocalizationManager = FTextLocalizationManager::Get();
#if WITH_EDITOR
	if (GIsEditor)
	{
		LocalizationManager.EnableGameLocalizationPreview(LanguageStr);
		LocalizationManager.WaitForAsyncTasks();
		return;
	}
#endif

	LocalizationManager.RefreshResources();
	LocalizationManager.WaitForAsyncTasks();
}

void UOptionPersistData::SetOverallQuality(EOverallQualityType QualityType)
{
	UGameUserSettings* GameUserSettings = UGameUserSettings::GetGameUserSettings();
	checkf(GameUserSettings != nullptr, TEXT("게임 유저 세팅 nullptr"));

	mOverallQuality = QualityType;

	GameUserSettings->SetOverallScalabilityLevel(StaticCast<int32>(QualityType));
	GameUserSettings->ApplySettings(false);
	ApplyScreenPercentage();
}

void UOptionPersistData::SetFpsLimit(int32 FpsLimit)
{
	UGameUserSettings* GameUserSettings = UGameUserSettings::GetGameUserSettings();
	checkf(GameUserSettings != nullptr, TEXT("게임 유저 세팅 nullptr"));

	mFpsLimit = FpsLimit <= 30 ? 30 : 60;

	GameUserSettings->SetFrameRateLimit(mFpsLimit);
	GameUserSettings->ApplyNonResolutionSettings();
}

void UOptionPersistData::SetCameraShakeEnabled(bool IsEnabled)
{
	mCameraShakeEnabled = IsEnabled;
}

void UOptionPersistData::SetEffectVFXEnabled(bool IsEnabled)
{
	mEffectVFXEnabled = IsEnabled;
}

void UOptionPersistData::ApplyCurrentOptions()
{
	const int32 MaxGameVolumeType = StaticCast<int32>(EGameVolumeType::Count);
	for (int32 i = 0; i < MaxGameVolumeType; ++i)
	{
		SetVolume(StaticCast<EGameVolumeType>(i), mVolumes[i]);
	}
	SetLanguage(mLanguageType);
	SetOverallQuality(mOverallQuality);
	SetFpsLimit(mFpsLimit);
	SetCameraShakeEnabled(mCameraShakeEnabled);
	SetEffectVFXEnabled(mEffectVFXEnabled);
}

float UOptionPersistData::GetVolume(EGameVolumeType VolumeType) const
{
	return mVolumes[StaticCast<int32>(VolumeType)];
}

ELanguageType UOptionPersistData::GetLanguage() const
{
	return mLanguageType;
}

EOverallQualityType UOptionPersistData::GetOverallQuality() const
{
	return mOverallQuality;
}

int32 UOptionPersistData::GetFpsLimit() const
{
	return mFpsLimit;
}

bool UOptionPersistData::IsCameraShakeEnabled() const
{
	return mCameraShakeEnabled;
}

bool UOptionPersistData::IsEffectVFXEnabled() const
{
	return mEffectVFXEnabled;
}

bool UOptionPersistData::IsActive() const
{
	return true;
}

void UOptionPersistData::ApplyScreenPercentage() const
{
	FIntPoint BackBufferSize = FIntPoint::ZeroValue;
	if (GEngine != nullptr && GEngine->GameViewport != nullptr && GEngine->GameViewport->Viewport != nullptr)
	{
		BackBufferSize = GEngine->GameViewport->Viewport->GetSizeXY();
	}
	else
	{
		BackBufferSize = FIntPoint(GSystemResolution.ResX, GSystemResolution.ResY);
	}

	const int32 ShortSide = BackBufferSize.GetMin();
	if (ShortSide <= 0)
	{
		return;
	}

	const float Percentage = FMath::Clamp(100.f * ToRenderResolutionHeight(mOverallQuality) / StaticCast<float>(ShortSide), 10.f, 100.f);
	if (IConsoleVariable* ScreenPercentageCVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.ScreenPercentage")))
	{
		// 디바이스 프로파일보다 낮고 콘솔 입력보다 낮은 게임 설정 우선순위로 기록한다.
		ScreenPercentageCVar->Set(Percentage, ECVF_SetByGameSetting);
	}
}

void UOptionPersistData::OnResizeViewport(FViewport* Viewport, uint32 Unused)
{
	FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateWeakLambda(this, [this](float /*DeltaTime*/)
		{
			ApplyScreenPercentage();

			// 한 번만 돈다
			return false;   
		}), 0.f);
}

