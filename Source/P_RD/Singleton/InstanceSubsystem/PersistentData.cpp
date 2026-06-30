#include "Singleton/InstanceSubsystem/PersistentData.h"
#include "AttributeSet/UnitAttributeSet.h"

#include "Dice/DicePoolModel.h"
#include "Component/AttributeComponent/AttributeSetComponentModel.h"
#include "Pawn/Player/PlayerUnitModel.h"

#include "Setting/GameBalanceSettings.h"
#include "Engine/AssetManager.h"
#include "PCGStage/StageBuilder.h"

#include "DataAsset/UnitSpawnData/StaticPlayerUnitSpawnData.h"
#include "DataAsset/SkillData/StaticSkillData.h"
#include "DataAsset/EquipmentData/StaticEquipmentData.h"
#include "DataAsset/DiceData/StaticDiceData.h"

#include "FunctionLibrary/RandomStreamFunctionLibrary.h"

#include "Setting/GamePlaySettings.h"
#include "Sound/SoundClass.h"

/**
 * @file PersistentData.cpp
 * @brief 런(Run)/유저/플레이어 유닛/옵션의 영속 데이터 객체 구현부.
 *        한 판(런) 진행 중에 누적/유지되어야 하는 상태(플레이어 스탯, 스킬/장비/주사위 보유 목록,
 *        스테이지 진행 상황, 사운드 볼륨/언어/해상도 옵션 등)를 보관하고, 유닛 모델과 동기화한다.
 *
 *        [PR #191 마이그레이션 근거]
 *        본 PR은 효과 모디파이어의 "연산 종류" enum을 GAS의 EGameplayModOp 에서
 *        자체 정의 ETacticalModOp(TacticalEffectType.h) 로 치환한다(GAS 폐기 작업의 일부).
 *        이 파일에서는 SyncPlayerPersistData()가 저장된 스탯을 유닛 속성에 되돌릴 때
 *        구 EGameplayModOp::Override 대신 ETacticalModOp::Override 를 사용하도록 바뀌었다.
 *        - ETacticalModOp 의 정수값은 구 EGameplayModOp 와 동일하게 유지된다. 이유:
 *          (1) 직렬화 호환  : 기존 에셋/세이브에 정수로 박힌 op 값을 그대로 보존.
 *          (2) 배열 인덱싱  : Aggregator가 mMods[ETacticalModOp::Max] 형태로 op 값을 인덱스로 사용.
 *          (3) CoreRedirect : DefaultEngine.ini가 구 enum 이름을 새 enum으로 매핑.
 *        - Override(=정수 3)는 "덮어쓰기" 연산. 누적이 아니라 최종값을 그 값으로 강제 설정한다.
 *          영속 스탯 복원은 항상 정확한 절대값을 다시 심어야 하므로 Override가 적합하다.
 */

/**
 * @brief 한 런(Run) 동안 누적된 로그(처치 적/획득 스킬/장비/주사위)를 모두 비운다.
 *        새 런 시작 시 직전 런의 기록이 섞이지 않도록 초기화하는 용도.
 */
void FRunLog::Clear()
{
	mKilledEnemyUnits.Empty();
	mAcquiredSkills.Empty();
	mAcquiredEquipment.Empty();
	mAcquiredDices.Empty();
}

/**
 * @brief 유저 단위 누적 로그(총 플레이 횟수, 유닛별 플레이 횟수, 도감 격 "이미 본" 식별자 집합)를 비운다.
 *        유저 데이터 자체를 초기화(새 유저 생성 등)할 때 호출된다.
 */
void FUserLog::Clear()
{
	mRunCount = 0;
	mRunCountPerUnit.Empty();
	mKnownEnemyUnitIds.Empty();
	mKnownSkillIds.Empty();
	mKnownEquipmentIds.Empty();
	mKnownDiceIds.Empty();
}

/**
 * @brief 스폰된 플레이어 유닛을 영속 데이터에 등록한다.
 *        영속 데이터 ↔ 유닛 속성을 양방향으로 연결한다:
 *        - SyncPlayerPersistData : (저장본 -> 유닛) 또는 (유닛 -> 저장본 최초 캡처)
 *        - BindPlayerUnitEvent   : (유닛 -> 저장본) 이후 변경 사항을 계속 추적하도록 델리게이트 바인딩
 * @param PlayerUnit 등록 대상 플레이어 유닛 모델(어트리뷰트 컴포넌트 보유 필수)
 */
