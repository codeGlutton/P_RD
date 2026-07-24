#include "Component/SkillComponent/SkillComponentModel.h"

#include "Singleton/WorldSubsystem/PresentationBarrier.h"
#include "Singleton/WorldSubsystem/WorldCameraModel.h"

#include "Engine/AssetManager.h"
#include "DataAsset/SkillData/StaticSkillData.h"

#include "Pawn/UnitModel.h"
#include "Actor/BoardActor/BoardCombatTarget.h"

#include "Actor/TileMap/TileMapModel.h"

#include "Component/AttributeComponent/AttributeSetComponentModel.h"
#include "AttributeSet/UnitAttributeSet.h"
#include "TAS/Effect/TacticalEffectContext.h"
#include "TAS/Effect/Cooldown/TacticalEffect_Cooldown.h"
#include "TAS/Effect/Stat/TacticalEffect_Movement.h"

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
	mSelfTileIndex = FTileIndex::Invalid;
	mTargetTileIndex = FTileIndex::Invalid;
	mEffectTileIndexes.Reset();

	mSkillIndex = INDEX_NONE;
	mMotionIndex = INDEX_NONE;

	mEndCallback.Clear();

	mTargetTileIndexes.Reset();
	mOtherCombatTargets.Reset();

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
	return mData != nullptr && mData->mSkillMotionLayers.IsEmpty() == false;
}

USkillComponentModel::USkillComponentModel()
{
}

void USkillComponentModel::SetSkillFrom(const TArray<TSoftObjectPtr<UStaticSkillData>>& SkillList)
{
	// 초기화 로직 (몬스터는 6개 이상의 스킬도 소유할 수 있음)

	int32 SkillPoolSize = 4 /*추가 스킬*/ + 2 /*기본 스킬*/;
	if (SkillList.Num() > SkillPoolSize)
	{
		SkillPoolSize = SkillList.Num();
	}
	mSkillEntries.Init(FSkillEntry(), SkillPoolSize);

	int32 NextSkillIndex = 0;
	for (const TSoftObjectPtr<UStaticSkillData>& Skill : SkillList)
	{
		SetSkill(NextSkillIndex++, Skill.Get());
	}
}

