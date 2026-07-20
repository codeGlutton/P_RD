#include "Component/SkillComponent/SkillComponentModel.h"

#include "Singleton/WorldSubsystem/PresentationBarrier.h"
#include "Singleton/WorldSubsystem/WorldCameraModel.h"

#include "Engine/AssetManager.h"
#include "DataAsset/SkillData/StaticSkillData.h"

#include "Pawn/UnitModel.h"
#include "Actor/BoardActor/BoardCombatTarget.h"

#include "Actor/TileMap/TileMapModel.h"

#include "Component/PassiveComponent/PassiveComponentModel.h"
#include "TAS/Passive/TacticalPassive.h"
#include "TAS/Passive/PassiveActivateContext.h"
#include "TAS/Passive/DynamicPassiveData.h"

#include "Simulation/Logger/EventLog.h"
#include "Simulation/Logger/EventLogger.h"

#include "Animation/Notify/EventTriggerPayload.h"

#include "Setting/GamePlaySettings.h"

namespace
{
	const FName DicePushSkillAssetName(TEXT("DA_SwordNormalSmash_Common"));
	const FName DicePullSkillAssetName(TEXT("DA_SwordBlade_Rare"));
	const FName DiceStaggerSkillAssetName(TEXT("DA_NomalDefense_Common"));
	const FName DiceSwapSkillAssetName(TEXT("DA_NomalHeal_Common"));
	constexpr int32 LongDisplacementAimRange = 8;
	constexpr int32 AdjacentDisplacementAimRange = 1;
	constexpr int32 IntegratedStrikeAimRange = 4;

	bool IsPlayerDisplacementSkill(const UStaticSkillData* SkillData, const UUnitModel* OwnerUnit)
	{
		return SkillData != nullptr
			&& OwnerUnit != nullptr
			&& OwnerUnit->IsPlayerUnitModel()
			&& (SkillData->GetFName() == DicePushSkillAssetName
				|| SkillData->GetFName() == DicePullSkillAssetName
				|| SkillData->GetFName() == DiceStaggerSkillAssetName
				|| SkillData->GetFName() == DiceSwapSkillAssetName);
	}

	int32 GetPlayerDisplacementAimRange(const UStaticSkillData* SkillData)
	{
		if (SkillData != nullptr
			&& (SkillData->GetFName() == DicePullSkillAssetName
				|| SkillData->GetFName() == DiceStaggerSkillAssetName))
		{
			return LongDisplacementAimRange;
		}
		return AdjacentDisplacementAimRange;
	}

	UStaticSkillData* LoadStaticSkillData(const FPrimaryAssetId& SkillId)
	{
		if (SkillId.IsValid() == false)
		{
			return nullptr;
		}

		UAssetManager* AssetManager = UAssetManager::GetIfInitialized();
		if (AssetManager == nullptr)
		{
			return nullptr;
		}

		if (UStaticSkillData* Loaded = AssetManager->GetPrimaryAssetObject<UStaticSkillData>(SkillId))
		{
			return Loaded;
		}

		const FSoftObjectPath AssetPath = AssetManager->GetPrimaryAssetPath(SkillId);
		return Cast<UStaticSkillData>(AssetPath.TryLoad());
	}
}

void FActiveSkillContext::Clear()
{
	mMapModel = nullptr;
	mDiceSum = 0;
	mSelfTileIndex = FTileIndex::Invalid;
	mTargetTileIndex = FTileIndex::Invalid;
	mEffectTileIndexes.Reset();
	mAllowFriendlyFire = false;

	mSkillIndex = INDEX_NONE;
	mMotionIndex = INDEX_NONE;

	mEndCallback.Clear();
	mTriggerCallback.Unbind();

	mTargetTileIndexes.Reset();
	mOtherCombatTargets.Reset();
	mResolvedCombatTargets.Reset();

	mMotionTileMapDir = ETileActorDirection::Forward;
	mMotionEndBarrier = nullptr;
	mIsMotionTriggered = false;
}

bool FActiveSkillContext::IsValid() const
{
	return mMapModel != nullptr;
}

FSkillEntry::FSkillEntry(UStaticSkillData* Data) : mData(Data)
{
}

bool FSkillEntry::IsValid() const
{
	return mData != nullptr;
}

USkillComponentModel::USkillComponentModel()
{
	mSkillEntries.Init(FSkillEntry(), 4 /*추가 스킬*/ + 2 /*기본 스킬*/);
}

void USkillComponentModel::SetSkillFrom(const TArray<TSoftObjectPtr<UStaticSkillData>>& SkillList)
{
	// 초기화 로직 (몬스터는 6개 이상의 스킬도 소유할 수 있음)

	int32 NextSkillIndex = 0;
	for (const TSoftObjectPtr<UStaticSkillData>& Skill : SkillList)
	{
		SetSkill(NextSkillIndex++, Skill.Get());
	}
}

void USkillComponentModel::SetSkillFrom(const TArray<FPrimaryAssetId>& SkillList)
{
	// 초기화 로직 (몬스터는 6개 이상의 스킬도 소유할 수 있음)

	int32 NextSkillIndex = 0;
	for (const FPrimaryAssetId& AssetId : SkillList)
	{
		UStaticSkillData* StaticSkillData = LoadStaticSkillData(AssetId);
		if (StaticSkillData != nullptr)
		{
			SetSkill(NextSkillIndex++, StaticSkillData);
		}
	}
}

