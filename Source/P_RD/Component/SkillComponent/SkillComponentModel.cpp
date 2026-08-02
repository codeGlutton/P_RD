#include "Component/SkillComponent/SkillComponentModel.h"

#include "Singleton/WorldSubsystem/PresentationBarrier.h"

#include "Engine/AssetManager.h"
#include "DataAsset/SkillData/StaticSkillData.h"

#include "Actor/BoardActor/BoardCombatTarget.h"

#include "Actor/TileMap/TileMapModel.h"

#include "Component/AttributeComponent/AttributeSetComponentModel.h"
#include "AttributeSet/UnitAttributeSet.h"
#include "TAS/Effect/TacticalEffectContext.h"
#include "TAS/Effect/Cooldown/TacticalEffect_Cooldown.h"
#include "TAS/Effect/Stat/TacticalEffect_ActionPoint.h"

#include "Simulation/Logger/EventLog.h"
#include "Simulation/Logger/EventLogger.h"

#include "Animation/Notify/EventTriggerPayload.h"
#include "Animation/SkillAnimationMetaData.h"

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

FSkillEntry::FSkillEntry(UStaticSkillData* Data) : mData(Data)
{
}

bool FSkillEntry::IsValid() const
{
	return mData != nullptr && mData->mSkillPhaseLayers.IsEmpty() == false;
}

void FActiveSkillContext::Clear()
{
	mInstigator = nullptr;
	mMapModel = nullptr;
	mSelfTileIndex = FTileIndex::Invalid;
	mAimedTileIndex = FTileIndex::Invalid;
	mEffectTileIndexes.Reset();

	mMotionLocalDir = ETileActorDirection::Forward;
	mSkillEndBarrier = nullptr;

	mSkillIndex = INDEX_NONE;
	mAnimationIndex = INDEX_NONE;
	mPhaseIndex = INDEX_NONE;

	mEndCallback.Clear();

	mTargetTileIndexes.Reset();
	mOtherCombatTargets.Reset();
}

bool FActiveSkillContext::IsValid() const
{
	return mMapModel != nullptr;
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

	OnChangeSkillUI.Broadcast(SkillIndex, PreSkillData, SkillData);
}

bool USkillComponentModel::CanActiveSkill(int32 SkillIndex) const
{
	return CanActiveSkill_Internal(SkillIndex);
}

bool USkillComponentModel::ActivateSkill(UTileMapModel* MapModel, int32 SkillIndex, const FTileIndex& AimedTileIndex, FOnEndSkillUI Callback)
{
	if (CanActiveSkill_Internal(SkillIndex) == false)
	{
		return false;
	}

	ConsumeResources_Internal(SkillIndex);
	ActivateSkill_Internal(MapModel, SkillIndex, AimedTileIndex, Callback);
	return true;
}

void USkillComponentModel::ForcedActivateSkill(UTileMapModel* MapModel, int32 SkillIndex, const FTileIndex& AimedTileIndex, FOnEndSkillUI Callback)
{
	ConsumeResources_Internal(SkillIndex);
	ActivateSkill_Internal(MapModel, SkillIndex, AimedTileIndex, Callback);
}

bool USkillComponentModel::CanActiveSkill_Internal(int32 SkillIndex) const
{
	const bool IsWaitingCooldown = IsCooldown(SkillIndex);

	return IsWaitingCooldown == false;
}

void USkillComponentModel::ConsumeResources_Internal(int32 SkillIndex)
{
	checkf(mSkillEntries.IsValidIndex(SkillIndex) == true, TEXT("잘못된 사용 스킬 인덱스"));

	FSkillEntry& SkillEntry = mSkillEntries[SkillIndex];
	const UStaticSkillData* SkillData = SkillEntry.mData;

	IBoardCombatTarget* OwnerCombatTarget = GetOwnerModel<IBoardCombatTarget>();
	checkf(OwnerCombatTarget != nullptr, TEXT("스킬을 시전할 Owner가 유효하지 않음"));

	UAttributeSetComponentModel* AttributeSetCompModel = OwnerCombatTarget->GetAttributeComponentModel();
	checkf(AttributeSetCompModel != nullptr, TEXT("속성 컴포넌트 nullptr"));

	UTacticalEffectContext* EffectContext = AttributeSetCompModel->MakeEffectContext();

	/* 쿨다운 처리 */

	{
		TSharedPtr<FTacticalEffectSpec> EffectSpec = AttributeSetCompModel->MakeOutgoingSpec(UTacticalEffect_Cooldown::StaticClass(), EffectContext);
		EffectSpec->mDynamicDurationMagnitude = GetStaticCooldownDuration(SkillIndex);
		SkillEntry.mCooldownHandle = AttributeSetCompModel->ApplyTacticalEffectSpecToSelf(*EffectSpec);
	}
}