void UPlayerUnitPersistData::RegisterPlayerUnit(UPlayerUnitModel* PlayerUnit)
{
	checkf(PlayerUnit != nullptr, TEXT("플레이어 유닛 nullptr"));
	UAttributeSetComponentModel* ASCModel = PlayerUnit->GetAttributeComponentModel();
	checkf(ASCModel != nullptr, TEXT("어빌리티 시스템 컴포넌트 nullptr"));
	
	SyncPlayerPersistData(PlayerUnit);
	BindPlayerUnitEvent(PlayerUnit);
}

/** @brief 등록된 플레이어 유닛의 PrimaryAssetId 를 반환한다. @return 플레이어 유닛 식별자 */
const FPrimaryAssetId& UPlayerUnitPersistData::GetPlayerUnitId() const
{
	return mPlayerUnitId;
}

/** @brief 현재 플레이어 레벨을 반환한다. @return 레벨 값 */
int32 UPlayerUnitPersistData::GetPlayerLevel() const
{
	return mPlayerLevel;
}

/** @brief 현재 런의 난이도를 반환한다. @return 난이도 값 */
int32 UPlayerUnitPersistData::GetDifficulty() const
{
	return mDifficulty;
}

/** @brief 보유 스킬 식별자 목록을 반환한다. @return 스킬 PrimaryAssetId 배열(읽기 전용) */
const TArray<FPrimaryAssetId>& UPlayerUnitPersistData::GetSkillIds() const
{
	return mSkillIds;
}

/** @brief 보유 장비 식별자 목록을 반환한다. @return 장비 PrimaryAssetId 배열(읽기 전용) */
const TArray<FPrimaryAssetId>& UPlayerUnitPersistData::GetEquipmentIds() const
{
	return mEquipmentIds;
}

/** @brief 보유 주사위 식별자 목록을 반환한다. @return 주사위 PrimaryAssetId 배열(읽기 전용) */
const TArray<FPrimaryAssetId>& UPlayerUnitPersistData::GetDiceIds() const
{
	return mDiceIds;
}

/**
 * @brief 영속 스탯(최대 체력/체력/경험치/돈)을 유닛 어트리뷰트와 동기화한다.
 *        - 최초(mIsNewData==true) : 유닛의 현재 속성값을 저장본으로 "캡처"하고 곧바로 반환한다.
 *          (이때는 유닛 -> 저장본 방향이므로 모디파이어를 적용하지 않는다.)
 *        - 이후(mIsNewData==false): 저장된 값을 유닛 속성에 Override 연산으로 "복원"하고,
 *          누적된 Loose 게임플레이 태그(태그별 개수)까지 다시 심는다.
 *
 *        [PR #191] 복원 시 사용하는 ApplyModToAttribute 의 연산 종류 인자가
 *        구 EGameplayModOp::Override 에서 ETacticalModOp::Override(정수값 3, 동일하게 유지)로 치환되었다.
 *        Override 는 기존값에 더하거나 곱하지 않고 최종값을 인자 그대로 강제 설정하는 "덮어쓰기"이며,
 *        저장된 절대값을 손실 없이 그대로 되돌려야 하는 영속 복원에 정확히 부합한다.
 * @param PlayerUnit 동기화 대상 플레이어 유닛 모델
 */