const TArray<FSkillEntry>& USkillComponentModel::GetSkills() const
{
	return mSkillEntries;
}

const FSkillEntry* USkillComponentModel::GetSkill(int32 SkillIndex) const
{
	checkf(mSkillEntries.IsValidIndex(SkillIndex) == true, TEXT("잘못된 스킬 인덱스 범위"));
	return &mSkillEntries[SkillIndex];
}

void USkillComponentModel::SetSkill(int32 SkillIndex, UStaticSkillData* SkillData)
{
	checkf(mSkillEntries.IsValidIndex(SkillIndex) == true, TEXT("잘못된 스킬 인덱스 범위"));

	const UStaticSkillData* PreSkillData = mSkillEntries[SkillIndex].mData;
	mSkillEntries[SkillIndex] = FSkillEntry(SkillData);

	OnChangeSkillUI.Broadcast(SkillIndex, SkillData, PreSkillData);
}

void USkillComponentModel::ActivateSkill(
	UTileMapModel* MapModel,
	int32 SkillIndex,
	const FTileIndex& TargetIndex,
	int32 DiceSum,
	FOnEndSkillUI Callback,
	const TArray<FTileIndex>* FixedEffectTileIndexes,
	bool bAllowFriendlyFire,
	FOnTriggerSkillMotionUI TriggerCallback)
{
	checkf(mSkillEntries.IsValidIndex(SkillIndex) == true, TEXT("잘못된 사용 스킬 인덱스"));

	FSkillEntry& SkillEntry = mSkillEntries[SkillIndex];
	checkf(SkillEntry.IsValid() == true, TEXT("빈 스킬 시전 오류"));

	const UStaticSkillData* SkillData = SkillEntry.mData;
	checkf(SkillData != nullptr, TEXT("빈 스킬 시전 오류"));

	UUnitModel* OwnerUnitModel = GetOwnerModel<UUnitModel>();
	checkf(OwnerUnitModel != nullptr, TEXT("스킬을 시전할 Owner가 유효하지 않음"));

	UPassiveComponentModel* PassiveComponentModel = OwnerUnitModel->GetPassiveComponentModel();
	checkf(PassiveComponentModel != nullptr, TEXT("패시브 컴포넌트 nullptr"));

	/* 활성화 스킬 데이터 채우기 */

	mActiveSkillContext.mDiceSum = DiceSum;
	mActiveSkillContext.mMapModel = MapModel;
	mActiveSkillContext.mSelfTileIndex = OwnerUnitModel->GetTileTransform().mIndex;
	mActiveSkillContext.mTargetTileIndex = TargetIndex;
	mActiveSkillContext.mEffectTileIndexes = FixedEffectTileIndexes != nullptr
		? *FixedEffectTileIndexes
		: GetEffectTiles(MapModel, SkillIndex, TargetIndex, DiceSum);
	mActiveSkillContext.mAllowFriendlyFire = bAllowFriendlyFire;
	mActiveSkillContext.mSkillIndex = SkillIndex;
	mActiveSkillContext.mMotionIndex = 0;
	mActiveSkillContext.mEndCallback = MoveTemp(Callback);
	mActiveSkillContext.mTriggerCallback = MoveTemp(TriggerCallback);

	/* 스킬 실행 콜백 */

	OnPlaySkillUI.Broadcast(mActiveSkillContext, SkillData);

	/* 카메라 줌인 */

	FVector MinTileLocation;
	FVector MaxTileLocation;
	FVector ZoomInLocation = MapModel->TileToWorldLocation(mActiveSkillContext.mSelfTileIndex);
	MinTileLocation = MaxTileLocation = ZoomInLocation;

	for (const FTileIndex& EffectTileIndex : mActiveSkillContext.mEffectTileIndexes)
	{
		const FVector TileLocation = MapModel->TileToWorldLocation(EffectTileIndex);
		ZoomInLocation += TileLocation;

		MinTileLocation.X = FMath::Min(MinTileLocation.X, TileLocation.X);
		MinTileLocation.Y = FMath::Min(MinTileLocation.Y, TileLocation.Y);
		MaxTileLocation.X = FMath::Max(MaxTileLocation.X, TileLocation.X);
		MaxTileLocation.Y = FMath::Max(MaxTileLocation.Y, TileLocation.Y);
	}
	ZoomInLocation /= (1 + mActiveSkillContext.mEffectTileIndexes.Num());
	ZoomInLocation.Z = OwnerUnitModel->GetWorldTransform().GetLocation().Z;

	const float ZoomInSize = FMath::Max(MaxTileLocation.X - MinTileLocation.X, MaxTileLocation.Y - MinTileLocation.Y);

	UWorldCameraModel* WorldCameraModel = GetWorldSubsystemModel<UWorldCameraModel>(this);
	checkf(WorldCameraModel != nullptr, TEXT("월드 카메라 모델 nullptr"));

	const UGamePlaySettings* GamePlaySettings = GetDefault<UGamePlaySettings>();
	checkf(GamePlaySettings != nullptr, TEXT("게임 플레이 세팅 nullptr"));

	const float FinalZoomInSize = FMath::Clamp(GamePlaySettings->mSkillZoomDefaultSize + ZoomInSize * GamePlaySettings->mSkillZoomSizeRatio, GamePlaySettings->mSkillMinZoomSize, GamePlaySettings->mSkillMaxZoomSize);
	WorldCameraModel->RequestZoomInMainCamera(ZoomInLocation, FinalZoomInSize);

	/* 스킬 사용 시 패시브 발동 */

	TArray<UTacticalPassive*> Passives = PassiveComponentModel->GetPassivesByTiming(AbilityTags::GameplayAbility_Passive_OnStartUsingSkill);

	UBoardCombatTargetSnapshotData* OwnerSnapshot = OwnerUnitModel->MakeSnapshotData();

	FPassiveActivateContext PassiveContext;
	PassiveContext.mOwner = OwnerUnitModel;
	PassiveContext.mOwnerSnapshot = OwnerSnapshot;
	PassiveContext.mTargets.Add(OwnerUnitModel);
	PassiveContext.mTargetSnapshots.Add(OwnerSnapshot);

	for (UTacticalPassive*& Passive : Passives)
	{
		TInstancedStruct<FDynamicPassiveData> DynamicPassiveData;
		Passive->ActivatePassive(AbilityTags::GameplayAbility_Passive_OnStartUsingSkill, PassiveContext, OUT DynamicPassiveData);
		Passive->CommitPassive(DynamicPassiveData);
	}

	
	// 모션 레이어가 하나도 없는(미저작) 스킬은 시전을 무동작으로 즉시 종료
	if (SkillData->mSkillMotionLayers.Num() == 0)
	{
		UE_LOG(LogRD, Warning, TEXT("스킬(index %d)에 모션 레이어가 없어 시전을 건너뜁니다 — DA에 mSkillMotionLayers 미설정"), SkillIndex);
		DeactivateSkill();
		return;
	}

	PlayMotionLayer();
}