void USkillComponentModel::ActivateSkill_Internal(UTileMapModel* MapModel, int32 SkillIndex, const FTileIndex& AimedTileIndex, FOnEndSkillUI Callback)
{
	checkf(mSkillEntries.IsValidIndex(SkillIndex) == true, TEXT("잘못된 사용 스킬 인덱스"));

	FSkillEntry& SkillEntry = mSkillEntries[SkillIndex];
	const UStaticSkillData* SkillData = SkillEntry.mData;

	UBoardActorModel* OwnerBoardActorModel = GetOwnerModel<UBoardActorModel>();
	checkf(OwnerBoardActorModel != nullptr, TEXT("스킬을 시전할 Owner가 유효하지 않음"));

	IBoardCombatTarget* OwnerCombatTarget = GetOwnerModel<IBoardCombatTarget>();
	checkf(OwnerCombatTarget != nullptr, TEXT("스킬을 시전할 Owner가 유효하지 않음"));

	UAttributeSetComponentModel* AttributeSetCompModel = OwnerCombatTarget->GetAttributeComponentModel();
	checkf(AttributeSetCompModel != nullptr, TEXT("속성 컴포넌트 nullptr"));

	/* 활성화 스킬 데이터 채우기 */

	auto SkillEndBarrier = FPresentationBarrier::Make(FOnFinishPresentation::CreateWeakLambda(this, [this]() {
		DeactivateSkill();
		}));
	{
		mActiveSkillContext.mInstigator = OwnerBoardActorModel;
		mActiveSkillContext.mMapModel = MapModel;
		mActiveSkillContext.mSelfTileIndex = OwnerBoardActorModel->GetTileTransform().mIndex;
		mActiveSkillContext.mAimedTileIndex = AimedTileIndex;
		mActiveSkillContext.mEffectTileIndexes = GetEffectTiles(MapModel, SkillIndex, AimedTileIndex);
		mActiveSkillContext.mSkillEndBarrier = SkillEndBarrier;
		mActiveSkillContext.mSkillIndex = SkillIndex;
		mActiveSkillContext.mAnimationIndex = 0;
		mActiveSkillContext.mPhaseIndex = 0;
		mActiveSkillContext.mEndCallback = MoveTemp(Callback);
	}

	/* 스킬 실행 콜백 */

	OnPlaySkillUI.Broadcast(mActiveSkillContext, SkillData, SkillEndBarrier);
	OwnerCombatTarget->OnStartUsingSkill(mActiveSkillContext, SkillIndex);

	/* 스킬 페이즈 시작 */

	PreparePhaseLayer();

	/* 애니메이션 시작 */

	const ETileActorDirection MotionTileMapDir = MapModel->TileDeltaToDirection(
		mActiveSkillContext.mSelfTileIndex,
		mActiveSkillContext.mAimedTileIndex,
		OwnerBoardActorModel->GetTileTransform().mDirection
	);

	if (SkillData->mSkillAnimationSet.mAutoRotateTowardTarget == true)
	{
		// 자동 회전

		TSharedPtr<FPresentationBarrier> RotateBarrier = FPresentationBarrier::Make(
			FOnFinishPresentation::CreateWeakLambda(this, [this]() {
				// 회전 완료 후 로컬 정면방향으로 실행
				mActiveSkillContext.mMotionLocalDir = ETileActorDirection::Forward;
				PlaySkillAnimation();
				}));

		// 타일맵의 절대적 방향으로 회전
		mActiveSkillContext.mMapModel->RotateActor(MotionTileMapDir, OwnerBoardActorModel, RotateBarrier);
	}
	else
	{
		// 자동 회전 안 함

		mActiveSkillContext.mMotionLocalDir = TileMapToLocalDirection(MotionTileMapDir, OwnerBoardActorModel->GetTileTransform().mDirection);
		PlaySkillAnimation();
	}
}

