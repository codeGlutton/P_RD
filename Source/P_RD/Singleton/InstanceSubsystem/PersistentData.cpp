#include "Singleton/InstanceSubsystem/PersistentData.h"
#include "AttributeSet/UnitAttributeSet.h"
#include "Component/AttributeComponent/AttributeSetComponentModel.h"
#include "Pawn/Player/PlayerUnitModel.h"

#include "Setting/GameBalanceSettings.h"
#include "Engine/AssetManager.h"
#include "PCGStage/StageBuilder.h"

#include "DataAsset/UnitSpawnData/StaticPlayerUnitSpawnData.h"
#include "DataAsset/DiceData/StaticDiceData.h"

#include "FunctionLibrary/RandomStreamFunctionLibrary.h"

#include "Setting/GamePlaySettings.h"
#include "Sound/SoundClass.h"

void FRunLog::Clear()
{
	mKilledEnemyUnits.Empty();
	mAcquiredSkills.Empty();
	mAcquiredEquipment.Empty();
	mAcquiredDices.Empty();
}

void FUserLog::Clear()
{
	mRunCount = 0;
	mRunCountPerUnit.Empty();
	mKnownEnemyUnitIds.Empty();
	mKnownSkillIds.Empty();
	mKnownEquipmentIds.Empty();
	mKnownDiceIds.Empty();
}

void UPlayerUnitPersistData::RegisterPlayerUnit(UPlayerUnitModel* PlayerUnit)
{
	checkf(PlayerUnit != nullptr, TEXT("플레이어 유닛 nullptr"));
	UAttributeSetComponentModel* ASCModel = PlayerUnit->GetAttributeComponentModel();
	checkf(ASCModel != nullptr, TEXT("어빌리티 시스템 컴포넌트 nullptr"));
	
	SyncPlayerPersistData(PlayerUnit);
	BindPlayerUnitEvent(PlayerUnit);
}

const FPrimaryAssetId& UPlayerUnitPersistData::GetPlayerUnitId() const
{
	return mPlayerUnitId;
}

int32 UPlayerUnitPersistData::GetPlayerLevel() const
{
	return mPlayerLevel;
}

int32 UPlayerUnitPersistData::GetDifficulty() const
{
	return mDifficulty;
}

const TArray<FPrimaryAssetId>& UPlayerUnitPersistData::GetSkillIds() const
{
	return mSkillIds;
}

const TArray<FPrimaryAssetId>& UPlayerUnitPersistData::GetEquipmentIds() const
{
	return mEquipmentIds;
}

const TArray<FPrimaryAssetId>& UPlayerUnitPersistData::GetDiceIds() const
{
	return mDiceIds;
}

void UPlayerUnitPersistData::SyncPlayerPersistData(UPlayerUnitModel* PlayerUnit)
{
	 checkf(PlayerUnit != nullptr, TEXT("플레이어 유닛 nullptr"));
	 UAttributeSetComponentModel* ASCModel = PlayerUnit->GetAttributeComponentModel();
	 checkf(ASCModel != nullptr, TEXT("어빌리티 시스템 컴포넌트 nullptr"));
	 
	 if (mIsNewData == true)
	 {
	 	mIsNewData = false;
	 
	 	bool IsFound;
	 
	 	IsFound = false;
	 	mMaxHP = ASCModel->GetAttributeCurrentValue(UPlayerUnitAttributeSet::GetMaxHPAttribute(), IsFound);
	 	checkf(IsFound == true, TEXT("최대 체력 속성 미발견"));
	 
	 	IsFound = false;
	 	mHP = ASCModel->GetAttributeCurrentValue(UPlayerUnitAttributeSet::GetHPAttribute(), IsFound);
	 	checkf(IsFound == true, TEXT("체력 속성 미발견"));
	 
	 	IsFound = false;
	 	mExp = ASCModel->GetAttributeCurrentValue(UPlayerUnitAttributeSet::GetExpAttribute(), IsFound);
	 	checkf(IsFound == true, TEXT("경험치 속성 미발견"));
	 
	 	IsFound = false;
	 	mMoney = ASCModel->GetAttributeCurrentValue(UPlayerUnitAttributeSet::GetMoneyAttribute(), IsFound);
	 	checkf(IsFound == true, TEXT("돈 속성 미발견"));
	 
	 	return;
	 }
	 
	 ASCModel->ApplyModToAttribute(UPlayerUnitAttributeSet::GetMaxHPAttribute(), EGameplayModOp::Override, mMaxHP);
	 ASCModel->ApplyModToAttribute(UPlayerUnitAttributeSet::GetHPAttribute(), EGameplayModOp::Override, mHP);
	 ASCModel->ApplyModToAttribute(UPlayerUnitAttributeSet::GetExpAttribute(), EGameplayModOp::Override, mExp);
	 ASCModel->ApplyModToAttribute(UPlayerUnitAttributeSet::GetMoneyAttribute(), EGameplayModOp::Override, mMoney);
	 for (auto& Pair : mTagCountMap)
	 {
		 ASCModel->AddLooseGameplayTag(Pair.Key, Pair.Value);
	 }
}