void USkillComponentModel::PlayMotionLayer()
{
	checkf(mSkillEntries.IsValidIndex(mActiveSkillContext.mSkillIndex) == true, TEXT("잘못된 사용 스킬 인덱스"));

	FSkillEntry& SkillEntry = mSkillEntries[mActiveSkillContext.mSkillIndex];
	checkf(SkillEntry.IsValid() == true, TEXT("빈 스킬 시전 오류"));

	const UStaticSkillData* SkillData = SkillEntry.mData;
	checkf(SkillData != nullptr, TEXT("빈 스킬 시전 오류"));

	UUnitModel* OwnerUnitModel = GetOwnerModel<UUnitModel>();
	checkf(OwnerUnitModel != nullptr, TEXT("스킬을 시전할 Owner가 유효하지 않음"));

	IBoardCombatTarget* OwnerCombatTarget = Cast<IBoardCombatTarget>(OwnerUnitModel);
	checkf(OwnerCombatTarget != nullptr, TEXT("스킬을 시전할 Owner가 유효하지 않음"));

	UPassiveComponentModel* PassiveComponentModel = OwnerUnitModel->GetPassiveComponentModel();
	checkf(PassiveComponentModel != nullptr, TEXT("패시브 컴포넌트 nullptr"));

	/* 모션 로그 시작 */

	GetWorldEventLogger(this)->BeginMotionLog();
	const FSkillMotionLayer& MotionLayer = SkillData->mSkillMotionLayers[mActiveSkillContext.mMotionIndex];

	/* 활성화 모션 데이터 채우기 */

	mActiveSkillContext.mTargetTileIndexes = MotionLayer.FilterTileIndexes(mActiveSkillContext.mSelfTileIndex, mActiveSkillContext.mEffectTileIndexes);
	mActiveSkillContext.mOtherCombatTargets = MotionLayer.FilterCombatTargets(mActiveSkillContext.mMapModel.Get(), OwnerCombatTarget, mActiveSkillContext.mTargetTileIndexes);
	// 직접 조작 전술은 원본 스킬 데이터의 아군/자가 버프 필터를 재사용하지 않는다.
	// 손가락으로 고른 한 적이 애니메이션과 후속 위치/계획 판정의 대상이 되도록 명시적으로 고정한다.
	if (IsPlayerDisplacementSkill(SkillData, OwnerUnitModel))
	{
		mActiveSkillContext.mOtherCombatTargets.Reset();
		for (UBoardActorModel* BoardActor : mActiveSkillContext.mMapModel->GetActorsOnTile(
			mActiveSkillContext.mTargetTileIndex,
			ETileLayerFlag::Unit))
		{
			IBoardCombatTarget* CombatTarget = Cast<IBoardCombatTarget>(BoardActor);
			if (CombatTarget != nullptr && CombatTarget != OwnerCombatTarget && CombatTarget->IsTargetable())
			{
				mActiveSkillContext.mOtherCombatTargets.AddUnique(CombatTarget);
			}
		}
	}

	// 적의 고정 의도 중 'Hostile 대상 모션'만 오사 대상을 추가한다.
	// 기존 필터 결과는 보존해서 자가 버프/아군 보조 모션의 원래 대상을 망가뜨리지 않는다.
	const bool bHostileMotion = EnumHasAnyFlags(
		StaticCast<ETeamAttitudeFilter>(MotionLayer.mTeamAttitudeFilter),
		ETeamAttitudeFilter::Hostile);
	if (mActiveSkillContext.mAllowFriendlyFire && bHostileMotion)
	{
		for (const FTileIndex& TargetTileIndex : mActiveSkillContext.mTargetTileIndexes)
		{
			for (UBoardActorModel* BoardActor : mActiveSkillContext.mMapModel->GetActorsOnTile(TargetTileIndex))
			{
				IBoardCombatTarget* CombatTarget = Cast<IBoardCombatTarget>(BoardActor);
				if (CombatTarget != nullptr
					&& CombatTarget != OwnerCombatTarget
					&& CombatTarget->IsTargetable())
				{
					mActiveSkillContext.mOtherCombatTargets.AddUnique(CombatTarget);
				}
			}
		}
	}

	for (IBoardCombatTarget* CombatTarget : mActiveSkillContext.mOtherCombatTargets)
	{
		mActiveSkillContext.mResolvedCombatTargets.AddUnique(CombatTarget);
	}
	mActiveSkillContext.mMotionTileMapDir = mActiveSkillContext.mMapModel->TileDeltaToDirection(
		mActiveSkillContext.mSelfTileIndex,
		mActiveSkillContext.mTargetTileIndex,
		OwnerUnitModel->GetTileTransform().mDirection
	);
	mActiveSkillContext.mIsMotionTriggered = false;

	/* Effect 기본 값부터 참고용으로 적용 */

	for (const TInstancedStruct<FSkillEffectLayer>& EffectLayer : MotionLayer.mSkillEffectLayers)
	{
		EffectLayer.Get().ApplyPointEffect(OwnerCombatTarget, mActiveSkillContext.mDiceSum);
	}

	/* 이펙트 가격 전 패시브 적용 */

	{
		UBoardCombatTargetSnapshotData* OwnerSnapshot = OwnerCombatTarget->MakeSnapshotData();

		TArray<UTacticalPassive*> Passives = PassiveComponentModel->GetPassivesByTiming(AbilityTags::GameplayAbility_Passive_OnStartApplyingEffect);

		FPassiveActivateContext PassiveContext;
		PassiveContext.mOwner = OwnerUnitModel;
		PassiveContext.mOwnerSnapshot = OwnerSnapshot;

		TArray<UBoardCombatTargetSnapshotData*> OtherSnapshots;
		OtherSnapshots.Reserve(mActiveSkillContext.mOtherCombatTargets.Num());
		for (IBoardCombatTarget* OtherCombatTarget : mActiveSkillContext.mOtherCombatTargets)
		{
			UBoardActorModel* OtherActorModel = Cast<UBoardActorModel>(OtherCombatTarget);
			checkf(OtherActorModel != nullptr, TEXT("스킬을 받는 타겟이 유효하지 않음"));
			OtherSnapshots.Add(OtherCombatTarget->MakeSnapshotData());

			PassiveContext.mTargets.Add(OtherActorModel);
			PassiveContext.mTargetSnapshots.Add(OtherSnapshots.Last());
		}

		for (UTacticalPassive* Passive : Passives)
		{
			TInstancedStruct<FDynamicPassiveData> DynamicPassiveData;

			Passive->ActivatePassive(AbilityTags::GameplayAbility_Passive_OnStartApplyingEffect, PassiveContext, OUT DynamicPassiveData);
			Passive->CommitPassive(DynamicPassiveData);
		}
	}

	/* 이펙트 피격 전 패시브 적용(선택적) */
	{
		UBoardCombatTargetSnapshotData* OwnerSnapshot = OwnerCombatTarget->MakeSnapshotData();

		for (IBoardCombatTarget* OtherCombatTarget : mActiveSkillContext.mOtherCombatTargets)
		{
			UBoardActorModel* OtherActorModel = Cast<UBoardActorModel>(OtherCombatTarget);
			checkf(OtherActorModel != nullptr, TEXT("스킬을 받는 타겟이 유효하지 않음"));

			UPassiveComponentModel* OtherPassiveComponentModel = OtherActorModel->FindComponentModelByClass<UPassiveComponentModel>();
			if (OtherPassiveComponentModel == nullptr)
			{
				continue;
			}

			TArray<UTacticalPassive*> OtherPassives = OtherPassiveComponentModel->GetPassivesByTiming(AbilityTags::GameplayAbility_Passive_OnStartReceivingEffect);

			UBoardCombatTargetSnapshotData* OtherSnapshot = OtherCombatTarget->MakeSnapshotData();

			FPassiveActivateContext PassiveContext;
			PassiveContext.mOwner = OtherActorModel;
			PassiveContext.mOwnerSnapshot = OtherSnapshot;
			PassiveContext.mTargets.Add(OwnerUnitModel);
			PassiveContext.mTargetSnapshots.Add(OwnerSnapshot);

			for (UTacticalPassive* OtherPassive : OtherPassives)
			{
				TInstancedStruct<FDynamicPassiveData> DynamicPassiveData;

				OtherPassive->ActivatePassive(AbilityTags::GameplayAbility_Passive_OnStartReceivingEffect, PassiveContext, OUT DynamicPassiveData);
				OtherPassive->CommitPassive(DynamicPassiveData);
			}
		}
	}

	/* 애니메이션 시작 */

	// 자동 회전
	if (MotionLayer.mAutoRotateTowardTarget == true)
	{
		// 회전이 끝나면 베리어가 소멸하면서 PlayMotionLayerAnimation() 함수 호출.
		// 이때 방향은 로컬방향이라서 Forward는 정면을 향하고 있음
		TSharedPtr<FPresentationBarrier> RotateBarrier = FPresentationBarrier::Make(
			FOnFinishPresentation::CreateWeakLambda(this, [this]() {
				PlayMotionLayerAnimation(ETileActorDirection::Forward);
				}));
		
		// 타일맵의 절대적 방향으로 회전
		mActiveSkillContext.mMapModel->RotateActor(mActiveSkillContext.mMotionTileMapDir, OwnerUnitModel, RotateBarrier);
	}
	// 자동 회전 안 함
	else
	{
		// 회전하지 않으므로 즉시 모션 시작
		PlayMotionLayerAnimation(TileMapToLocalDirection(mActiveSkillContext.mMotionTileMapDir, OwnerUnitModel->GetTileTransform().mDirection));
	}
}