void UPlayerUnitPersistData::SyncPlayerPersistData(UPlayerUnitModel* PlayerUnit)
{
	 checkf(PlayerUnit != nullptr, TEXT("플레이어 유닛 nullptr"));
	 UAttributeSetComponentModel* ASCModel = PlayerUnit->GetAttributeComponentModel();
	 checkf(ASCModel != nullptr, TEXT("어빌리티 시스템 컴포넌트 nullptr"));

	 // 최초 등록: 유닛의 현재 속성값을 저장본으로 캡처한 뒤 더 진행하지 않는다(복원 단계 건너뜀).
	 if (mIsNewData == true)
	 {
	 	mIsNewData = false;
	 }
	 
	 // 저장본 -> 유닛 복원. ETacticalModOp::Override(정수 3, 구 EGameplayModOp::Override 대체)는
	 // 누적이 아닌 "덮어쓰기"라 저장된 절대값을 그대로 다시 세팅한다.
	 // ※ ETacticalModOp 의 정수값을 구 enum과 동일하게 유지하는 이유: 직렬화 호환 / Aggregator의 op-값 배열 인덱싱 / CoreRedirect 매핑.
	 ASCModel->ApplyModToAttribute(UPlayerUnitAttributeSet::GetMaxHPAttribute(), ETacticalModOp::Override, mMaxHP);
	 ASCModel->ApplyModToAttribute(UPlayerUnitAttributeSet::GetHPAttribute(), ETacticalModOp::Override, mHP);
	 ASCModel->ApplyModToAttribute(UPlayerUnitAttributeSet::GetExpAttribute(), ETacticalModOp::Override, mExp);
	 ASCModel->ApplyModToAttribute(UPlayerUnitAttributeSet::GetMoneyAttribute(), ETacticalModOp::Override, mMoney);
	 
	 // (여러 방 내에서 유지되는 특수한 패시브 스택) Loose 게임플레이 태그를 개수(Pair.Value)만큼 다시 부여한다.
	 for (auto& Pair : mTagCountMap)
	 {
		 ASCModel->AddLooseGameplayTag(Pair.Key, Pair.Value);
	 }

	 // TODO : 플레이어 스킬 동기화
	 // TODO : 플레이어 장비 동기화

	 // 플레이어 주사위 동기화
	 if (UDicePoolModel* DicePool = PlayerUnit->GetDicePoolModel())
	 {
		 DicePool->BuildFromDiceIds(GetDiceIds());
	 }
}

/**
 * @brief 유닛 속성/태그의 변경 이벤트를 영속 데이터(mMaxHP/mHP/mExp/mMoney/mTagCountMap)에 반영하도록 델리게이트를 바인딩한다.
 *        이로써 전투/이동 중 유닛에서 발생한 스탯·태그 변화가 자동으로 저장본에 누적되어,
 *        이후 SyncPlayerPersistData 의 복원 단계에서 동일한 값을 그대로 되돌릴 수 있다.
 * @param PlayerUnit 이벤트를 구독할 대상 플레이어 유닛 모델
 */
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
	 // 패시브 스택 비용 태그의 개수 변화를 추적: 현재 개수를 맵에 저장하되, 0이 되면 항목을 제거해 저장본을 깔끔히 유지한다.
	 ASCModel->RegisterTacticalTagEvent(EffectTags::GameplayEffect_Cost_PassiveStack, EGameplayTagEventType::AnyCountChange).AddLambda([this](const FGameplayTag Tag, int32 Count) {
	 	mTagCountMap[Tag] = Count;
	 	if (Count == 0)
	 	{
	 		mTagCountMap.Remove(Tag);
	 	}
	 	});

	 // TODO: 스킬 변경 시 대리자에 바인딩. 이걸로 영구 데이터도 동기화
	 // TODO: 장비 변경 시 대리자에 바인딩. 이걸로 영구 데이터도 동기화
	 // TODO: 주사위 변경 시 대리자에 바인딩. 이걸로 영구 데이터도 동기화
}

/**
 * @brief 새 런을 시작한다. 직전 런 상태를 초기화하고, 난수 스트림을 시드하며,
 *        선택 캐릭터의 고정 주사위를 런 보유 목록으로 펼친다.
 * @param PlayerUnitId 이번 런에 사용할 플레이어 유닛의 PrimaryAssetId
 * @param Difficulty   이번 런의 난이도
 */
