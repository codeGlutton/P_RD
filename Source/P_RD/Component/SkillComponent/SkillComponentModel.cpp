#include "Component/SkillComponent/SkillComponentModel.h"

#include "Singleton/WorldSubsystem/PresentationBarrier.h"

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
	mDiceSum = 0;
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

void USkillComponentModel::ActivateSkill(UTileMapModel* MapModel, int32 SkillIndex, const FTileIndex& TargetIndex, int32 DiceSum, FOnEndSkill Callback)
{
	checkf(mSkillEntries.IsValidIndex(SkillIndex) == true, TEXT("잘못된 사용 스킬 인덱스"));

	UUnitModel* OwnerUnitModel = GetOwnerModel<UUnitModel>();
	checkf(OwnerUnitModel != nullptr, TEXT("스킬을 시전할 Owner가 유효하지 않음"));

	UPassiveComponentModel* PassiveComponentModel = OwnerUnitModel->GetPassiveComponentModel();
	checkf(PassiveComponentModel != nullptr, TEXT("패시브 컴포넌트 nullptr"));

	/* 활성화 스킬 데이터 채우기 */

	mActiveSkillContext.mDiceSum = DiceSum;
	mActiveSkillContext.mMapModel = MapModel;
	mActiveSkillContext.mSelfTileIndex = OwnerUnitModel->GetTileTransform().mIndex;
	mActiveSkillContext.mTargetTileIndex = TargetIndex;
	mActiveSkillContext.mEffectTileIndexes = GetEffectTiles(MapModel, SkillIndex, TargetIndex, DiceSum);
	mActiveSkillContext.mSkillIndex = SkillIndex;
	mActiveSkillContext.mMotionIndex = 0;
	mActiveSkillContext.mEndCallback = MoveTemp(Callback);

	/* 스킬 사용 시 패시브 발동 */

	TArray<UTacticalPassive*> Passives = PassiveComponentModel->GetPassivesByTiming(AbilityTags::GameplayAbility_Passive_OnStartUsingSkill);

	FBoardCombatTargetSnapshotData OwnerSnapshot = OwnerUnitModel->MakeSnapshotData();

	FPassiveActivateContext PassiveContext;
	PassiveContext.mOwner = OwnerUnitModel;
	PassiveContext.mOwnerSnapshot = &OwnerSnapshot;
	PassiveContext.mTargets.Add(OwnerUnitModel);
	PassiveContext.mTargetSnapshots.Add(&OwnerSnapshot);

	for (UTacticalPassive*& Passive : Passives)
	{
		TInstancedStruct<FDynamicPassiveData> DynamicPassiveData;
		Passive->ActivatePassive(AbilityTags::GameplayAbility_Passive_OnStartUsingSkill, PassiveContext, OUT DynamicPassiveData);
		Passive->CommitPassive(DynamicPassiveData);
	}

	/*
	 * 모션 레이어가 하나도 없는(미저작) 스킬은 시전을 무동작으로 즉시 종료한다.
	 * PlayMotionLayer/Trigger/EndMotionLayer가 전부 mSkillMotionLayers[mMotionIndex]를 인덱싱하므로,
	 * 빈 배열이면 [0] 접근에서 out-of-bounds 크래시가 난다. 실제 효과가 필요하면 DA에
	 * mSkillMotionLayers(+효과 레이어)를 채워야 한다.
	 */
	const UStaticSkillData* ActiveSkillData = mSkillEntries[SkillIndex].mData;
	if (ActiveSkillData == nullptr || ActiveSkillData->mSkillMotionLayers.Num() == 0)
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
		FBoardCombatTargetSnapshotData OwnerSnapshot = OwnerCombatTarget->MakeSnapshotData();

		TArray<UTacticalPassive*> Passives = PassiveComponentModel->GetPassivesByTiming(AbilityTags::GameplayAbility_Passive_OnStartApplyingEffect);

		FPassiveActivateContext PassiveContext;
		PassiveContext.mOwner = OwnerUnitModel;
		PassiveContext.mOwnerSnapshot = &OwnerSnapshot;

		TArray<FBoardCombatTargetSnapshotData> OtherSnapshots;
		OtherSnapshots.Reserve(mActiveSkillContext.mOtherCombatTargets.Num());
		for (IBoardCombatTarget* OtherCombatTarget : mActiveSkillContext.mOtherCombatTargets)
		{
			UBoardActorModel* OtherActorModel = Cast<UBoardActorModel>(OtherCombatTarget);
			checkf(OtherActorModel != nullptr, TEXT("스킬을 받는 타겟이 유효하지 않음"));
			OtherSnapshots.Add(OtherCombatTarget->MakeSnapshotData());

			PassiveContext.mTargets.Add(OtherActorModel);
			PassiveContext.mTargetSnapshots.Add(&OtherSnapshots.Last());
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
		FBoardCombatTargetSnapshotData OwnerSnapshot = OwnerCombatTarget->MakeSnapshotData();

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

			FBoardCombatTargetSnapshotData OtherSnapshot = OtherCombatTarget->MakeSnapshotData();

			FPassiveActivateContext PassiveContext;
			PassiveContext.mOwner = OtherActorModel;
			PassiveContext.mOwnerSnapshot = &OtherSnapshot;
			PassiveContext.mTargets.Add(OwnerUnitModel);
			PassiveContext.mTargetSnapshots.Add(&OwnerSnapshot);

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

	for (const TInstancedStruct<FSkillEffectLayer>& EffectLayer : MotionLayer.mSkillEffectLayers)
	{
		EffectLayer.Get().CommitEffect(OwnerCombatTarget, mActiveSkillContext.mTargetTileIndexes, mActiveSkillContext.mOtherCombatTargets, mActiveSkillContext.mDiceSum);
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
		FBoardCombatTargetSnapshotData OwnerSnapshot = OwnerCombatTarget->MakeSnapshotData();

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

			FBoardCombatTargetSnapshotData OtherSnapshot = OtherCombatTarget->MakeSnapshotData();

			FPassiveActivateContext PassiveContext;
			PassiveContext.mOwner = OtherActorModel;
			PassiveContext.mOwnerSnapshot = &OtherSnapshot;
			PassiveContext.mTargets.Add(OwnerUnitModel);
			PassiveContext.mTargetSnapshots.Add(&OwnerSnapshot);

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
		FBoardCombatTargetSnapshotData OwnerSnapshot = OwnerCombatTarget->MakeSnapshotData();

		TArray<UTacticalPassive*> Passives = PassiveComponentModel->GetPassivesByTiming(AbilityTags::GameplayAbility_Passive_OnEndApplyingEffect);

		FPassiveActivateContext PassiveContext;
		PassiveContext.mOwner = OwnerUnitModel;
		PassiveContext.mOwnerSnapshot = &OwnerSnapshot;

		TArray<FBoardCombatTargetSnapshotData> OtherSnapshots;
		OtherSnapshots.Reserve(mActiveSkillContext.mOtherCombatTargets.Num());
		for (IBoardCombatTarget* OtherCombatTarget : mActiveSkillContext.mOtherCombatTargets)
		{
			UBoardActorModel* OtherActorModel = Cast<UBoardActorModel>(OtherCombatTarget);
			checkf(OtherActorModel != nullptr, TEXT("스킬을 받는 타겟이 유효하지 않음"));
			OtherSnapshots.Add(OtherCombatTarget->MakeSnapshotData());

			PassiveContext.mTargets.Add(OtherActorModel);
			PassiveContext.mTargetSnapshots.Add(&OtherSnapshots.Last());
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

	FBoardCombatTargetSnapshotData OwnerSnapshot = OwnerUnitModel->MakeSnapshotData();

	FPassiveActivateContext PassiveContext;
	PassiveContext.mOwner = OwnerUnitModel;
	PassiveContext.mOwnerSnapshot = &OwnerSnapshot;
	PassiveContext.mTargets.Add(OwnerUnitModel);
	PassiveContext.mTargetSnapshots.Add(&OwnerSnapshot);

	for (UTacticalPassive*& Passive : Passives)
	{
		TInstancedStruct<FDynamicPassiveData> DynamicPassiveData;
		Passive->ActivatePassive(AbilityTags::GameplayAbility_Passive_OnEndUsingSkill, PassiveContext, OUT DynamicPassiveData);
		Passive->CommitPassive(DynamicPassiveData);
	}

	/* 스킬 종료 콜백 */

	mActiveSkillContext.mEndCallback.Broadcast(mActiveSkillContext.mSkillIndex, SkillData);

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

	const float AimRange = StaticSkillData->mAimRangeDefaultValue + DiceSum * StaticSkillData->mAimRangeRatio;
	const EAimPattern Pattern = StaticSkillData->mAimPattern;
	const bool CanAimObstacle = StaticSkillData->mCanAimBoardActor;
	const bool IsIndirect = StaticSkillData->mIsIndirect;

	return MapModel->GetAimableTiles(GetOwnerModel<UBoardActorModel>()->GetTileTransform().mIndex, AimRange, Pattern, CanAimObstacle, IsIndirect);
}

TArray<FTileIndex> USkillComponentModel::GetEffectTiles(UTileMapModel* MapModel, int32 SkillIndex, const FTileIndex& TargetIndex, int32 DiceSum) const
{
	TArray<FTileIndex> AimableTiles;
	checkf(mSkillEntries.IsValidIndex(SkillIndex) == true, TEXT("잘못된 스킬 인덱스 범위"));
	UStaticSkillData* StaticSkillData = mSkillEntries[SkillIndex].mData;
	checkf(StaticSkillData != nullptr, TEXT("잘못된 스킬 데이터"));

	const EEffectPattern Pattern = StaticSkillData->mEffectPattern;
	const int32 EffectRange = StaticSkillData->mEffectAreaDefaultValue + DiceSum * StaticSkillData->mEffectAreaRatio;
	const bool IsPenetration = StaticSkillData->mIsPenetration;

	return MapModel->GetEffectTiles(GetOwnerModel<UBoardActorModel>()->GetTileTransform().mIndex, TargetIndex, Pattern, EffectRange, IsPenetration);
}