#include "TAS/Effect/Tag/TacticalEffect_Pull.h"
#include "GameplayTagType.h"

#include "Actor/TileMap/TileMapModel.h"
#include "Actor/BoardActor/BoardActorModel.h"
#include "Actor/BoardActor/BoardCombatTarget.h"

#include "Component/BoardMovementComponent/BoardMovementComponentModel.h"
#include "Component/AttributeComponent/AttributeSetComponentModel.h"

#include "Simulation/Logger/EventLogger.h"

void UTacticalEffectExecutionCalculation_Pull::Execute(const FTacticalEffectCustomExecutionParameters& ExecutionParams, FTacticalEffectCustomExecutionOutput& OutExecutionOutput) const
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


	//const FTileIndex SourceTileIndex = SourceModel->GetTileTransform().mIndex;
	//const ETileActorDirection PushDirection = SourceModel->GetTileTransform().mDirection;

	//// 밀리는 경로 계산 (앞이 막히면 막히기 직전까지로 짧아짐)
	//const TArray<FTileIndex> PushPath = TileMap->GetPushPath(TargetModel->GetTileTransform().mIndex, PushDirection, ExecutionParams.GetOwningSpec().GetStackCount());

	//// 한 칸도 당기지 못하면 아무것도 안 함
	//if (PushPath.Num() >= 2)
	//{
	//	if (TargetMoveCompModel->IsMoving() == true)
	//	{
	//		// 이동 중인 대상: 등록만 하고, 이동 루프가 현재 스텝을 마무리하며 남은 경로를 당기기 경로로 교체
	//		TargetMoveCompModel->TryRegisterPendingPull(SourceTileIndex, PushPath);
	//	}
	//	else
	//	{
	//		// 정지 상태 대상: 즉시 당기기 시작 (방 시작 시 발판 위 배치 발동 등)
	//		TargetMoveCompModel->PullAlongPath(PushPath);
	// 
	//		/* 로그 작성 */
	//		
	//		for (int32 PathIndex = 1; PathIndex < PathNum; ++PathIndex)
	//		{
	//			FSRPGTileEffectEventLog Log;
	//			Log.mOccupancyState = ESRPGTileOccupancyState::Move;
	//			Log.mPreTileIndex = PushPath[PathIndex - 1];
	//			Log.mNextTileIndex = PushPath[PathIndex];
	//		
	//			GetWorldEventLogger(TargetModel)->LogTileEffect(TargetModel->GetModelId(), TargetModel->GetClass(), Log);
	//		}
	//	}
	//}

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