void USkillComponentModel::PlayMotionLayerAnimation(ETileActorDirection LocalDirectionToTarget)
{
	checkf(mSkillEntries.IsValidIndex(mActiveSkillContext.mSkillIndex) == true, TEXT("잘못된 사용 스킬 인덱스"));

	FSkillEntry& SkillEntry = mSkillEntries[mActiveSkillContext.mSkillIndex];
	checkf(SkillEntry.IsValid() == true, TEXT("빈 스킬 시전 오류"));

	const UStaticSkillData* SkillData = SkillEntry.mData;
	checkf(SkillData != nullptr, TEXT("빈 스킬 시전 오류"));

	UUnitModel* OwnerUnitModel = GetOwnerModel<UUnitModel>();
	checkf(OwnerUnitModel != nullptr, TEXT("스킬을 시전할 Owner가 유효하지 않음"));

	const FSkillMotionLayer& MotionLayer = SkillData->mSkillMotionLayers[mActiveSkillContext.mMotionIndex];

	auto MotionEndBarrier = FPresentationBarrier::Make(FOnFinishPresentation::CreateWeakLambda(this, [this]() {
		TriggerMotionLayer(nullptr);
		EndMotionLayer();
		}));

	FOnRequestReceiveAnimation TriggerCallback;
	TriggerCallback.BindWeakLambda(this, [this](const FApplyEventTriggerPayload* Payload) {
		TriggerMotionLayer(Payload);
		});

	mActiveSkillContext.mMotionEndBarrier = MotionEndBarrier;

	OwnerUnitModel->OnPlayApplyAnimationUI.Broadcast(MotionEndBarrier, MoveTemp(TriggerCallback), MotionLayer.mApplyMotionTag, LocalDirectionToTarget);
	OnPlayMotionLayerUI.Broadcast(mActiveSkillContext.mMotionIndex, MotionEndBarrier, MotionLayer.mApplyMotionTag, LocalDirectionToTarget);
}

