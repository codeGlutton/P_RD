/*****************************************************************//**
 * @file   BoardMovementComponentModel.cpp
 * @brief  보드 액터 공용 이동 컴포넌트 모델 구현 파일
 * @author 이문환
 * @date   2026-08-09
 *********************************************************************/

#include "Component/BoardMovementComponent/BoardMovementComponentModel.h"

#include "Singleton/WorldSubsystem/SRPGCombatModel.h"
#include "Singleton/WorldSubsystem/PresentationBarrier.h"

#include "Actor/BoardActor/BoardActorModel.h"
#include "Actor/TileMap/TileMapModel.h"

bool UBoardMovementComponentModel::MoveAlongPath(const TArray<FTileIndex>& PathTileIndexes, FOnBoardMoveFinished OnFinished)
{
	// 이동 중 재호출이거나 경로가 2칸 미만이면 시작 거부
	if (IsMoving() == true || PathTileIndexes.Num() < 2)
	{
		return false;
	}

	mPathTileIndexes = PathTileIndexes;
	mOnFinished = OnFinished;
	mCancelRequested = false;

	// 코너링에 진입/진출 타일 정보가 필요하므로 전체 경로를 전달
	{
		UTileMapModel* TileMap = GetTileMap();
		TArray<FVector> PathWorldLocations;
		PathWorldLocations.Reserve(mPathTileIndexes.Num());
		for (const FTileIndex& TileIndex : mPathTileIndexes)
		{
			PathWorldLocations.Add(TileMap->TileToWorldLocation(TileIndex));
		}
		GetOwnerModel<UBoardActorModel>()->OnStartMovePath.Broadcast(PathWorldLocations);
	}

	// 1번 타일로 이동하는 것부터 시작 (0번은 현재 타일).
	// 해당 타일로 이동이 완료되면, 베리어가 끝나고 다시 다음 스텝으로 가는 걸 마지막 타일까지 반복.
	// 시뮬레이션모드에서는 베리어가 없으므로 즉각 다음 스텝으로 진행하므로 문제 없음
	StartStep(1);
	return true;
}

bool UBoardMovementComponentModel::IsMoving() const
{
	return mCurrentStepIndex != INDEX_NONE;
}

void UBoardMovementComponentModel::CancelMove()
{
	// 이동 중일 때만 의미 있음 - 실제 정지는 진행 중인 스텝의 연출 종료 시점
	if (IsMoving() == true)
	{
		mCancelRequested = true;
	}
}

void UBoardMovementComponentModel::StartStep(int32 StepIndex)
{
	checkf(mPathTileIndexes.IsValidIndex(StepIndex) == true, TEXT("이동 경로 인덱스 오류"));
	mCurrentStepIndex = StepIndex;

	// 파생 훅 (유닛의 AP 차감 등)
	OnStartStep(StepIndex);

	UBoardActorModel* Owner = GetOwnerModel<UBoardActorModel>();
	UTileMapModel* TileMap = GetTileMap();

	// 직전 타일에서 이번 타일을 바라볼때의 방향 계산
	// 직전->현재와 현재->다음 방향을 보간해서 자연스럽게 코너링 할 계획
	const ETileActorDirection Direction = UTileMapModel::TileDeltaToDirection(
		mPathTileIndexes[StepIndex - 1],
		mPathTileIndexes[StepIndex],
		Owner->GetTileTransform().mDirection);

	// 다음 타일로 이동 (모델의 논리적 위치 변경)
	// 점유는 즉시 하고, 도착 오버랩 통지는 CompleteStep가 함
	const FTileTransform NextTransform(mPathTileIndexes[StepIndex], Direction);
	TileMap->StartActorMovement(NextTransform, Owner);

	// 이동 후 받을 베리어 생성
	// 컴포넌트 모델이 먼저 파괴될 수 있으므로 WeakLambda로 보호.
	TSharedPtr<FPresentationBarrier> Barrier = FPresentationBarrier::Make(
		FOnFinishPresentation::CreateWeakLambda(this, [this]() {
			OnStepPresentationFinished();
			}));

	// 이번 스텝 도착 후 최종 목적지까지 남은 경로 거리 계산
	// -> 제동거리에 들어가면 감속할 때 사용
	float RemainingPathDistance = 0.0f;
	for (int32 i = StepIndex; i < mPathTileIndexes.Num() - 1; ++i)
	{
		RemainingPathDistance += FVector::Dist(
			TileMap->TileToWorldLocation(mPathTileIndexes[i]),
			TileMap->TileToWorldLocation(mPathTileIndexes[i + 1]));
	}

	// OnStartMoveStep을 구독하고 있던 뷰가 이동 시작 (뷰의 물리적 위치 변경)
	Owner->OnStartMoveStep.Broadcast(NextTransform, TileMap->TileToWorldTransform(NextTransform), Barrier, RemainingPathDistance);
}

void UBoardMovementComponentModel::CompleteStep()
{
	// 현재(도착) 타일의 오버랩 통지 — 함정/장판 등 타일 효과가 여기서 발동
	GetTileMap()->CompleteActorMovement(GetOwnerModel<UBoardActorModel>());
}

void UBoardMovementComponentModel::OnStepPresentationFinished()
{
	// 취소 요청됐으면 완료 통지 없이 정지
	if (mCancelRequested == true)
	{
		ResetMoveState();
		return;
	}

	// 현재 타일 도착 처리 -> 함정/장판 등 오버랩 관련된 처리
	CompleteStep();

	// OnEndMoveStep을 구독하고 있던 뷰가 도착 확인 (UI 변경 등)
	UBoardActorModel* Owner = GetOwnerModel<UBoardActorModel>();
	Owner->OnEndMoveStep.Broadcast(Owner->GetTileTransform(), Owner->GetWorldTransform());

	// 1) 마지막 타일이면 이동 완료. 상태를 먼저 비워서 완료 통지 안에서 새 이동을 시작할 수 있게 함
	if (mCurrentStepIndex >= mPathTileIndexes.Num() - 1)
	{
		FOnBoardMoveFinished Finished = mOnFinished;
		ResetMoveState();
		Finished.ExecuteIfBound();
	}
	// 2) 마지막 타일이 아니면 다음 타일로 이동
	else
	{
		StartStep(mCurrentStepIndex + 1);
	}
}

void UBoardMovementComponentModel::ResetMoveState()
{
	mPathTileIndexes.Empty();
	mCurrentStepIndex = INDEX_NONE;
	mCancelRequested = false;
	mOnFinished.Unbind();
}

UTileMapModel* UBoardMovementComponentModel::GetTileMap() const
{
	// 전투 모델을 월드 서브시스템에서 바로 받아 타일 맵을 꺼낸다 (턴 컨텍스트 체인 의존 제거)
	USRPGCombatModel* CombatModel = GetWorldSubsystemModel<USRPGCombatModel>(this);
	checkf(CombatModel != nullptr, TEXT("전투 모델 nullptr"));

	UTileMapModel* TileMap = CombatModel->GetTileMap();
	checkf(TileMap != nullptr, TEXT("타일 맵 nullptr"));
	return TileMap;
}
