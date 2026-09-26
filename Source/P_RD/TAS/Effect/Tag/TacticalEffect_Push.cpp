#include "TAS/Effect/Tag/TacticalEffect_Push.h"
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
	// @brief 밀치기 출발 (경로 계산 + 이동 시작 + 로그)
	// 광역 밀치기는 앞 대상이 밀린 뒤의 배치로 경로를 계산해야 하므로, 이펙트 적용 시점이 아니라 출발 차례가 왔을 때 실행
	void StartPush(TSharedPtr<FPresentationBarrier> MoveEndBarrier, UBoardActorModel* SourceModel, UBoardActorModel* TargetModel, UBoardMovementComponentModel* TargetMoveCompModel, UTileMapModel* TileMap, int32 PushDistance)
	{
		const FTileIndex SourceTileIndex = SourceModel->GetTileTransform().mIndex;
		const FTileIndex TargetTileIndex = TargetModel->GetTileTransform().mIndex;

		// 밀리는 경로 계산 (뒤가 막히면 막히기 직전까지로 짧아짐)
		// 시전자와 대상이 같은 타일이면 발판이므로 시전자→대상 방향을 구할 수 없어 발판의 고정 방향으로 밀고,
		// 다르면 유닛 스킬이므로 시전자에서 멀어지는 방향으로 민다
		const TArray<FTileIndex> PushPath = (SourceTileIndex == TargetTileIndex)
			? TileMap->GetPushPath(TargetTileIndex, SourceModel->GetTileTransform().mDirection, PushDistance)
			: TileMap->GetPushPath(SourceTileIndex, TargetTileIndex, PushDistance);

		const int32 PathNum = PushPath.Num();
		if (TargetMoveCompModel->IsMoving() == true)
		{
			// 이동 중인 대상: 등록만 하고, 이동 루프가 현재 스텝을 마무리하며 남은 경로를 밀치기 경로로 교체
			// 한 칸도 밀리지 못해도 등록해서 잔여 걷기를 끊음 (밀치기 함정을 밟으면 밀린 거리와 무관하게 이동 종료)
			TargetMoveCompModel->TryRegisterPendingPush(SourceTileIndex, PushPath);
		}
		else if (PathNum >= 2)
		{
			// 정지 상태 대상: 즉시 밀기 시작 (방 시작 시 발판 위 배치 발동 등). 한 칸도 밀리지 못하면 아무것도 안 함
			TargetMoveCompModel->PushAlongPath(PushPath, FOnBoardMoveFinished(), MoveEndBarrier);

			/* 로그 작성 */

			for (int32 PathIndex = 1; PathIndex < PathNum; ++PathIndex)
			{
				FSRPGTileEffectEventLog Log;
				Log.mOccupancyState = ESRPGTileOccupancyState::Move;
				Log.mPreTileIndex = PushPath[PathIndex - 1];
				Log.mNextTileIndex = PushPath[PathIndex];

				GetWorldEventLogger(TargetModel)->LogTileEffect(TargetModel->GetModelId(), TargetModel->GetClass(), Log);
			}
		}
	}
}

void UTacticalEffectExecutionCalculation_Push::Execute(const FTacticalEffectCustomExecutionParameters& ExecutionParams, FTacticalEffectCustomExecutionOutput& OutExecutionOutput) const
{
	Super::Execute(ExecutionParams, OutExecutionOutput);

	UAttributeSetComponentModel* SourceAttributeSetCompModel = ExecutionParams.GetSourceAttributeSetComponentModel();
	checkf(SourceAttributeSetCompModel != nullptr, TEXT("소스 컴포넌트 모델 nullptr"));

	UAttributeSetComponentModel* TargetAttributeSetCompModel = ExecutionParams.GetTargetAttributeSetComponentModel();
	checkf(TargetAttributeSetCompModel != nullptr, TEXT("타겟 컴포넌트 모델 nullptr"));

	UBoardActorModel* SourceModel = SourceAttributeSetCompModel->GetOwnerModel<UBoardActorModel>();
	checkf(SourceModel != nullptr, TEXT("밀치기 시전자가 보드 액터가 아님"));

	UBoardActorModel* TargetModel = TargetAttributeSetCompModel->GetOwnerModel<UBoardActorModel>();
	checkf(TargetModel != nullptr, TEXT("밀치기 대상자가 보드 액터가 아님"));

	IBoardCombatTarget* TargetCombatTarget = Cast<IBoardCombatTarget>(TargetModel);
	checkf(TargetCombatTarget != nullptr, TEXT("밀치기 대상자가 전투 대상이 아님"));

	UBoardMovementComponentModel* TargetMoveCompModel = TargetCombatTarget->GetBoardMovementComponentModel();
	checkf(TargetMoveCompModel != nullptr, TEXT("타겟 움직임 컴포넌트 모델 nullptr"));

	// 타일맵은 대상의 이동 컴포넌트에서 획득 (같은 보드 위이므로 동일한 맵)
	UTileMapModel* TileMap = TargetMoveCompModel->GetTileMap();
	checkf(TileMap != nullptr, TEXT("타일 맵 nullptr"));


	const int32 PushDistance = ExecutionParams.GetOwningSpec().GetStackCount();

	// 시전자 스킬이 페이즈 이펙트 적용 중이면 큐에 넣어 순차 출발, 아니면 그 자리에서 바로 출발
	IBoardCombatTarget* SourceCombatTarget = Cast<IBoardCombatTarget>(SourceModel);
	USkillComponentModel* SourceSkillComp = (SourceCombatTarget != nullptr) ? SourceCombatTarget->GetSkillComponentModel() : nullptr;
	if (SourceSkillComp == nullptr || SourceSkillComp->EnqueueForcedMove(FOnStartForcedMove::CreateStatic(&StartPush, SourceModel, TargetModel, TargetMoveCompModel, TileMap, PushDistance)) == false)
	{
		StartPush(nullptr, SourceModel, TargetModel, TargetMoveCompModel, TileMap, PushDistance);
	}

	OutExecutionOutput.MarkDynamicMagnitudeHandledManually();
	OutExecutionOutput.MarkStackCountHandledManually();
}

UTacticalEffect_Push::UTacticalEffect_Push()
{
	mCachedAssetTags.AddTag(EffectTags::GameplayEffect_StatusEffect_Instant_Debuff_Push);
	mCachedGrantedTags.AddTag(EffectTags::GameplayEffect_StatusEffect_Instant_Debuff_Push);

	FTacticalEffectExecutionDefinition Definition;
	Definition.mCalculationClass = UTacticalEffectExecutionCalculation_Push::StaticClass();
	mExecutions.Add(Definition);
}

UTacticalEffect_AddPush::UTacticalEffect_AddPush()
{
	mStatusEffect = UTacticalEffect_Push::StaticClass();
}

UTacticalEffect_GetPush::UTacticalEffect_GetPush()
{
	mStatusEffect = UTacticalEffect_Push::StaticClass();
}

bool UTacticalEffect_GetPush::CanApply(const FActiveTacticalEffectsContainer& ActiveTEContainer, const FTacticalEffectSpec& TESpec) const
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