void USkillComponentModel::TriggerMotionLayer(const FApplyEventTriggerPayload* Payload)
{
	if (mActiveSkillContext.mIsMotionTriggered == true)
	{
		return;
	}
	mActiveSkillContext.mIsMotionTriggered = true;

	checkf(mSkillEntries.IsValidIndex(mActiveSkillContext.mSkillIndex) == true, TEXT("잘못된 사용 스킬 인덱스"));

	FSkillEntry& SkillEntry = mSkillEntries[mActiveSkillContext.mSkillIndex];
	checkf(SkillEntry.IsValid() == true, TEXT("빈 스킬 시전 오류"));

	const UStaticSkillData* SkillData = SkillEntry.mData;
	checkf(SkillData != nullptr, TEXT("빈 스킬 시전 오류"));

	UUnitModel* OwnerUnitModel = GetOwnerModel<UUnitModel>();
	checkf(OwnerUnitModel != nullptr, TEXT("스킬을 시전할 Owner가 유효하지 않음"));

	IBoardCombatTarget* OwnerCombatTarget = Cast<IBoardCombatTarget>(OwnerUnitModel);
	checkf(OwnerCombatTarget != nullptr, TEXT("스킬을 시전할 Owner가 유효하지 않음"));

	UPassiveComponentModel* PassiveComponentModel = OwnerUnitModel->GetPassiveComponentModel();
	checkf(PassiveComponentModel != nullptr, TEXT("패시브 컴포넌트 nullptr"));

	const FSkillMotionLayer& MotionLayer = SkillData->mSkillMotionLayers[mActiveSkillContext.mMotionIndex];

	/* 실제 Effect 적용 */

	{
		TArray<FActiveTacticalEffectHandle> FactorEffectHandles;
		FactorEffectHandles.Reserve(MotionLayer.mSkillEffectLayers.Num());

		const int32 EffectLayerNum = MotionLayer.mSkillEffectLayers.Num();
		for (int32 i = 0; i < EffectLayerNum; ++i)
		{
			const TInstancedStruct<FSkillEffectLayer>& EffectLayer = MotionLayer.mSkillEffectLayers[i];
			FactorEffectHandles.Add(EffectLayer.Get().ApplyFactorEffect(OwnerCombatTarget));
		}

		UBoardCombatTargetSnapshotData* OwnerSnapshot = OwnerCombatTarget->MakeSnapshotData();
		TArray<TScriptInterface<IBoardCombatTarget>> OtherCombatTargets;
		OtherCombatTargets.Reserve(mActiveSkillContext.mOtherCombatTargets.Num());
		TArray<TObjectPtr<UBoardCombatTargetSnapshotData>> OtherSnapshots;
		OtherSnapshots.Reserve(mActiveSkillContext.mOtherCombatTargets.Num());
		for (IBoardCombatTarget* OtherCombatTarget : mActiveSkillContext.mOtherCombatTargets)
		{
			UBoardActorModel* OtherActorModel = Cast<UBoardActorModel>(OtherCombatTarget);
			checkf(OtherActorModel != nullptr, TEXT("스킬을 받는 타겟이 유효하지 않음"));

			OtherCombatTargets.Add(OtherActorModel);
			OtherSnapshots.Add(OtherCombatTarget->MakeSnapshotData());
		}

		FSkillEffectCommitParams Params(
			OwnerUnitModel,
			OwnerSnapshot, 
			OtherCombatTargets, 
			OtherSnapshots, 
			mActiveSkillContext.mTargetTileIndexes, 
			mActiveSkillContext.mDiceSum
		);
		// 이 브랜치에서 플레이어 강타는 피해량 스킬이 아니라 위치를 바꾸는 순수 개입기다.
		// 대상 수집/애니메이션은 그대로 두고 실제 효과 commit만 생략해, 대상이 먼저 죽어
		// 후속 강제 이동과 공개 계획 갱신이 누락되는 경우를 막는다.
		if (IsPlayerDisplacementSkill(SkillData, OwnerUnitModel) == false)
		{
			for (int32 i = 0; i < EffectLayerNum; ++i)
			{
				const TInstancedStruct<FSkillEffectLayer>& EffectLayer = MotionLayer.mSkillEffectLayers[i];
				EffectLayer.Get().CommitEffect(Params);
			}
		}

		for (int32 i = 0; i < EffectLayerNum; ++i)
		{
			const TInstancedStruct<FSkillEffectLayer>& EffectLayer = MotionLayer.mSkillEffectLayers[i];
			EffectLayer.Get().ClearFactorEffect(OwnerCombatTarget, FactorEffectHandles[i]);
		}
	}

	/* 피격 애니메이션 적용 */

	for (IBoardCombatTarget* OtherCombatTarget : mActiveSkillContext.mOtherCombatTargets)
	{
		UBoardActorModel* OtherActorModel = Cast<UBoardActorModel>(OtherCombatTarget);
		checkf(OtherActorModel != nullptr, TEXT("스킬을 받은 타겟이 유효하지 않음"));

		ETileActorDirection LocalDirectionToTarget = TileMapToLocalDirection(mActiveSkillContext.mMotionTileMapDir, OtherActorModel->GetTileTransform().mDirection);
		OtherActorModel->OnPlayReceiveAnimationUI.Broadcast(mActiveSkillContext.mMotionEndBarrier.Pin(), Payload, MotionLayer.mReceiveMotionTag, LocalDirectionToTarget);
	}

	/* 이펙트 피격 후 패시브 적용 (선택적) */

	{
		UBoardCombatTargetSnapshotData* OwnerSnapshot = OwnerCombatTarget->MakeSnapshotData();

		for (IBoardCombatTarget* OtherCombatTarget : mActiveSkillContext.mOtherCombatTargets)
		{
			UBoardActorModel* OtherActorModel = Cast<UBoardActorModel>(OtherCombatTarget);
			checkf(OtherActorModel != nullptr, TEXT("스킬을 받은 타겟이 유효하지 않음"));

			UPassiveComponentModel* OtherPassiveComponentModel = OtherActorModel->FindComponentModelByClass<UPassiveComponentModel>();
			if (OtherPassiveComponentModel == nullptr)
			{
				continue;
			}

			TArray<UTacticalPassive*> OtherPassives = OtherPassiveComponentModel->GetPassivesByTiming(AbilityTags::GameplayAbility_Passive_OnEndReceivingEffect);

			UBoardCombatTargetSnapshotData* OtherSnapshot = OtherCombatTarget->MakeSnapshotData();

			FPassiveActivateContext PassiveContext;
			PassiveContext.mOwner = OtherActorModel;
			PassiveContext.mOwnerSnapshot = OtherSnapshot;
			PassiveContext.mTargets.Add(OwnerUnitModel);
			PassiveContext.mTargetSnapshots.Add(OwnerSnapshot);

			for (UTacticalPassive* OtherPassive : OtherPassives)
			{
				TInstancedStruct<FDynamicPassiveData> DynamicPassiveData;

				OtherPassive->ActivatePassive(AbilityTags::GameplayAbility_Passive_OnEndReceivingEffect, PassiveContext, OUT DynamicPassiveData);
				OtherPassive->CommitPassive(DynamicPassiveData);
			}
		}
	}

	/* 이펙트 가격 후 패시브 적용 */
	{
		UBoardCombatTargetSnapshotData* OwnerSnapshot = OwnerCombatTarget->MakeSnapshotData();

		TArray<UTacticalPassive*> Passives = PassiveComponentModel->GetPassivesByTiming(AbilityTags::GameplayAbility_Passive_OnEndApplyingEffect);

		FPassiveActivateContext PassiveContext;
		PassiveContext.mOwner = OwnerUnitModel;
		PassiveContext.mOwnerSnapshot = OwnerSnapshot;

		TArray<UBoardCombatTargetSnapshotData*> OtherSnapshots;
		OtherSnapshots.Reserve(mActiveSkillContext.mOtherCombatTargets.Num());
		for (IBoardCombatTarget* OtherCombatTarget : mActiveSkillContext.mOtherCombatTargets)
		{
			UBoardActorModel* OtherActorModel = Cast<UBoardActorModel>(OtherCombatTarget);
			checkf(OtherActorModel != nullptr, TEXT("스킬을 받는 타겟이 유효하지 않음"));
			OtherSnapshots.Add(OtherCombatTarget->MakeSnapshotData());

			PassiveContext.mTargets.Add(OtherActorModel);
			PassiveContext.mTargetSnapshots.Add(OtherSnapshots.Last());
		}

		for (UTacticalPassive* Passive : Passives)
		{
			TInstancedStruct<FDynamicPassiveData> DynamicPassiveData;

			Passive->ActivatePassive(AbilityTags::GameplayAbility_Passive_OnEndApplyingEffect, PassiveContext, OUT DynamicPassiveData);
			Passive->CommitPassive(DynamicPassiveData);
		}
	}

	// 피해/피격 처리가 발생한 Hit 노티 프레임에 후속 물리 반응을 시작한다.
	// 전체 몽타주 종료까지 기다리지 않으므로 강타의 밀치기가 타격과 붙어서 재생된다.
	mActiveSkillContext.mTriggerCallback.ExecuteIfBound(mActiveSkillContext, SkillData);
}