void USkillComponentModel::PlaySkillAnimation()
{
	checkf(mSkillEntries.IsValidIndex(mActiveSkillContext.mSkillIndex) == true, TEXT("잘못된 사용 스킬 인덱스"));

	FSkillEntry& SkillEntry = mSkillEntries[mActiveSkillContext.mSkillIndex];
	const UStaticSkillData* SkillData = SkillEntry.mData;
	checkf(SkillEntry.IsValid() == true, TEXT("빈 스킬 시전 오류"));

	UBoardActorModel* OwnerBoardActorModel = GetOwnerModel<UBoardActorModel>();
	checkf(OwnerBoardActorModel != nullptr, TEXT("스킬을 시전할 Owner가 유효하지 않음"));

	const FSkillAnimationSet& SkillAnimationSet = SkillData->mSkillAnimationSet;
	checkf(SkillAnimationSet.mApplyMotionTags.IsValidIndex(mActiveSkillContext.mAnimationIndex) == true, TEXT("잘못된 실행 애님 인덱스"));

	/* 애니메이션 도중 실 타격 이벤트 콜백 등록 */

	FBoardActorAnimationEvent ApplyEvent;
	ApplyEvent.mIsOneTimeEvent = false;
	ApplyEvent.OnTriggerAnimationEvent.AddWeakLambda(this, [this](const FBoardActorAnimationContext& Context, UAnimMontage* EndAnim, const FEventTriggerPayload* Payload) {
		TriggerPhaseLayer(Payload);
		});
	const FGameplayTag& ApplyMotionTag = SkillAnimationSet.mApplyMotionTags[mActiveSkillContext.mAnimationIndex];
	FBoardActorAnimationContext Context(ApplyMotionTag, mActiveSkillContext.mMotionLocalDir);
	Context.mMetaData.InitializeAs<FSkillAnimationMetaData>();
	Context.mMetaData.GetMutable<FSkillAnimationMetaData>().mInstigator = this;
	Context.mMontageEvents.Add(AnimationTags::Animation_Event_Skill_HitLogic, ApplyEvent);

	/* 애니메이션 순차 재생 예약 */

	auto MotionEndBarrier = FPresentationBarrier::Make(FOnFinishPresentation::CreateWeakLambda(this, [this]() {
		EndSkillAnimation();
		}));

	/* 애니메이션 실행 */

	OwnerBoardActorModel->OnPlayAnimationUI.Broadcast(MotionEndBarrier, Context);
}

void USkillComponentModel::EndSkillAnimation()
{
	checkf(mSkillEntries.IsValidIndex(mActiveSkillContext.mSkillIndex) == true, TEXT("잘못된 사용 스킬 인덱스"));

	FSkillEntry& SkillEntry = mSkillEntries[mActiveSkillContext.mSkillIndex];
	const UStaticSkillData* SkillData = SkillEntry.mData;
	checkf(SkillEntry.IsValid() == true, TEXT("빈 스킬 시전 오류"));

	const FSkillAnimationSet& SkillAnimationSet = SkillData->mSkillAnimationSet;
	checkf(SkillAnimationSet.mApplyMotionTags.IsValidIndex(mActiveSkillContext.mAnimationIndex) == true, TEXT("잘못된 실행 애님 인덱스"));

	++mActiveSkillContext.mAnimationIndex;
	if (mActiveSkillContext.mAnimationIndex == SkillAnimationSet.mApplyMotionTags.Num())
	{
		/* 애니메이션에 의한 로직 대기 종료 */

		mActiveSkillContext.mSkillEndBarrier.Reset();
	}
	else
	{
		/* 다음 애니메이션 실행 */

		PlaySkillAnimation();
	}
}

