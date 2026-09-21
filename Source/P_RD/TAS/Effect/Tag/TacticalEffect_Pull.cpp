#include "TAS/Effect/Tag/TacticalEffect_Pull.h"
#include "GameplayTagType.h"

#include "Actor/TileMap/TileMapModel.h"
#include "Actor/BoardActor/BoardActorModel.h"
#include "Actor/BoardActor/BoardCombatTarget.h"

#include "Component/BoardMovementComponent/BoardMovementComponentModel.h"
#include "Component/AttributeComponent/AttributeSetComponentModel.h"
#include "Component/SkillComponent/SkillComponentModel.h"

#include "Simulation/Logger/EventLogger.h"

namespace
{
	// @brief 당기기 출발 (경로 계산 + 이동 시작 + 로그)
	// 광역 당기기는 앞 대상이 당겨진 뒤의 배치로 경로를 계산해야 하므로, 이펙트 적용 시점이 아니라 출발 차례가 왔을 때 실행
	void StartPull(TSharedPtr<FPresentationBarrier> MoveEndBarrier, UBoardActorModel* SourceModel, UBoardActorModel* TargetModel, UBoardMovementComponentModel* TargetMoveCompModel, UTileMapModel* TileMap, int32 PullDistance)
	{
		// 당겨지는 경로 계산 (거리 소진, 시전자에 붙음, 중간 막힘 중 먼저 오는 곳까지로 짧아짐)
		const TArray<FTileIndex> PullPath = TileMap->GetPullPath(SourceModel->GetTileTransform().mIndex, TargetModel->GetTileTransform().mIndex, PullDistance);

		// 한 칸도 당기지 못하거나 대상이 이동 중이면 아무것도 안 함 (이동 중 당기기는 지원하지 않음)
		const int32 PathNum = PullPath.Num();
		if (PathNum >= 2 && TargetMoveCompModel->IsMoving() == false)
		{
			// 정지 상태 대상: 즉시 당기기 시작
			TargetMoveCompModel->PullAlongPath(PullPath, FOnBoardMoveFinished(), MoveEndBarrier);

			/* 로그 작성 */

			for (int32 PathIndex = 1; PathIndex < PathNum; ++PathIndex)
			{
				FSRPGTileEffectEventLog Log;
				Log.mOccupancyState = ESRPGTileOccupancyState::Move;
				Log.mPreTileIndex = PullPath[PathIndex - 1];
				Log.mNextTileIndex = PullPath[PathIndex];

				GetWorldEventLogger(TargetModel)->LogTileEffect(TargetModel->GetModelId(), TargetModel->GetClass(), Log);
			}
		}
	}
}

void UTacticalEffectExecutionCalculation_Pull::Execute(const FTacticalEffectCustomExecutionParameters& ExecutionParams, FTacticalEffectCustomExecutionOutput& OutExecutionOutput) const
{
	Super::Execute(ExecutionParams, OutExecutionOutput);

	UAttributeSetComponentModel* SourceAttributeSetCompModel = ExecutionParams.GetSourceAttributeSetComponentModel();
	checkf(SourceAttributeSetCompModel != nullptr, TEXT("소스 컴포넌트 모델 nullptr"));

	UAttributeSetComponentModel* TargetAttributeSetCompModel = ExecutionParams.GetTargetAttributeSetComponentModel();
	checkf(TargetAttributeSetCompModel != nullptr, TEXT("타겟 컴포넌트 모델 nullptr"));

	UBoardActorModel* SourceModel = SourceAttributeSetCompModel->GetOwnerModel<UBoardActorModel>();
	checkf(SourceModel != nullptr, TEXT("당기기 시전자가 보드 액터가 아님"));

	UBoardActorModel* TargetModel = TargetAttributeSetCompModel->GetOwnerModel<UBoardActorModel>();
	checkf(TargetModel != nullptr, TEXT("당기기 대상자가 보드 액터가 아님"));

	IBoardCombatTarget* TargetCombatTarget = Cast<IBoardCombatTarget>(TargetModel);
	checkf(TargetCombatTarget != nullptr, TEXT("당기기 대상자가 전투 대상이 아님"));

	UBoardMovementComponentModel* TargetMoveCompModel = TargetCombatTarget->GetBoardMovementComponentModel();
	checkf(TargetMoveCompModel != nullptr, TEXT("타겟 움직임 컴포넌트 모델 nullptr"));

	// 타일맵은 대상의 이동 컴포넌트에서 획득 (같은 보드 위이므로 동일한 맵)
	UTileMapModel* TileMap = TargetMoveCompModel->GetTileMap();
	checkf(TileMap != nullptr, TEXT("타일 맵 nullptr"));

	const int32 PullDistance = ExecutionParams.GetOwningSpec().GetStackCount();

	// 시전자 스킬이 페이즈 이펙트 적용 중이면 큐에 넣어 순차 출발, 아니면 그 자리에서 바로 출발
	IBoardCombatTarget* SourceCombatTarget = Cast<IBoardCombatTarget>(SourceModel);
	USkillComponentModel* SourceSkillComp = (SourceCombatTarget != nullptr) ? SourceCombatTarget->GetSkillComponentModel() : nullptr;
	if (SourceSkillComp == nullptr || SourceSkillComp->EnqueueForcedMove(FOnStartForcedMove::CreateStatic(&StartPull, SourceModel, TargetModel, TargetMoveCompModel, TileMap, PullDistance)) == false)
	{
		StartPull(nullptr, SourceModel, TargetModel, TargetMoveCompModel, TileMap, PullDistance);
	}

	OutExecutionOutput.MarkDynamicMagnitudeHandledManually();
	OutExecutionOutput.MarkStackCountHandledManually();
}

UTacticalEffect_Pull::UTacticalEffect_Pull()
{
	mCachedAssetTags.AddTag(EffectTags::GameplayEffect_StatusEffect_Instant_Debuff_Pull);
	mCachedGrantedTags.AddTag(EffectTags::GameplayEffect_StatusEffect_Instant_Debuff_Pull);

	FTacticalEffectExecutionDefinition Definition;
	Definition.mCalculationClass = UTacticalEffectExecutionCalculation_Pull::StaticClass();
	mExecutions.Add(Definition);
}

UTacticalEffect_AddPull::UTacticalEffect_AddPull()
{
	mStatusEffect = UTacticalEffect_Pull::StaticClass();
}

UTacticalEffect_GetPull::UTacticalEffect_GetPull()
{
	mStatusEffect = UTacticalEffect_Pull::StaticClass();
}

bool UTacticalEffect_GetPull::CanApply(const FActiveTacticalEffectsContainer& ActiveTEContainer, const FTacticalEffectSpec& TESpec) const
{
	if (Super::CanApply(ActiveTEContainer, TESpec) == false)
	{
		return false;
	}

	UAttributeSetComponentModel* AttributeSetCompModelInstance = ActiveTEContainer.mOwner.Get();
	if (AttributeSetCompModelInstance != nullptr && AttributeSetCompModelInstance->HasMatchingGameplayTag(EffectTags::GameplayEffect_StatusEffect_RoundDuration_Buff_ForcedMovementImmunity) == true)
	{
		return false;
	}

	return true;
}