void USkillComponentModel::EndMotionLayer()
{
	checkf(mSkillEntries.IsValidIndex(mActiveSkillContext.mSkillIndex) == true, TEXT("잘못된 사용 스킬 인덱스"));

	FSkillEntry& SkillEntry = mSkillEntries[mActiveSkillContext.mSkillIndex];
	checkf(SkillEntry.IsValid() == true, TEXT("빈 스킬 시전 오류"));

	const UStaticSkillData* SkillData = SkillEntry.mData;
	checkf(SkillData != nullptr, TEXT("빈 스킬 시전 오류"));

	UUnitModel* OwnerUnitModel = GetOwnerModel<UUnitModel>();
	checkf(OwnerUnitModel != nullptr, TEXT("스킬을 시전한 Owner가 유효하지 않음"));

	IBoardCombatTarget* OwnerCombatTarget = Cast<IBoardCombatTarget>(OwnerUnitModel);
	checkf(OwnerCombatTarget != nullptr, TEXT("스킬을 시전한 Owner가 유효하지 않음"));

	const FSkillMotionLayer& MotionLayer = SkillData->mSkillMotionLayers[mActiveSkillContext.mMotionIndex];

	/* Effect 포인트 수치 비우기 */
	for (const TInstancedStruct<FSkillEffectLayer>& EffectLayer : MotionLayer.mSkillEffectLayers)
	{
		EffectLayer.Get().ClearPointEffect(OwnerCombatTarget);
	}

	/* 모션 로그 종료 */
	
	GetWorldEventLogger(this)->EndMotionLog();
	OnEndMotionLayerUI.Broadcast(mActiveSkillContext.mMotionIndex);

	/* 종료 판정 */

	++mActiveSkillContext.mMotionIndex;
	if (SkillData->mSkillMotionLayers.Num() == mActiveSkillContext.mMotionIndex)
	{
		DeactivateSkill();
	}
	else
	{
		PlayMotionLayer();
	}
}