void USkillComponentModel::PreparePhaseLayer()
{
	checkf(mSkillEntries.IsValidIndex(mActiveSkillContext.mSkillIndex) == true, TEXT("잘못된 사용 스킬 인덱스"));

	FSkillEntry& SkillEntry = mSkillEntries[mActiveSkillContext.mSkillIndex];
	const UStaticSkillData* SkillData = SkillEntry.mData;
	checkf(SkillEntry.IsValid() == true, TEXT("빈 스킬 시전 오류"));

	IBoardCombatTarget* OwnerCombatTarget = GetOwnerModel<IBoardCombatTarget>();
	checkf(OwnerCombatTarget != nullptr, TEXT("스킬을 시전할 Owner가 유효하지 않음"));

	const FSkillPhaseLayer& MotionLayer = SkillData->mSkillPhaseLayers[mActiveSkillContext.mPhaseIndex];

	/* 활성화 페이즈 데이터 채우기 */

	mActiveSkillContext.mTargetTileIndexes = MotionLayer.FilterTileIndexes(mActiveSkillContext.mSelfTileIndex, mActiveSkillContext.mEffectTileIndexes);
	mActiveSkillContext.mOtherCombatTargets = MotionLayer.FilterCombatTargets(mActiveSkillContext.mMapModel.Get(), OwnerCombatTarget, mActiveSkillContext.mTargetTileIndexes);

	/* 페이즈 시작 시 대리자 호출 */

	OnPlayPhaseLayerUI.Broadcast(mActiveSkillContext, mActiveSkillContext.mPhaseIndex);
}

void USkillComponentModel::TriggerPhaseLayer(const FEventTriggerPayload* Payload)
{
	checkf(mSkillEntries.IsValidIndex(mActiveSkillContext.mSkillIndex) == true, TEXT("잘못된 사용 스킬 인덱스"));

	FSkillEntry& SkillEntry = mSkillEntries[mActiveSkillContext.mSkillIndex];
	const UStaticSkillData* SkillData = SkillEntry.mData;
	checkf(SkillEntry.IsValid() == true, TEXT("빈 스킬 시전 오류"));

	UBoardActorModel* OwnerBoardActorModel = GetOwnerModel<UBoardActorModel>();
	checkf(OwnerBoardActorModel != nullptr, TEXT("스킬을 시전할 Owner가 유효하지 않음"));

	IBoardCombatTarget* OwnerCombatTarget = GetOwnerModel<IBoardCombatTarget>();
	checkf(OwnerCombatTarget != nullptr, TEXT("스킬을 시전할 Owner가 유효하지 않음"));

	const FSkillPhaseLayer& PhaseLayer = SkillData->mSkillPhaseLayers[mActiveSkillContext.mPhaseIndex];

	/* 모션 로그 시작 */

	GetWorldEventLogger(this)->BeginMotionLog();

	/* Effect 기본 값부터 적용 */

	struct FFactorEffectHandleContainer
	{
	public:
		FFactorEffectHandleContainer(TArray<FActiveTacticalEffectHandle> Handle) : mHandles(Handle)
		{
		}

	public:
		TArray<FActiveTacticalEffectHandle> mHandles;
	};

	TArray<FFactorEffectHandleContainer> FactorEffectHandleContainers;
	FactorEffectHandleContainers.Reserve(PhaseLayer.mSkillEffectLayers.Num());

	const int32 EffectLayerNum = PhaseLayer.mSkillEffectLayers.Num();
	for (int32 i = 0; i < EffectLayerNum; ++i)
	{
		const TInstancedStruct<FSkillEffectLayer>& EffectLayer = PhaseLayer.mSkillEffectLayers[i];
		FactorEffectHandleContainers.Add(EffectLayer.Get().ApplyFactorEffect(OwnerCombatTarget));
	}

	/* 이펙트 전 이벤트들 */

	OwnerCombatTarget->OnStartApplyingEffects(mActiveSkillContext, mActiveSkillContext.mPhaseIndex);
	{
		UBoardCombatTargetSnapshotData* OwnerSnapshot = OwnerCombatTarget->MakeSnapshotData();

		for (IBoardCombatTarget* OtherCombatTarget : mActiveSkillContext.mOtherCombatTargets)
		{
			OtherCombatTarget->OnStartReceivingEffects(OwnerSnapshot, mActiveSkillContext, mActiveSkillContext.mPhaseIndex);
		}
	}

	/* 실제 Effect 적용 */

	{
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
			OwnerBoardActorModel,
			OwnerSnapshot, 
			OtherCombatTargets, 
			OtherSnapshots, 
			mActiveSkillContext.mTargetTileIndexes
		);
		for (int32 i = 0; i < EffectLayerNum; ++i)
		{
			const TInstancedStruct<FSkillEffectLayer>& EffectLayer = PhaseLayer.mSkillEffectLayers[i];
			EffectLayer.Get().CommitEffect(Params);
		}
	}

	/* 이펙트 후 이벤트들 */

	{
		UBoardCombatTargetSnapshotData* OwnerSnapshot = OwnerCombatTarget->MakeSnapshotData();

		for (IBoardCombatTarget* OtherCombatTarget : mActiveSkillContext.mOtherCombatTargets)
		{
			OtherCombatTarget->OnEndReceivingEffects(OwnerSnapshot, mActiveSkillContext, mActiveSkillContext.mPhaseIndex);
		}
	}
	OwnerCombatTarget->OnEndApplyingEffects(mActiveSkillContext, mActiveSkillContext.mPhaseIndex);

	/* Effect 포인트 수치 비우기 */

	for (int32 i = 0; i < EffectLayerNum; ++i)
	{
		const TInstancedStruct<FSkillEffectLayer>& EffectLayer = PhaseLayer.mSkillEffectLayers[i];
		EffectLayer.Get().ClearFactorEffect(OwnerCombatTarget, FactorEffectHandleContainers[i].mHandles);
	}

	/* 모션 로그 종료 */

	GetWorldEventLogger(this)->EndMotionLog();

	/* 페이즈 시작 시 대리자 호출 */

	OnEndPhaseLayerUI.Broadcast(mActiveSkillContext.mPhaseIndex);

	/* 종료 판정 */

	++mActiveSkillContext.mPhaseIndex;
	if (mActiveSkillContext.mPhaseIndex < SkillData->mSkillPhaseLayers.Num())
	{
		/* 다음 스킬 준비 */

		PreparePhaseLayer();
	}
}

