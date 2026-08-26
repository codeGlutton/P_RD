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
	return StartPathInternal(PathTileIndexes, EBoardMoveMode::Normal, OnFinished);
}

bool UBoardMovementComponentModel::PushAlongPath(const TArray<FTileIndex>& PathTileIndexes, FOnBoardMoveFinished OnFinished)
{
	return StartPathInternal(PathTileIndexes, EBoardMoveMode::Push, OnFinished);
}

bool UBoardMovementComponentModel::PullAlongPath(const TArray<FTileIndex>& PathTileIndexes, FOnBoardMoveFinished OnFinished)
{
	return StartPathInternal(PathTileIndexes, EBoardMoveMode::Pull, OnFinished);
}

bool UBoardMovementComponentModel::TryRegisterPendingPush(const FTileIndex& TrapTileIndex, const TArray<FTileIndex>& PushPathTileIndexes)
{
	// 이동 루프 밖에서는 소비 지점이 없으므로 등록 불가 (정지 대상은 PushAlongPath 사용)
	if (IsMoving() == false)
	{
		return false;
	}

	// 한 연쇄 안에서 같은 함정은 1회만 발동 (무한 연쇄 방지)
	if (mChainedTrapTiles.Contains(TrapTileIndex) == true)
	{
		return false;
	}

	// 밀 곳이 없는 경로는 거부 (벽/장애물에 막히는 처리는 경로 생성 쪽 책임)
	if (PushPathTileIndexes.Num() < 2)
	{
		return false;
	}

	// 보류 경로의 시작은 피격자의 현재 타일이어야 함
	checkf(PushPathTileIndexes[0] == GetOwnerModel<UBoardActorModel>()->GetTileTransform().mIndex,
		TEXT("보류 밀치기 경로의 시작 타일 불일치"));

	// 한 타일에 함정이 여러개 있을 경우 마지막 등록이 덮어 씀
	mChainedTrapTiles.Add(TrapTileIndex);
	mPendingPushPath = PushPathTileIndexes;
	return true;
}

bool UBoardMovementComponentModel::StartPathInternal(const TArray<FTileIndex>& PathTileIndexes, EBoardMoveMode MoveMode, FOnBoardMoveFinished OnFinished)
{
	// 이동 중 재호출이거나 경로가 2칸 미만이면 시작 거부
	if (IsMoving() == true || PathTileIndexes.Num() < 2)
	{
		return false;
	}

	mPathTileIndexes = PathTileIndexes;
	mMoveMode = MoveMode;
	mOnFinished = OnFinished;
	mCancelRequested = false;

	// 외부 요청 시작 = 새 연쇄 시작 (연쇄 기록/보류 경로/깊이 초기화)
	mChainedTrapTiles.Empty();
	mPendingPushPath.Empty();
	mAdoptedPushCount = 0;

	// 전체 경로를 뷰에 통지
	BroadcastStartMovePath();

	// 1번 타일로 이동하는 것부터 시작 (0번은 현재 타일).
	// 해당 타일로 이동이 완료되면, 베리어가 끝나고 다시 다음 스텝으로 가는 걸 마지막 타일까지 반복.
	// 시뮬레이션모드에서는 베리어가 없으므로 즉각 다음 스텝으로 진행하므로 문제 없음
	StartStep(1);
	return true;
}

void UBoardMovementComponentModel::BroadcastStartMovePath()
{
	// 코너링에 진입/진출 타일 정보가 필요하므로 전체 경로를 전달
	UTileMapModel* TileMap = GetTileMap();
	TArray<FVector> PathWorldLocations;
	PathWorldLocations.Reserve(mPathTileIndexes.Num());
	for (const FTileIndex& TileIndex : mPathTileIndexes)
	{
		PathWorldLocations.Add(TileMap->TileToWorldLocation(TileIndex));
	}
	GetOwnerModel<UBoardActorModel>()->OnStartMovePath.Broadcast(PathWorldLocations, mMoveMode);
}

bool UBoardMovementComponentModel::IsMoving() const
{
	return mCurrentStepIndex != INDEX_NONE;
}

EBoardMoveMode UBoardMovementComponentModel::GetMoveMode() const
{
	return mMoveMode;
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
	OnStartStep(StepIndex, mMoveMode);

	UBoardActorModel* Owner = GetOwnerModel<UBoardActorModel>();
	UTileMapModel* TileMap = GetTileMap();

	// 직전 타일에서 이번 타일을 바라볼때의 방향 계산
	// 직전->현재와 현재->다음 방향을 보간해서 자연스럽게 코너링 할 계획
	// 밀치기는 밀려나는 것이므로 바라보는 방향 유지 (뒤로 밀려도 몸은 그대로)
	const ETileActorDirection Direction = (mMoveMode == EBoardMoveMode::Normal)
		? UTileMapModel::TileDeltaToDirection(
			mPathTileIndexes[StepIndex - 1],
			mPathTileIndexes[StepIndex],
			Owner->GetTileTransform().mDirection)
		: Owner->GetTileTransform().mDirection;

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
	Owner->OnStartMoveStep.Broadcast(NextTransform, TileMap->TileToWorldTransform(NextTransform), Barrier, RemainingPathDistance, mMoveMode);
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

	// 1) 보류 밀치기 경로가 있으면 남은 경로를 폐기하고 밀치기 경로로 교체
	//    mOnFinished는 유지해서 연쇄 전체가 끝날 때 1회 호출
	if (mPendingPushPath.Num() > 0)
	{
		// 함정은 연쇄당 1회만 발동하므로 정상 연쇄가 이 상한을 넘을 수 없음. 넘으면 연쇄 기록 로직 버그
		++mAdoptedPushCount;
		checkf(mAdoptedPushCount <= MaxPushChainDepth, TEXT("밀치기 연쇄 깊이 상한 초과"));

		mPathTileIndexes = MoveTemp(mPendingPushPath);
		mPendingPushPath.Empty();
		mMoveMode = EBoardMoveMode::Push;

		// 교체된 경로를 뷰에 다시 통지 (일반 이동 폴리라인 폐기, 밀치기 연출 준비)
		BroadcastStartMovePath();
		StartStep(1);
	}
	// 2) 마지막 타일이면 이동 완료. 상태를 먼저 비워서 완료 통지 안에서 새 이동을 시작할 수 있게 함
	else if (mCurrentStepIndex >= mPathTileIndexes.Num() - 1)
	{
		FOnBoardMoveFinished Finished = mOnFinished;
		ResetMoveState();
		Finished.ExecuteIfBound();
	}
	// 3) 마지막 타일이 아니면 다음 타일로 이동
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

	// 연쇄 상태 정리 (mMoveMode는 유지해서 GetMoveMode()가 마지막 모드 반환)
	mChainedTrapTiles.Empty();
	mPendingPushPath.Empty();
	mAdoptedPushCount = 0;
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