void USkillComponentModel::DeactivateSkill()
{
	FSkillEntry& SkillEntry = mSkillEntries[mActiveSkillContext.mSkillIndex];
	checkf(SkillEntry.IsValid() == true, TEXT("빈 스킬 시전 오류"));

	const UStaticSkillData* SkillData = SkillEntry.mData;
	checkf(SkillData != nullptr, TEXT("빈 스킬 시전 오류"));

	UUnitModel* OwnerUnitModel = GetOwnerModel<UUnitModel>();
	checkf(OwnerUnitModel != nullptr, TEXT("스킬을 시전할 Owner가 유효하지 않음"));

	UPassiveComponentModel* PassiveComponentModel = OwnerUnitModel->GetPassiveComponentModel();
	checkf(PassiveComponentModel != nullptr, TEXT("패시브 컴포넌트 nullptr"));

	/* 스킬 종료 시 패시브 발동 */

	TArray<UTacticalPassive*> Passives = PassiveComponentModel->GetPassivesByTiming(AbilityTags::GameplayAbility_Passive_OnEndUsingSkill);

	UBoardCombatTargetSnapshotData* OwnerSnapshot = OwnerUnitModel->MakeSnapshotData();

	FPassiveActivateContext PassiveContext;
	PassiveContext.mOwner = OwnerUnitModel;
	PassiveContext.mOwnerSnapshot = OwnerSnapshot;
	PassiveContext.mTargets.Add(OwnerUnitModel);
	PassiveContext.mTargetSnapshots.Add(OwnerSnapshot);

	for (UTacticalPassive*& Passive : Passives)
	{
		TInstancedStruct<FDynamicPassiveData> DynamicPassiveData;
		Passive->ActivatePassive(AbilityTags::GameplayAbility_Passive_OnEndUsingSkill, PassiveContext, OUT DynamicPassiveData);
		Passive->CommitPassive(DynamicPassiveData);
	}

	/* 카메라 줌아웃 */

	UWorldCameraModel* WorldCameraModel = GetWorldSubsystemModel<UWorldCameraModel>(this);
	checkf(WorldCameraModel != nullptr, TEXT("월드 카메라 모델 nullptr"));

	WorldCameraModel->RequestZoomOutMainCamera();

	/* 스킬 종료 콜백 */

	mActiveSkillContext.mEndCallback.Broadcast(mActiveSkillContext, SkillData);
	OnEndSkillUI.Broadcast(mActiveSkillContext, SkillData);

	/* 활성화 스킬 데이터 비우기 */

	mActiveSkillContext.Clear();
}