void USkillComponentModel::FlushRemainingPhaseLayers()
{
	FSkillEntry& SkillEntry = mSkillEntries[mActiveSkillContext.mSkillIndex];
	const UStaticSkillData* SkillData = SkillEntry.mData;
	checkf(SkillEntry.IsValid() == true, TEXT("빈 스킬 시전 오류"));

	/* 남은 스킬 페이즈 털어내기 */

	while (mActiveSkillContext.mPhaseIndex < SkillData->mSkillPhaseLayers.Num())
	{
		TriggerPhaseLayer(nullptr);
	}
}

void USkillComponentModel::DeactivateSkill()
{
	FSkillEntry& SkillEntry = mSkillEntries[mActiveSkillContext.mSkillIndex];
	const UStaticSkillData* SkillData = SkillEntry.mData;
	checkf(SkillEntry.IsValid() == true, TEXT("빈 스킬 시전 오류"));

	IBoardCombatTarget* OwnerCombatTarget = GetOwnerModel<IBoardCombatTarget>();
	checkf(OwnerCombatTarget != nullptr, TEXT("스킬을 시전할 Owner가 유효하지 않음"));

	/* 미호출한 Phase 단계들 처리 */

	FlushRemainingPhaseLayers();

	/* 스킬 종료 콜백 */

	OwnerCombatTarget->OnEndUsingSkill(mActiveSkillContext.mSkillIndex);
	mActiveSkillContext.mEndCallback.Broadcast(mActiveSkillContext, SkillData);
	OnEndSkillUI.Broadcast(mActiveSkillContext, SkillData);

	/* 활성화 스킬 데이터 비우기 */

	mActiveSkillContext.Clear();
}

bool USkillComponentModel::IsAnySkillActivated() const
{
	return mActiveSkillContext.IsValid() == true;
}

const FActiveSkillContext& USkillComponentModel::GetActiveSkillContext() const
{
	return mActiveSkillContext;
}