void USkillComponentModel::SetSkillFrom(const TArray<FPrimaryAssetId>& SkillList)
{
	// 초기화 로직 (몬스터는 6개 이상의 스킬도 소유할 수 있음)

	int32 SkillPoolSize = 4 /*추가 스킬*/ + 2 /*기본 스킬*/;
	if (SkillList.Num() > SkillPoolSize)
	{
		SkillPoolSize = SkillList.Num();
	}
	mSkillEntries.Init(FSkillEntry(), SkillPoolSize);

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

bool USkillComponentModel::CanActiveSkill(int32 SkillIndex) const
{
	const bool HasEnoughMovement = HasRequiredMovement(SkillIndex);
	const bool IsWaitingCooldown = IsCooldown(SkillIndex);

	return HasEnoughMovement == true && IsWaitingCooldown == false;
}

void USkillComponentModel::ActivateSkill(UTileMapModel* MapModel, int32 SkillIndex, const FTileIndex& TargetIndex, FOnEndSkillUI Callback)
{
	checkf(mSkillEntries.IsValidIndex(SkillIndex) == true, TEXT("잘못된 사용 스킬 인덱스"));

	FSkillEntry& SkillEntry = mSkillEntries[SkillIndex];
	const UStaticSkillData* SkillData = SkillEntry.mData;

	UUnitModel* OwnerUnitModel = GetOwnerModel<UUnitModel>();
	checkf(OwnerUnitModel != nullptr, TEXT("스킬을 시전할 Owner가 유효하지 않음"));

	UAttributeSetComponentModel* AttributeSetCompModel = OwnerUnitModel->GetAttributeComponentModel();
	checkf(AttributeSetCompModel != nullptr, TEXT("속성 컴포넌트 nullptr"));

	UPassiveComponentModel* PassiveComponentModel = OwnerUnitModel->GetPassiveComponentModel();
	checkf(PassiveComponentModel != nullptr, TEXT("패시브 컴포넌트 nullptr"));

	UTacticalEffectContext* EffectContext = AttributeSetCompModel->MakeEffectContext();

	/* 쿨다운 처리 */
	
	{
		TSharedPtr<FTacticalEffectSpec> EffectSpec = AttributeSetCompModel->MakeOutgoingSpec(UTacticalEffect_Cooldown::StaticClass(), EffectContext);
		EffectSpec->mDynamicDurationMagnitude = GetStaticCooldownDuration(SkillIndex);
		SkillEntry.mCooldownHandle = AttributeSetCompModel->ApplyTacticalEffectSpecToSelf(*EffectSpec);
	}

	/* 행동력 소모 */

	{
		TSharedPtr<FTacticalEffectSpec> EffectSpec = AttributeSetCompModel->MakeOutgoingSpec(UTacticalEffect_Movement::StaticClass(), EffectContext);
		EffectSpec->mDynamicMagnitude = -GetRequiredMovement(SkillIndex);
		AttributeSetCompModel->ApplyTacticalEffectSpecToSelf(*EffectSpec);
	}

	/* 활성화 스킬 데이터 채우기 */

	{
		mActiveSkillContext.mMapModel = MapModel;
		mActiveSkillContext.mSelfTileIndex = OwnerUnitModel->GetTileTransform().mIndex;
		mActiveSkillContext.mTargetTileIndex = TargetIndex;
		mActiveSkillContext.mEffectTileIndexes = GetEffectTiles(MapModel, SkillIndex, TargetIndex);
		mActiveSkillContext.mSkillIndex = SkillIndex;
		mActiveSkillContext.mMotionIndex = 0;
		mActiveSkillContext.mEndCallback = MoveTemp(Callback);
	}

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

	PlayMotionLayer();
}

void USkillComponentModel::PlayMotionLayer()
{
	checkf(mSkillEntries.IsValidIndex(mActiveSkillContext.mSkillIndex) == true, TEXT("잘못된 사용 스킬 인덱스"));

	FSkillEntry& SkillEntry = mSkillEntries[mActiveSkillContext.mSkillIndex];
	const UStaticSkillData* SkillData = SkillEntry.mData;
	checkf(SkillEntry.IsValid() == true, TEXT("빈 스킬 시전 오류"));

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
	mActiveSkillContext.mMotionTileMapDir = mActiveSkillContext.mMapModel->TileDeltaToDirection(
		mActiveSkillContext.mSelfTileIndex,
		mActiveSkillContext.mTargetTileIndex,
		OwnerUnitModel->GetTileTransform().mDirection
	);
	mActiveSkillContext.mIsMotionTriggered = false;

	/* Effect 기본 값부터 참고용으로 적용 */

	for (const TInstancedStruct<FSkillEffectLayer>& EffectLayer : MotionLayer.mSkillEffectLayers)
	{
		EffectLayer.Get().ApplyPointEffect(OwnerCombatTarget);
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
	const UStaticSkillData* SkillData = SkillEntry.mData;
	checkf(SkillEntry.IsValid() == true, TEXT("빈 스킬 시전 오류"));

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
	const UStaticSkillData* SkillData = SkillEntry.mData;
	checkf(SkillEntry.IsValid() == true, TEXT("빈 스킬 시전 오류"));

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
			mActiveSkillContext.mTargetTileIndexes
		);
		for (int32 i = 0; i < EffectLayerNum; ++i)
		{
			const TInstancedStruct<FSkillEffectLayer>& EffectLayer = MotionLayer.mSkillEffectLayers[i];
			EffectLayer.Get().CommitEffect(Params);
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
}

void USkillComponentModel::EndMotionLayer()
{
	checkf(mSkillEntries.IsValidIndex(mActiveSkillContext.mSkillIndex) == true, TEXT("잘못된 사용 스킬 인덱스"));

	FSkillEntry& SkillEntry = mSkillEntries[mActiveSkillContext.mSkillIndex];
	const UStaticSkillData* SkillData = SkillEntry.mData;
	checkf(SkillEntry.IsValid() == true, TEXT("빈 스킬 시전 오류"));

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
	const UStaticSkillData* SkillData = SkillEntry.mData;
	checkf(SkillEntry.IsValid() == true, TEXT("빈 스킬 시전 오류"));

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

TArray<FTileIndex> USkillComponentModel::GetAimableTiles(UTileMapModel* MapModel, int32 SkillIndex) const
{
	checkf(mSkillEntries.IsValidIndex(SkillIndex) == true, TEXT("잘못된 스킬 인덱스 범위"));
	
	UStaticSkillData* StaticSkillData = mSkillEntries[SkillIndex].mData;
	checkf(StaticSkillData != nullptr, TEXT("잘못된 스킬 데이터"));

	const float AimRange = StaticSkillData->mAimRange;
	const EAimPattern Pattern = StaticSkillData->mAimPattern;
	const bool CanAimObstacle = StaticSkillData->mCanAimBoardActor;
	const bool IsIndirect = StaticSkillData->mIsIndirect;

	return MapModel->GetAimableTiles(GetOwnerModel<UBoardActorModel>()->GetTileTransform().mIndex, AimRange, Pattern, CanAimObstacle, IsIndirect);
}

TArray<FTileIndex> USkillComponentModel::GetEffectTiles(UTileMapModel* MapModel, int32 SkillIndex, const FTileIndex& TargetIndex) const
{
	checkf(mSkillEntries.IsValidIndex(SkillIndex) == true, TEXT("잘못된 스킬 인덱스 범위"));
	
	UStaticSkillData* StaticSkillData = mSkillEntries[SkillIndex].mData;
	checkf(StaticSkillData != nullptr, TEXT("잘못된 스킬 데이터"));

	const EEffectPattern Pattern = StaticSkillData->mEffectPattern;
	const int32 EffectRange = StaticSkillData->mEffectArea;
	const bool IsPenetration = StaticSkillData->mIsPenetration;

	return MapModel->GetEffectTiles(GetOwnerModel<UBoardActorModel>()->GetTileTransform().mIndex, TargetIndex, Pattern, EffectRange, IsPenetration);
}

bool USkillComponentModel::HasRequiredMovement(int32 SkillIndex) const
{
	const FSkillEntry* SkillEntry = GetSkill(SkillIndex);
	if (SkillEntry == nullptr || SkillEntry->IsValid() == false)
	{
		return false;
	}

	const UUnitModel* OwnerUnitModel = GetOwnerModel<UUnitModel>();
	if (OwnerUnitModel == nullptr)
	{
		return false;
	}

	UAttributeSetComponentModel* AttributeSetCompModel = OwnerUnitModel->GetAttributeComponentModel();
	if (AttributeSetCompModel == nullptr)
	{
		return false;
	}

	return SkillEntry->mData->mRequiredMovement <= AttributeSetCompModel->GetAttributeCurrentValue(UUnitAttributeSet::GetMovementAttribute());
}

int32 USkillComponentModel::GetRequiredMovement(int32 SkillIndex) const
{
	const FSkillEntry* SkillEntry = GetSkill(SkillIndex);
	if (SkillEntry == nullptr || SkillEntry->IsValid() == false)
	{
		return false;
	}

	return SkillEntry->mData->mRequiredMovement;
}

bool USkillComponentModel::IsCooldown(int32 SkillIndex) const
{
	const FSkillEntry* SkillEntry = GetSkill(SkillIndex);
	if (SkillEntry == nullptr || SkillEntry->IsValid() == false)
	{
		return false;
	}

	return SkillEntry->mCooldownHandle.IsValid() == true && SkillEntry->mCooldownHandle.GetOwningAttributeSetComponentModel() != nullptr;
}

ETacticalEffectDurationUnitType USkillComponentModel::GetCooldownUnit(int32 SkillIndex) const
{
	const FSkillEntry* SkillEntry = GetSkill(SkillIndex);
	if (SkillEntry == nullptr || SkillEntry->IsValid() == false)
	{
		return ETacticalEffectDurationUnitType::EveryTurn;
	}

	UClass* CooldownEffectClass = SkillEntry->mData->mCooldownEffectClass.Get();
	if (CooldownEffectClass == nullptr)
	{
		CooldownEffectClass = SkillEntry->mData->mCooldownEffectClass.LoadSynchronous();
	}
	if (CooldownEffectClass == nullptr)
	{
		return ETacticalEffectDurationUnitType::EveryTurn;
	}

	return GetDefault<UTacticalEffect>(CooldownEffectClass)->mDurationUnitPolicy;
}

int32 USkillComponentModel::GetStaticCooldownDuration(int32 SkillIndex) const
{
	const FSkillEntry* SkillEntry = GetSkill(SkillIndex);
	if (SkillEntry == nullptr || SkillEntry->IsValid() == false)
	{
		return INDEX_NONE;
	}

	return SkillEntry->mData->mCooldownDuration;
}

int32 USkillComponentModel::GetCooldownDuration(int32 SkillIndex) const
{
	const FSkillEntry* SkillEntry = GetSkill(SkillIndex);
	if (SkillEntry == nullptr || SkillEntry->IsValid() == false)
	{
		return INDEX_NONE;
	}

	const UUnitModel* OwnerUnitModel = GetOwnerModel<UUnitModel>();
	if (OwnerUnitModel == nullptr)
	{
		return INDEX_NONE;
	}

	UAttributeSetComponentModel* AttributeSetCompModel = OwnerUnitModel->GetAttributeComponentModel();
	if (AttributeSetCompModel == nullptr)
	{
		return INDEX_NONE;
	}
	return AttributeSetCompModel->GetActiveEffectsDuration(SkillEntry->mCooldownHandle);
}

int32 USkillComponentModel::GetRemainingCooldownTime(int32 SkillIndex) const
{
	const FSkillEntry* SkillEntry = GetSkill(SkillIndex);
	if (SkillEntry == nullptr || SkillEntry->IsValid() == false)
	{
		return INDEX_NONE;
	}

	const UUnitModel* OwnerUnitModel = GetOwnerModel<UUnitModel>();
	if (OwnerUnitModel == nullptr)
	{
		return INDEX_NONE;
	}

	UAttributeSetComponentModel* AttributeSetCompModel = OwnerUnitModel->GetAttributeComponentModel();
	if (AttributeSetCompModel == nullptr)
	{
		return INDEX_NONE;
	}
	return AttributeSetCompModel->GetActiveEffectsTimeRemaining(SkillEntry->mCooldownHandle);
}