bool USkillComponentModel::IsAnySkillActivated() const
{
	return mActiveSkillContext.IsValid() == true;
}

TArray<FTileIndex> USkillComponentModel::GetAimableTiles(UTileMapModel* MapModel, int32 SkillIndex, int32 DiceSum) const
{
	TArray<FTileIndex> AimableTiles;
	checkf(mSkillEntries.IsValidIndex(SkillIndex) == true, TEXT("잘못된 스킬 인덱스 범위"));
	UStaticSkillData* StaticSkillData = mSkillEntries[SkillIndex].mData;
	checkf(StaticSkillData != nullptr, TEXT("잘못된 스킬 데이터"));

	const bool bIsDisplacement = IsPlayerDisplacementSkill(
		StaticSkillData,
		GetOwnerModel<UUnitModel>());
	const bool bIsPlayerIntegratedStrike = bIsDisplacement == false
		&& GetOwnerModel<UUnitModel>() != nullptr
		&& GetOwnerModel<UUnitModel>()->IsPlayerUnitModel();
	// 위치 개입 스킬의 주사위 합은 강제 이동 거리에만 쓰고, 조준 사거리는 역할별 고정값을 쓴다.
	// 당기기/다리 걸기는 원거리(8), 던지기/자리 바꾸기는 인접(1)이며 UI 표기와 실제 판정이 같다.
	const float AimRange = bIsDisplacement
		? StaticCast<float>(GetPlayerDisplacementAimRange(StaticSkillData))
		: (bIsPlayerIntegratedStrike
			? StaticCast<float>(IntegratedStrikeAimRange)
			: StaticSkillData->mAimRangeDefaultValue + DiceSum * StaticSkillData->mAimRangeRatio);
	const EAimPattern Pattern = bIsDisplacement || bIsPlayerIntegratedStrike
		? EAimPattern::Square : StaticSkillData->mAimPattern;
	const bool CanAimObstacle = bIsDisplacement || bIsPlayerIntegratedStrike || StaticSkillData->mCanAimBoardActor;
	const bool IsIndirect = bIsDisplacement || bIsPlayerIntegratedStrike || StaticSkillData->mIsIndirect;

	AimableTiles = MapModel->GetAimableTiles(
		GetOwnerModel<UBoardActorModel>()->GetTileTransform().mIndex,
		AimRange,
		Pattern,
		CanAimObstacle,
		IsIndirect);
	// 빈 칸도 반환해야 HUD가 사거리 영역 전체를 보여줄 수 있다. 실제 대상 선택 가능 여부는
	// SRPGSkillBuildAction::CanSelectTargetTile에서 적 유닛 존재/인접 조건을 별도로 검사한다.
	return AimableTiles;
}

TArray<FTileIndex> USkillComponentModel::GetEffectTiles(UTileMapModel* MapModel, int32 SkillIndex, const FTileIndex& TargetIndex, int32 DiceSum) const
{
	TArray<FTileIndex> AimableTiles;
	checkf(mSkillEntries.IsValidIndex(SkillIndex) == true, TEXT("잘못된 스킬 인덱스 범위"));
	UStaticSkillData* StaticSkillData = mSkillEntries[SkillIndex].mData;
	checkf(StaticSkillData != nullptr, TEXT("잘못된 스킬 데이터"));
	if (IsPlayerDisplacementSkill(StaticSkillData, GetOwnerModel<UUnitModel>()))
	{
		// 밀기/당기기는 클릭한 한 유닛만 조작한다. 원본 강타/검기 범위가 주변 유닛을
		// 함께 수집하면 화면에서 고른 대상과 실제 이동 대상이 달라질 수 있다.
		return MapModel->IsValidIndex(TargetIndex)
			? TArray<FTileIndex>({ TargetIndex })
			: TArray<FTileIndex>();
	}

	const EEffectPattern Pattern = StaticSkillData->mEffectPattern;
	const int32 EffectRange = StaticSkillData->mEffectAreaDefaultValue + DiceSum * StaticSkillData->mEffectAreaRatio;
	const bool IsPenetration = StaticSkillData->mIsPenetration;

	return MapModel->GetEffectTiles(GetOwnerModel<UBoardActorModel>()->GetTileTransform().mIndex, TargetIndex, Pattern, EffectRange, IsPenetration);
}