TArray<FTileIndex> USkillComponentModel::GetAimableTiles(UTileMapModel* MapModel, int32 SkillIndex) const
{
	checkf(mSkillEntries.IsValidIndex(SkillIndex) == true, TEXT("잘못된 스킬 인덱스 범위"));
	
	UStaticSkillData* StaticSkillData = mSkillEntries[SkillIndex].mData;
	checkf(StaticSkillData != nullptr, TEXT("잘못된 스킬 데이터"));

	const float AimRange = StaticSkillData->mAimRange;
	const EAimPattern Pattern = StaticSkillData->mAimPattern;
	const bool CanAimObstacle = StaticSkillData->mCanAimBoardActor;
	const ETileLayerFlag BlockerLayers = static_cast<ETileLayerFlag>(StaticSkillData->mAimBlockerMask);

	return MapModel->GetAimableTiles(GetOwnerModel<UBoardActorModel>()->GetTileTransform().mIndex, AimRange, Pattern, CanAimObstacle, BlockerLayers);
}

TArray<FTileIndex> USkillComponentModel::GetTargetTiles(UTileMapModel* MapModel, int32 SkillIndex, const FTileIndex& AimedTileIndex) const
{
	checkf(mSkillEntries.IsValidIndex(SkillIndex) == true, TEXT("잘못된 스킬 인덱스 범위"));
	UStaticSkillData* StaticSkillData = mSkillEntries[SkillIndex].mData;
	checkf(StaticSkillData != nullptr, TEXT("잘못된 스킬 데이터"));

	return MapModel->GetTargetTiles(GetOwnerModel<UBoardActorModel>()->GetTileTransform().mIndex, AimedTileIndex, StaticSkillData->mTargetPattern);
}

TArray<FTileIndex> USkillComponentModel::GetEffectTiles(UTileMapModel* MapModel, int32 SkillIndex, const FTileIndex& AimedTileIndex) const
{
	checkf(mSkillEntries.IsValidIndex(SkillIndex) == true, TEXT("잘못된 스킬 인덱스 범위"));
	
	UStaticSkillData* StaticSkillData = mSkillEntries[SkillIndex].mData;
	checkf(StaticSkillData != nullptr, TEXT("잘못된 스킬 데이터"));

	const EEffectPattern Pattern = StaticSkillData->mEffectPattern;
	const int32 EffectRange = StaticSkillData->mEffectArea;
	const ETileLayerFlag BlockerLayers = static_cast<ETileLayerFlag>(StaticSkillData->mEffectBlockerMask);

	// 타겟 패턴으로 영향 범위의 중심이 될 타일들을 수집
	const TArray<FTileIndex> TargetTiles = GetTargetTiles(MapModel, SkillIndex, AimedTileIndex);

	// 각 타겟 타일에서 영향 범위로 확산, 겹치는 타일은 한 번만 포함
	TArray<FTileIndex> EffectTiles;
	for (const FTileIndex& TargetTile : TargetTiles)
	{
		for (const FTileIndex& EffectTile : MapModel->GetEffectTiles(TargetTile, Pattern, EffectRange, BlockerLayers))
		{
			EffectTiles.AddUnique(EffectTile);
		}
	}

	return EffectTiles;
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

	const IBoardCombatTarget* OwnerCombatTarget = GetOwnerModel<IBoardCombatTarget>();
	if (OwnerCombatTarget == nullptr)
	{
		return INDEX_NONE;
	}

	UAttributeSetComponentModel* AttributeSetCompModel = OwnerCombatTarget->GetAttributeComponentModel();
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

	const IBoardCombatTarget* OwnerCombatTarget = GetOwnerModel<IBoardCombatTarget>();
	if (OwnerCombatTarget == nullptr)
	{
		return INDEX_NONE;
	}

	UAttributeSetComponentModel* AttributeSetCompModel = OwnerCombatTarget->GetAttributeComponentModel();
	if (AttributeSetCompModel == nullptr)
	{
		return INDEX_NONE;
	}
	return AttributeSetCompModel->GetActiveEffectsTimeRemaining(SkillEntry->mCooldownHandle);
}