void URunPersistData::StartRun(const FPrimaryAssetId& PlayerUnitId, int32 Difficulty)
{
	UAssetManager* AssetManager = UAssetManager::GetIfInitialized();
	checkf(AssetManager != nullptr, TEXT("에셋 매니저 nullptr"));

	const UStaticPlayerUnitSpawnData* PlayerData = AssetManager->GetPrimaryAssetObject<UStaticPlayerUnitSpawnData>(PlayerUnitId);
	checkf(PlayerData != nullptr, TEXT("플레이어 데이터 nullptr"));

	ClearRun();

	// 시드 초기 세팅
	{
		mStageBuildStream.Initialize(FMath::Rand32());
		mEventStream.Initialize(FMath::Rand32());
	}

	// 플레이어 기본 데이터 세팅
	{
		mPlayerUnitId = PlayerUnitId;
		mDifficulty = Difficulty;
	}

	// 플레이어 기본 속성 세팅
	{
		mMaxHP = PlayerData->GetDefaultAttributeValue(GetWorld(), UPlayerUnitAttributeSet::StaticClass(), UPlayerUnitAttributeSet::GetMaxHPAttribute(), mPlayerLevel);
		mHP = mMaxHP;
		mExp = 0.f;
		mMoney = PlayerData->GetDefaultAttributeValue(GetWorld(), UPlayerUnitAttributeSet::StaticClass(), UPlayerUnitAttributeSet::GetMoneyAttribute(), mPlayerLevel);
	}

	// 스킬 기본 값 세팅
	{
		for (const TSoftObjectPtr<UStaticSkillData>& SkillSoft : PlayerData->mSkillDatas)
		{
			if (const UStaticSkillData* SkillData = SkillSoft.LoadSynchronous())
			{
				mSkillIds.Add(SkillData->GetPrimaryAssetId());
			}
		}
	}

	// 장비 기본 값 세팅
	{
		for (const TSoftObjectPtr<UStaticEquipmentData>& EquipmentSoft : PlayerData->mEquipmentDatas)
		{
			if (const UStaticEquipmentData* EquipmentData = EquipmentSoft.LoadSynchronous())
			{
				mEquipmentIds.Add(EquipmentData->GetPrimaryAssetId());
			}
		}
	}

	// 주사위 기본 값 세팅
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

/**
 * @brief 런 영속 데이터를 초기 상태로 되돌린다(레벨/난이도 리셋, 보유 목록·태그·스테이지·로그 비우기).
 *        새 런 시작 직전에 직전 런 잔여 상태를 제거하기 위한 용도.
 */
void URunPersistData::ClearRun()
{
	mIsNewData = true;
	mPlayerLevel = 1;
	mDifficulty = 1;

	mTagCountMap.Empty();
	mSkillIds.Empty();
	mEquipmentIds.Empty();
	mDiceIds.Empty();

	mStage.Reset();

	mRunLog.Clear();
}

/**
 * @brief 지정한 스테이지 레벨 타입에 맞는 스테이지를 비동기로 생성한다.
 *        밸런스 세팅 데이터테이블을 비동기 로드한 뒤, 행에서 빌더 파라미터를 찾아
 *        결정론적 빌드 스트림으로 FStage 를 생성하고 콜백으로 결과를 넘긴다.
 * @param Type          생성할 스테이지의 레벨 타입(밸런스 테이블 행 키)
 * @param OnCreateStage 생성 완료 시 호출될 델리게이트(생성된 FStage 전달)
 */
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

/**
 * @brief 현재 진행 중인 방을 스테이지 격자 좌표로 지정한다.
 * @param RowIndex    행 인덱스
 * @param ColumnIndex 열 인덱스
 */
void URunPersistData::SetCurrentRoomIndex(int32 RowIndex, int32 ColumnIndex)
{
	mStage.GetMutable().SetCurrentRoom(RowIndex, ColumnIndex);
}

/**
 * @brief 지정한 방을 로드하기 위해 필요한 모든 에셋 식별자를 수집한다.
 *        플레이어/스킬/장비/주사위 식별자와 스테이지/방 및 방 부가 에셋 식별자를 OUT 인자로 채운다.
 * @param RowIndex           대상 방의 행 인덱스
 * @param ColumnIndex        대상 방의 열 인덱스
 * @param PlayerIds          [out] 플레이어 유닛 + 보유 스킬/장비/주사위 식별자 누적 배열
 * @param StageId            [out] 스테이지 스폰 데이터 식별자
 * @param RoomId             [out] 방 스폰 데이터 식별자
 * @param AdditionalAssetIds [out] 방이 추가로 요구하는 부가 에셋 식별자 배열
 */
void URunPersistData::CollectAssetIds(int32 RowIndex, int32 ColumnIndex, OUT TArray<FPrimaryAssetId>& PlayerIds, OUT FPrimaryAssetId& StageId, OUT FPrimaryAssetId& RoomId, OUT TArray<FPrimaryAssetId>& AdditionalAssetIds) const
{
	PlayerIds.Add(mPlayerUnitId);
	PlayerIds.Append(mSkillIds);
	PlayerIds.Append(mEquipmentIds);
	PlayerIds.Append(mDiceIds);

	StageId = GetStage().mStaticStageSpawnDataId;
	GetRoom(RowIndex, ColumnIndex).CollectAssetIds(RoomId, AdditionalAssetIds);
}

/** @brief 스테이지 생성에 쓰이는 결정론적 난수 스트림을 반환한다. @return 스테이지 빌드 스트림 */
const FRandomStream& URunPersistData::GetStageBuildStream() const
{
	return mStageBuildStream;
}

/** @brief 이벤트 발생에 쓰이는 결정론적 난수 스트림을 반환한다. @return 이벤트 스트림 */
const FRandomStream& URunPersistData::GetEventStream() const
{
	return mEventStream;
}

/** @brief 현재 런의 스테이지를 반환한다. @return 스테이지(읽기 전용) */
const FStage& URunPersistData::GetStage() const
{
	return mStage.Get();
}

/**
 * @brief 스테이지 격자 좌표로 특정 방을 반환한다.
 * @param RowIndex    행 인덱스
 * @param ColumnIndex 열 인덱스
 * @return 해당 좌표의 방(읽기 전용)
 */
const FRoom& URunPersistData::GetRoom(int32 RowIndex, int32 ColumnIndex) const
{
	return mStage.Get().GetRoom(RowIndex, ColumnIndex);
}

/** @brief 스테이지의 시작 방을 반환한다. @return 시작 방(읽기 전용) */
const FRoom& URunPersistData::GetStartRoom() const
{
	return mStage.Get().GetStartRoom();
}

/** @brief 현재 진행 중인 방을 반환한다. @return 현재 방(읽기 전용) */
const FRoom& URunPersistData::GetCurrentRoom() const
{
	return mStage.Get().GetCurrentRoom();
}

/**
 * @brief 현재 진행 중인 방의 격자 좌표를 OUT 인자로 반환한다.
 * @param RowIndex    [out] 현재 방 행 인덱스
 * @param ColumnIndex [out] 현재 방 열 인덱스
 */
void URunPersistData::GetCurrentRoomIndex(OUT int32& RowIndex, OUT int32& ColumnIndex) const
{
	RowIndex = mStage.Get().mCurRow;
	ColumnIndex = mStage.Get().mCurColumn;
}

/** @brief 이번 런의 누적 로그를 반환한다. @return 런 로그(읽기 전용) */
const FRunLog& URunPersistData::GetRunLog() const
{
	return mRunLog;
}

/** @brief 런이 활성 상태인지(유효한 스테이지가 존재하는지) 여부를 반환한다. @return 스테이지 유효 시 true */
bool URunPersistData::IsActive() const
{
	return mStage.IsValid();
}

/**
 * @brief 새 유저를 생성한다. 기존 유저 데이터를 비운 뒤 표시 이름을 설정한다.
 * @param Name 새 유저의 표시 이름
 */
void UUserPersistData::MakeUser(const FText& Name)
{
	ClearUser();

	mUserName = Name;
}

/** @brief 유저 데이터를 초기화한다(이름 비우기 + 유저 로그 Clear). */
void UUserPersistData::ClearUser()
{
	mUserName = FText();
	mUserLog.Clear();
}

/**
 * @brief 한 런이 끝난 뒤 그 런 로그를 유저 누적 통계/도감에 반영한다.
 *        총 플레이 횟수와 유닛별 플레이 횟수를 증가시키고,
 *        런에서 처치/획득한 적·스킬·장비·주사위를 "이미 본" 식별자 집합에 추가한다.
 * @param PlayerUnitId 이번 런에서 사용한 플레이어 유닛 식별자(유닛별 카운트 키)
 * @param RunLog       반영할 런 단위 로그
 */
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

/** @brief 유저 표시 이름을 반환한다. @return 유저 이름 */
const FText& UUserPersistData::GetUserName() const
{
	return mUserName;
}

/** @brief 유저 데이터가 활성 상태인지(이름이 설정되어 있는지) 여부를 반환한다. @return 이름이 비어있지 않으면 true */
bool UUserPersistData::IsActive() const
{
	return mUserName.IsEmpty() == false;
}

/** @brief 유저 누적 로그(통계/도감)를 반환한다. @return 유저 로그(읽기 전용) */
const FUserLog& UUserPersistData::GetUserLog() const
{
	return mUserLog;
}

/**
 * @brief 옵션 영속 데이터 기본 생성자. 볼륨 배열을 볼륨 타입 개수만큼 1.0(최대)으로,
 *        사운드 클래스 캐시 배열을 같은 길이만큼 nullptr 로 초기화한다.
 */
UOptionPersistData::UOptionPersistData()
{
	mVolumes.Init(1.f, StaticCast<int32>(EGameVolumeType::Count));
	mOptionPersistDataCache.mSoundClassObjects.Init(nullptr, StaticCast<int32>(EGameVolumeType::Count));
}

/**
 * @brief 사운드 믹스/클래스 객체 캐시를 구성한다.
 *        설정에 지정된 SoundMix 와 볼륨 타입별 SoundClass 를 동기 로드해 캐시 배열에 채운다.
 */
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

/** @brief 옵션 데이터 초기 생성 훅(현재 별도 동작 없음, 향후 확장용). */
void UOptionPersistData::MakeOption()
{
}

/**
 * @brief 옵션을 기본값(CDO)으로 되돌리고 즉시 적용한다.
 *        볼륨/언어/해상도를 CDO 값으로 복사한 뒤 ApplyCurrentOptions 로 실제 시스템에 반영한다.
 */
void UOptionPersistData::ClearOption()
{
	const UOptionPersistData* CDO = GetDefault<UOptionPersistData>();
	mVolumes = CDO->mVolumes;
	mLanguageType = CDO->mLanguageType;
	mResolution = CDO->mResolution;

	ApplyCurrentOptions();
}

/**
 * @brief 특정 볼륨 타입의 볼륨을 설정하고 SoundMix 오버라이드로 즉시 반영한다.
 * @param VolumeType 설정할 볼륨 타입(마스터/BGM/SFX 등)
 * @param Volume     적용할 볼륨(0.0~1.0 범위로 클램프됨)
 */
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

/**
 * @brief 게임 언어(컬처)를 설정한다. 지원 언어를 컬처 코드로 매핑해 국제화 시스템에 적용한다.
 *        매핑되지 않은(빈 문자열) 언어 타입이면 아무 변경 없이 반환한다.
 * @param LanguageType 적용할 언어 타입(한국어/영어)
 */
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

/**
 * @brief 화면 해상도를 설정한다.
 * @param Resolution 적용할 해상도(가로x세로)
 * @note 미구현(TODO).
 */
void UOptionPersistData::SetResolution(const FIntPoint& Resolution)
{
	// TODO
}

/**
 * @brief 현재 보관 중인 모든 옵션(볼륨 전 타입 + 언어 + 해상도)을 실제 시스템에 일괄 적용한다.
 *        옵션 로드/초기화 직후 상태를 런타임에 동기화하는 진입점.
 */
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

/**
 * @brief 특정 볼륨 타입의 현재 볼륨을 반환한다.
 * @param VolumeType 조회할 볼륨 타입
 * @return 해당 타입의 볼륨(0.0~1.0)
 */
float UOptionPersistData::GetVolume(EGameVolumeType VolumeType) const
{
	return mVolumes[StaticCast<int32>(VolumeType)];
}

/** @brief 현재 설정된 언어 타입을 반환한다. @return 언어 타입 */
ELanguageType UOptionPersistData::GetLanguage() const
{
	return mLanguageType;
}

/** @brief 현재 설정된 해상도를 반환한다. @return 해상도(가로x세로) */
const FIntPoint& UOptionPersistData::GetResolution() const
{
	return mResolution;
}

/** @brief 옵션 데이터 활성 여부. 옵션은 항상 유효하므로 언제나 true. @return 항상 true */
bool UOptionPersistData::IsActive() const
{
	return true;
}