void UPlayerUnitPersistData::BindPlayerUnitEvent(UPlayerUnitModel* PlayerUnit)
{
	 checkf(PlayerUnit != nullptr, TEXT("플레이어 유닛 nullptr"));
	 UAttributeSetComponentModel* ASCModel = PlayerUnit->GetAttributeComponentModel();
	 checkf(ASCModel != nullptr, TEXT("어빌리티 시스템 컴포넌트 nullptr"));
	 
	 ASCModel->GetTacticalAttributeValueChangeDelegate(UPlayerUnitAttributeSet::GetMaxHPAttribute()).AddLambda([this](const FTacticalAttributeChangeData& Data) {
	 	mMaxHP = Data.mNewValue;
	 	});
	 ASCModel->GetTacticalAttributeValueChangeDelegate(UPlayerUnitAttributeSet::GetHPAttribute()).AddLambda([this](const FTacticalAttributeChangeData& Data) {
	 	mHP = Data.mNewValue;
	 	});
	 ASCModel->GetTacticalAttributeValueChangeDelegate(UPlayerUnitAttributeSet::GetExpAttribute()).AddLambda([this](const FTacticalAttributeChangeData& Data) {
	 	mExp = Data.mNewValue;
	 	});
	 ASCModel->GetTacticalAttributeValueChangeDelegate(UPlayerUnitAttributeSet::GetMoneyAttribute()).AddLambda([this](const FTacticalAttributeChangeData& Data) {
	 	mMoney = Data.mNewValue;
	 	});
	 ASCModel->RegisterTacticalTagEvent(EffectTags::GameplayEffect_Cost_PassiveStack, EGameplayTagEventType::AnyCountChange).AddLambda([this](const FGameplayTag Tag, int32 Count) {
	 	mTagCountMap[Tag] = Count;
	 	if (Count == 0)
	 	{
	 		mTagCountMap.Remove(Tag);
	 	}
	 	});
}

void URunPersistData::StartRun(const FPrimaryAssetId& PlayerUnitId, int32 Difficulty)
{
	ClearRun();

	mStageBuildStream.Initialize(FMath::Rand32());
	mEventStream.Initialize(FMath::Rand32());

	mIsNewData = true;
	mPlayerUnitId = PlayerUnitId;
	mDifficulty = Difficulty;

	// 선택한 캐릭터의 고정 주사위(mDiceDatas)를 런 다이스 목록(mDiceIds)으로 펼친다.
	// 이게 비면 전투에서 DicePool이 비어 HUD 주사위 수가 0이 된다.
	if (UAssetManager* AssetManager = UAssetManager::GetIfInitialized())
	{
		if (const UStaticPlayerUnitSpawnData* PlayerData = AssetManager->GetPrimaryAssetObject<UStaticPlayerUnitSpawnData>(PlayerUnitId))
		{
			for (const TSoftObjectPtr<UStaticDiceData>& DiceSoft : PlayerData->mDiceDatas)
			{
				if (const UStaticDiceData* DiceData = DiceSoft.LoadSynchronous())
				{
					mDiceIds.Add(DiceData->GetPrimaryAssetId());
				}
			}
		}
	}
}

void URunPersistData::ClearRun()
{
	mPlayerLevel = 1;
	mDifficulty = 1;
	mIsNewData = true;

	mTagCountMap.Empty();
	mSkillIds.Empty();
	mEquipmentIds.Empty();
	mDiceIds.Empty();

	mStage.Reset();

	mRunLog.Clear();
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

		mStage.InitializeAs<FStage>(FStageBuilder::Make(BuildStream, GameBalanceSetting->mGlobalStageBuildSetting, BuilderParams).Build());
		OnCreateStage.ExecuteIfBound(mStage.Get());

		}));
}

void URunPersistData::SetCurrentRoomIndex(int32 RowIndex, int32 ColumnIndex)
{
	mStage.GetMutable().SetCurrentRoom(RowIndex, ColumnIndex);
}

void URunPersistData::CollectAssetIds(int32 RowIndex, int32 ColumnIndex, OUT TArray<FPrimaryAssetId>& PlayerIds, OUT FPrimaryAssetId& StageId, OUT FPrimaryAssetId& RoomId, OUT TArray<FPrimaryAssetId>& AdditionalAssetIds) const
{
	PlayerIds.Add(mPlayerUnitId);
	PlayerIds.Append(mSkillIds);
	PlayerIds.Append(mEquipmentIds);
	PlayerIds.Append(mDiceIds);

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

void UUserPersistData::UpdateLog(const FPrimaryAssetId& PlayerUnitId, const FRunLog& RunLog)
{
	++mUserLog.mRunCount;
	++mUserLog.mRunCountPerUnit[PlayerUnitId];

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
}

void UOptionPersistData::MakeOption()
{
}

void UOptionPersistData::ClearOption()
{
	const UOptionPersistData* CDO = GetDefault<UOptionPersistData>();
	mVolumes = CDO->mVolumes;
	mLanguageType = CDO->mLanguageType;
	mResolution = CDO->mResolution;

	ApplyCurrentOptions();
}

void UOptionPersistData::SetVolume(EGameVolumeType VolumeType, float Volume)
{
	Volume = FMath::Clamp(Volume, 0.f, 1.f);

	mVolumes[StaticCast<int32>(VolumeType)] = Volume;
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
}

void UOptionPersistData::SetResolution(const FIntPoint& Resolution)
{
	// TODO
}

void UOptionPersistData::ApplyCurrentOptions()
{
	const int32 MaxGameVolumeType = StaticCast<int32>(EGameVolumeType::Count);
	for (int32 i = 0; i < MaxGameVolumeType; ++i)
	{
		SetVolume(StaticCast<EGameVolumeType>(i), mVolumes[i]);
	}
	SetLanguage(mLanguageType);
	SetResolution(mResolution);
}

float UOptionPersistData::GetVolume(EGameVolumeType VolumeType) const
{
	return mVolumes[StaticCast<int32>(VolumeType)];
}

ELanguageType UOptionPersistData::GetLanguage() const
{
	return mLanguageType;
}

const FIntPoint& UOptionPersistData::GetResolution() const
{
	return mResolution;
}

bool UOptionPersistData::IsActive() const
{
	return true;
}