#include "Actor/TileMap/TileMapModel.h"
#include "Actor/BoardActor/BoardActorModel.h"
#include "Singleton/WorldSubsystem/PresentationBarrier.h"
#include "Algo/Reverse.h"

namespace
{
	/**
	 * @brief 직교 4방향 단위 스텝 (오른쪽/왼쪽/아래/위)
	 */
	const FTileIndex Orthogonal4[] =
	{
		FTileIndex(1, 0), FTileIndex(-1, 0), FTileIndex(0, 1), FTileIndex(0, -1)
	};

	/**
	 * @brief 대각 4방향 단위 스텝 (우하/우상/좌하/좌상)
	 */
	const FTileIndex Diagonal4[] =
	{
		FTileIndex(1, 1), FTileIndex(1, -1), FTileIndex(-1, 1), FTileIndex(-1, -1)
	};

	/**
	 * @brief 경로 탐색용 4방향 이웃 스텝을 목표 방향 우선으로 정렬
	 * @details
	 * BFS에서 같은 길이의 최단경로가 여럿일 때, 먼저 닿은 칸이 부모가 되므로 이웃을 보는 순서가 경로 모양을 정한다.
	 * - 1·2순위: 목표 쪽 스텝. 남은 거리가 큰 축을 먼저 둬서 직선에 붙는 계단식 경로가 나오게 한다.
	 * - 3·4순위: 목표에서 멀어지는 스텝(장애물 우회용). 목표와 같은 줄인 축은 양방향 모두 우회 후보로 넣는다.
	 * 절대축(우/좌/하/상) 고정이 아니라 매 칸 목표 기준이라, 목표가 어느 쪽이든 "전진=목표 쪽"이 항상 1순위다.
	 * @param[in] Current 현재 칸
	 * @param[in] Goal    목표 칸
	 * @param[out] OutSteps 정렬된 스텝 목록 (항상 4방향 모두 포함, 호출 시 비우고 채움)
	 */
	void BuildGoalOrderedSteps(const FTileIndex& Current, const FTileIndex& Goal, TArray<FTileIndex>& OutSteps)
	{
		OutSteps.Reset();

		// 목표까지 각 축의 남은 변위
		const int32 DeltaX = Goal.mX - Current.mX;
		const int32 DeltaY = Goal.mY - Current.mY;

		// 목표 쪽 부호 (0이면 그 축은 목표와 같은 줄)
		const int32 SignX = (DeltaX > 0) - (DeltaX < 0);
		const int32 SignY = (DeltaY > 0) - (DeltaY < 0);

		// 남은 거리가 큰 축을 먼저 밟도록 우선순위 결정 (계단식 근접)
		const bool bXFirst = FMath::Abs(DeltaX) >= FMath::Abs(DeltaY);

		// 1·2순위: 목표 쪽 스텝 (해당 축이 0이 아닐 때만), 먼 축 먼저
		if (bXFirst)
		{
			if (SignX != 0) OutSteps.Add(FTileIndex(SignX, 0));
			if (SignY != 0) OutSteps.Add(FTileIndex(0, SignY));
		}
		else
		{
			if (SignY != 0) OutSteps.Add(FTileIndex(0, SignY));
			if (SignX != 0) OutSteps.Add(FTileIndex(SignX, 0));
		}

		// 3·4순위: 목표에서 멀어지는 스텝(우회용). 같은 줄(부호 0)인 축은 양방향 모두 후보
		if (SignX <= 0) OutSteps.AddUnique(FTileIndex(1, 0));
		if (SignX >= 0) OutSteps.AddUnique(FTileIndex(-1, 0));
		if (SignY <= 0) OutSteps.AddUnique(FTileIndex(0, 1));
		if (SignY >= 0) OutSteps.AddUnique(FTileIndex(0, -1));
	}
}

void UTileMapModel::Initialize()
{
	Super::Initialize();

	// 모델이 타일 저장소의 소유자이므로, 뷰 생성 여부와 무관하게 생성 시점에 직접 타일을 채운다.
	RebuildTiles();
}

int32 UTileMapModel::GetWidth() const
{
	return mWidth;
}

int32 UTileMapModel::GetHeight() const
{
	return mHeight;
}

bool UTileMapModel::IsValidIndex(const FTileIndex& TileIndex) const
{
	// 0 <= X < Width && 0 <= Y < Height 범위 검사
	return TileIndex.mX >= 0 && TileIndex.mX < mWidth
		&& TileIndex.mY >= 0 && TileIndex.mY < mHeight;
}

int32 UTileMapModel::TileIndexToLinearIndex(const FTileIndex& TileIndex) const
{
	// 범위 밖이면 무효
	if (!IsValidIndex(TileIndex))
	{
		return INDEX_NONE;
	}
	// 행 우선(y*Width + x)으로 1차원 인덱스 계산
	return TileIndex.mY * mWidth + TileIndex.mX;
}

const FTile* UTileMapModel::GetTile(const FTileIndex& TileIndex) const
{
	const int32 LinearIndex = TileIndexToLinearIndex(TileIndex);
	// 맵 범위(INDEX_NONE) + mTiles 미초기화/크기 불일치(IsValidIndex)를 모두 방어
	if (LinearIndex == INDEX_NONE || !mTiles.IsValidIndex(LinearIndex))
	{
		return nullptr;
	}
	return &mTiles[LinearIndex];
}

FTile* UTileMapModel::GetTile(const FTileIndex& TileIndex)
{
	// 로직 중복을 피하려 const 버전에 위임 후 반환값의 const만 제거
	return const_cast<FTile*>(AsConst(*this).GetTile(TileIndex));
}

void UTileMapModel::SetDimensions(int32 Width, int32 Height)
{
	// 1 미만 방지 후 멤버에 반영
	mWidth = FMath::Max(1, Width);
	mHeight = FMath::Max(1, Height);

	// 변경된 크기로 타일 저장소 재생성
	RebuildTiles();
}

void UTileMapModel::RebuildTiles()
{
	// 타일 저장소를 현재 크기에 맞춰 기본 타일로 새로 채움
	mTiles.Init(FTile(), FMath::Max(0, mWidth * mHeight));
}

ETileActorDirection UTileMapModel::TileDeltaToDirection(const FTileIndex& From, const FTileIndex& To, ETileActorDirection Fallback)
{
	const int32 DeltaX = To.mX - From.mX;
	const int32 DeltaY = To.mY - From.mY;

	// 제자리면 폴백 방향 리턴
	if (DeltaX == 0 && DeltaY == 0)
	{
		return Fallback;
	}

	// 델타값이 큰 쪽을 바라보게 되므로 그쪽으로 방향 결정.
	// X, Y축 델타가 같을때는 X 우선.
	if (FMath::Abs(DeltaX) >= FMath::Abs(DeltaY))
	{
		return DeltaX > 0 ? ETileActorDirection::Forward : ETileActorDirection::Backward;
	}
	return DeltaY > 0 ? ETileActorDirection::Right : ETileActorDirection::Left;
}

FTransform UTileMapModel::TileToWorldTransform(const FTileTransform& TileTransform) const
{
	// 뷰가 바인딩돼 있으면 뷰에 질의, 아니면(심 등) 항등 변환 (Invalid 값이 따로 없으니까)
	return
		mTileToWorldTransformDelegate.IsBound() ?
			mTileToWorldTransformDelegate.Execute(TileTransform) :
			FTransform::Identity;
}

FVector UTileMapModel::TileToWorldLocation(const FTileIndex& TileIndex) const
{
	// 뷰가 바인딩돼 있으면 뷰에 질의, 아니면(심 등) 원점
	return
		mTileToWorldLocationDelegate.IsBound() ?
			mTileToWorldLocationDelegate.Execute(TileIndex) :
			FVector::ZeroVector;
}

FTileIndex UTileMapModel::WorldToTileIndex(const FVector& WorldLocation) const
{
	// 뷰가 바인딩돼 있으면 뷰에 질의, 아니면(심 등) 무효 좌표
	return
		mWorldToTileIndexDelegate.IsBound() ?
			mWorldToTileIndexDelegate.Execute(WorldLocation) :
			FTileIndex::Invalid;
}

bool UTileMapModel::IsBlocked(const FTileIndex& TileIndex, const UBoardActorModel* Incoming) const
{
	const FTile* Tile = GetTile(TileIndex);
	// 맵 범위 밖은 블록된 것으로 간주
	if (Tile == nullptr)
	{
		return true;
	}

	// 진입 액터가 없으면 블록 없음으로 처리
	if (Incoming == nullptr)
	{
		return false;
	}

	// 기존 액터가 블록하는 레이어가 진입 액터의 레이어와 일치하면 블록
	for (const TWeakObjectPtr<UBoardActorModel>& Actor : Tile->mBoardActors)
	{
		if (Actor.IsValid() && EnumHasAnyFlags(Actor->GetBlockLayerFlags(), Incoming->GetTileLayerFlags()))
		{
			return true;
		}
	}

	return false;
}

TArray<UBoardActorModel*> UTileMapModel::GetReplaceableActors(const FTileIndex& TileIndex, const UBoardActorModel* Incoming) const
{
	TArray<UBoardActorModel*> Result;

	// 진입 액터가 없으면 교체 대상 없음
	if (Incoming == nullptr)
		return Result;

	// 타일 위 모든 액터를 받아서 교체 조건으로 거름
	for (UBoardActorModel* Actor : GetActorsOnTile(TileIndex, ETileLayerFlag::All))
	{
		if (Actor == nullptr)
			continue;

		// 기존 액터의 레이어가 진입 액터의 레이어와 다르면 제외
		if (!EnumHasAnyFlags(Actor->GetTileLayerFlags(), Incoming->GetTileLayerFlags()))
			continue;

		// 기존 액터가 진입 액터의 교체를 허용하지 않으면 제외
		if (!EnumHasAnyFlags(Actor->GetReplaceLayerFlags(), Incoming->GetTileLayerFlags()))
			continue;

		// 진입 액터의 우선순위가 낮으면 제외
		if (Incoming->GetOverlayLayerPriority() < Actor->GetOverlayLayerPriority())
			continue;

		// 모든 조건을 충족하면 교체대상
		Result.Add(Actor);
	}
	return Result;
}

void UTileMapModel::AppendRayTiles(const FTileIndex& Origin, const FTileIndex& Step, int32 Range, TArray<FTileIndex>& Out) const
{
	// 원점에서 Step 방향으로 한 칸씩 전진하며 수집 (원점 자신은 제외)
	FTileIndex Current = Origin;
	for (int32 Distance = 0; Distance < Range; ++Distance)
	{
		// 다음 칸으로 전진
		Current.mX += Step.mX;
		Current.mY += Step.mY;

		// 맵 밖으로 나가면 이 방향은 더 진행하지 않고 종료
		if (!IsValidIndex(Current))
			break;

		// 맵 안의 칸만 후보로 누적
		Out.Add(Current);
	}
}

void UTileMapModel::AppendBlockableRay(const FTileIndex& Origin, const FTileIndex& Step, int32 Range, bool bPenetrate, TArray<FTileIndex>& Out) const
{
	// 원점에서 Step 방향으로 한 칸씩 전진하며 수집 (원점 자신은 제외)
	FTileIndex Current = Origin;
	for (int32 Distance = 0; Distance < Range; ++Distance)
	{
		// 다음 칸으로 전진
		Current.mX += Step.mX;
		Current.mY += Step.mY;

		// 맵 밖으로 나가면 이 방향은 더 진행하지 않고 종료
		if (!IsValidIndex(Current))
			break;

		// 이 칸은 영향에 포함 (점유 칸이면 "맞고 멈춤"이라 포함 후 종료)
		Out.Add(Current);

		// 관통하지 않는데 점유 칸이면 그 너머로는 진행하지 않음
		if (!bPenetrate && IsOccupied(Current))
			break;
	}
}

bool UTileMapModel::IsOccupied(const FTileIndex& TileIndex, const UBoardActorModel* IgnoreBlocker) const
{
	// 장애물 또는 유닛이 있으면 점유로 판정
	for (const UBoardActorModel* Actor : GetActorsOnTile(TileIndex, ETileLayerFlag::Obstacle | ETileLayerFlag::Unit))
	{
		// 무시 대상(자리를 비울 예정인 유닛 등)은 점유로 치지 않음
		if (Actor != IgnoreBlocker)
			return true;
	}
	return false;
}

void UTileMapModel::AppendSquareTiles(const FTileIndex& Center, const int32 Radius, TArray<FTileIndex>& Out) const
{
	// 중심 기준 [-Radius, Radius] 정사각형 격자를 순회한다.
	for (int32 OffsetY = -Radius; OffsetY <= Radius; ++OffsetY)
	{
		for (int32 OffsetX = -Radius; OffsetX <= Radius; ++OffsetX)
		{
			// 중심 자신은 본체가 별도 처리하므로 제외 (중복 방지)
			if (OffsetX == 0 && OffsetY == 0)
				continue;

            // ReSharper disable once CppTooWideScopeInitStatement
            const FTileIndex Candidate(Center.mX + OffsetX, Center.mY + OffsetY);

			// 맵 안의 칸만 누적 (장애물/시야는 보지 않음 — 하늘 낙하 개념)
			if (IsValidIndex(Candidate))
				Out.Add(Candidate);
		}
	}
}

void UTileMapModel::BresenhamLine(const FTileIndex& From, const FTileIndex& To, TArray<FTileIndex>& Out) const
{
	// 동점(2*Error == ±Delta) 처리가 진행 방향에 따라 다른 칸을 고르지 않도록,
	// 항상 사전순(X 우선, 같으면 Y)으로 작은 쪽에서 큰 쪽으로만 그린다
	const bool bSwapped = (To.mX < From.mX) || (To.mX == From.mX && To.mY < From.mY);
	const FTileIndex& Start = bSwapped ? To : From;
	const FTileIndex& End = bSwapped ? From : To;

	// 이번 호출이 추가하는 구간의 시작 위치 (Out은 누적 배열이므로 이 구간만 뒤집어야 함)
	const int32 FirstIndex = Out.Num();

	// 시작점에서 끝점까지의 정수 좌표 (Start부터 한 칸씩 전진)
	int32 X = Start.mX;
	int32 Y = Start.mY;
	const int32 X1 = End.mX;
	const int32 Y1 = End.mY;

	// 각 축별로 전체이동량과 전진방향 결정
	const int32 DeltaX = FMath::Abs(X1 - X);
	const int32 DeltaY = FMath::Abs(Y1 - Y);
	const int32 StepX = (X < X1) ? 1 : -1;
	const int32 StepY = (Y < Y1) ? 1 : -1;

	// 직선에서 타일이 얼마나 멀리 있는 지 나타내는 오차 누적값 (dx - dy 기준)
	// 이 오차를 줄이는 방향, 즉 직선과 가까운 방향으로 움직이는 게 기본 아이디어
	// 단, dx - dy 이므로 x축으로 편향된 오차이므로, 오차가 클 수록 x축으로, 오차가 작을수록 y축으로 이동하는 압력이 세진다.
	int32 Error = DeltaX - DeltaY;

	while (true)
	{
		// 현재 칸 수집 (정규화된 방향이므로 Start가 첫 원소, End가 마지막 원소)
		Out.Add(FTileIndex(X, Y));

		// 끝점에 도달하면 종료
		if (X == X1 && Y == Y1)
			break;

		// 오차에 따라 x축/y축 전진 결정
		// 양쪽 동시에 넘으면 대각선 이동
		// @note 나눗셈 계산을 생략하기 위해 x2 값과 비교
		const int32 DoubleError = 2 * Error;

		// x축으로 이동하는 게 이동하지 않는 것보다 오차를 줄여주는 경우 -> x축으로 이동
		// @note 원래는 DeltaY의 절반을 기준으로 판단해야 하는데, 나눗셈을 피하기 위해 오차를 두 배 해서 비교한다.
		if (DoubleError > -DeltaY)
		{
			// x축으로 이동하는 게 y축 오차를 줄여주니까 dy만큼 오차 감소
			Error -= DeltaY;
			X += StepX;
		}
		// y축으로 이동하는 게 이동하지 않는 것보다 오차를 줄여주는 경우 -> y축으로 이동
		// @note 원래는 DeltaX의 절반을 기준으로 판단해야 하는데, 나눗셈을 피하기 위해 오차를 두 배 해서 비교한다.
		if (DoubleError < DeltaX)
		{
			// y축으로 이동하면 그 다음부터는 x축 이동 압력이 커질 수 있도록 Error 수치 조정
			Error += DeltaX;
			Y += StepY;
		}
	}

	// 뒤집어 그린 경우 이번에 추가한 구간만 반전해 "From이 첫 원소, To가 마지막 원소" 되도록 변경
	if (bSwapped)
		Algo::Reverse(MakeArrayView(Out.GetData() + FirstIndex, Out.Num() - FirstIndex));
}

void UTileMapModel::RasterizeLine(const FTileIndex& From, const FTileIndex& To, TArray<FTileIndex>& Out) const
{
	// 현재 래스터화 방식: Bresenham (Supercover 등으로 바꾸려면 이 호출만 교체)
	BresenhamLine(From, To, Out);
}

bool UTileMapModel::HasLineOfSight(const FTileIndex& From, const FTileIndex& To, const UBoardActorModel* IgnoreBlocker) const
{
	// From→To 직선이 지나는 칸들을 래스터화 (첫 원소=From, 마지막 원소=To 보장)
	TArray<FTileIndex> LineTiles;
	RasterizeLine(From, To, LineTiles);

	// 양 끝(From, To)을 제외한 중간 칸만 검사
	for (int32 Index = 1; Index < LineTiles.Num() - 1; ++Index)
	{
		// 중간 칸에 시야를 막는 액터(Obstacle 또는 Unit)가 있으면 시야가 막힘(=LoS:false)
		for (const UBoardActorModel* Actor : GetActorsOnTile(LineTiles[Index], ETileLayerFlag::Obstacle | ETileLayerFlag::Unit))
		{
			// 무시 대상(자리를 비울 예정인 유닛 등)은 차폐로 치지 않음
			if (Actor != IgnoreBlocker)
			{
				return false;
			}
		}
	}

	// 시야를 막는 액터가 없음 (=LoS:true)
	return true;
}

void UTileMapModel::RegisterActorToTile(FTile* Tile, UBoardActorModel* Actor)
{
	// 타일 액터 목록에 추가
	Tile->mBoardActors.Add(Actor);
}

void UTileMapModel::UnregisterActorFromTile(FTile* Tile, UBoardActorModel* Actor)
{
	// 타일 액터 목록에서 제거
	Tile->mBoardActors.Remove(Actor);
}

void UTileMapModel::NotifyBeginOverlap(FTile* Tile, UBoardActorModel* Actor)
{
	// 같은 타일의 다른 액터들과 양방향 OnBegin 통지 (자기 제외)
	for (const TWeakObjectPtr<UBoardActorModel>& Other : Tile->mBoardActors)
	{
		if (Other.Get() == Actor)
		{
			continue;
		}
		Actor->OnBeginTileOverlap(Tile, Other.Get());
		Other->OnBeginTileOverlap(Tile, Actor);
	}
}

void UTileMapModel::NotifyEndOverlap(FTile* Tile, UBoardActorModel* Actor)
{
	// 같은 타일의 다른 액터들과 양방향 OnEnd 통지 (자기 제외)
	for (const TWeakObjectPtr<UBoardActorModel>& Other : Tile->mBoardActors)
	{
		if (Other.Get() == Actor)
		{
			continue;
		}
		Actor->OnEndTileOverlap(Tile, Other.Get());
		Other->OnEndTileOverlap(Tile, Actor);
	}
}

/**
 * @details
 * - 이동범위는 맨해튼 이동방식 사용 (대각선 이동 없음)
 * - 시작점에서부터 4방향으로 이동할 수 있는 타일을 조사한 후 배열에 추가
 * - 배열에 있는 타일을 기준으로 이동할 수 있는 옆 타일을 조사
 * - 이미 선택된 타일의 인덱스를 Distance[] 배열에서 관리해서 중복 선택 방지
 * - 최대이동거리만큼 반복
 */
TArray<FTileIndex> UTileMapModel::GetReachableTiles(const FTileIndex& Origin, int32 MoveDistance) const
{
	TArray<FTileIndex> Result;

	// 시작점이 맵 밖이거나 이동력이 없으면 도달 가능 타일 없음
	if (!IsValidIndex(Origin) || MoveDistance <= 0)
		return Result;

	// 칸별 최단 도달 거리 기록 (-1=미방문), 1차원 인덱스로 접근
	TArray<int32> Distance;
	Distance.Init(-1, mWidth * mHeight);
	Distance[TileIndexToLinearIndex(Origin)] = 0;

	// BFS 큐: 가까운 칸부터 한 겹씩 확장 (인덱스로 순회해 pop 비용 회피)
	TArray<FTileIndex> Frontier;
	Frontier.Add(Origin);

	for (int32 Head = 0; Head < Frontier.Num(); ++Head)
	{
		const FTileIndex Current = Frontier[Head];
		const int32 CurrentDistance = Distance[TileIndexToLinearIndex(Current)];

		// 이동력을 모두 쓴 칸에서는 더 뻗지 않음
		if (CurrentDistance >= MoveDistance)
			continue;

		// 직교 4방향 이웃 검사
		for (const FTileIndex& Step : Orthogonal4)
		{
			const FTileIndex Next(Current.mX + Step.mX, Current.mY + Step.mY);

			// 맵 밖이면 제외 (INDEX_NONE)
			const int32 LinearIndex = TileIndexToLinearIndex(Next);
			if (LinearIndex == INDEX_NONE)
				continue;

			// 이미 방문한 칸은 건너뜀 (BFS라 먼저 방문한 경로가 최단)
			if (Distance[LinearIndex] != -1)
				continue;

			// 장애물·유닛이 점유한 칸은 통과·도착 불가
			// (확장 지점: 투명화 등 통과 규칙이 생기면 이 한 줄만 교체)
			if (IsOccupied(Next))
				continue;

			// 거리 확정 후 결과·큐에 추가
			Distance[LinearIndex] = CurrentDistance + 1;
			Result.Add(Next);
			Frontier.Add(Next);
		}
	}

	return Result;
}

TArray<int32> UTileMapModel::GetDistanceField(const FTileIndex& Target, const UBoardActorModel* IgnoreBlocker) const
{
	// 칸별 최단 경로 거리
	TArray<int32> Distance;
	Distance.Init(-1, mWidth * mHeight);

	// 목표가 맵 밖이면 도달 불가
	const int32 TargetLinear = TileIndexToLinearIndex(Target);
	if (TargetLinear == INDEX_NONE)
		return Distance;

	// 목표 칸 자신은 점유(유닛 등)여도 거리 0의 시작점으로 허용
	Distance[TargetLinear] = 0;

	// BFS 큐: 가까운 칸부터 한 겹씩 확장
	TArray<FTileIndex> Frontier;
	Frontier.Add(Target);

	for (int32 Head = 0; Head < Frontier.Num(); ++Head)
	{
		const FTileIndex Current = Frontier[Head];
		const int32 NextDistance = Distance[TileIndexToLinearIndex(Current)] + 1;

		// 직교 4방향 이웃 검사
		for (const FTileIndex& Step : Orthogonal4)
		{
			const FTileIndex Next(Current.mX + Step.mX, Current.mY + Step.mY);

			// 맵 밖이면 제외 (INDEX_NONE)
			const int32 LinearIndex = TileIndexToLinearIndex(Next);
			if (LinearIndex == INDEX_NONE)
				continue;

			// 이미 방문한 칸은 건너뜀 (BFS라 먼저 방문한 경로가 최단)
			if (Distance[LinearIndex] != -1)
				continue;

			// 장애물/유닛이 점유한 칸은 통과/도착 불가
			// 무시 대상 액터만 있으면 통과 가능
			if (IsOccupied(Next, IgnoreBlocker))
				continue;

			// 거리 확정 후 큐에 추가
			Distance[LinearIndex] = NextDistance;
			Frontier.Add(Next);
		}
	}

	return Distance;
}

/**
 * @details
 * - GetReachableTiles와 같은 4방향 BFS지만, 칸별 직전 칸(부모)을 기록해 목표 도달 후 경로를 거슬러 복원한다.
 * - BFS라 처음 닿은 경로가 곧 최단경로이므로, 목표를 만나면 즉시 탐색을 끝낸다.
 * - 이웃 탐색은 BuildGoalOrderedSteps로 목표 방향(먼 축 먼저)을 우선해, 빈 지형에선 직선에 붙는 계단식 경로가 나온다.
 */
TArray<FTileIndex> UTileMapModel::FindPath(const FTileIndex& Start, const FTileIndex& Goal) const
{
	TArray<FTileIndex> Result;

	// 양 끝 중 하나라도 맵 밖이면 경로 없음
	if (!IsValidIndex(Start) || !IsValidIndex(Goal))
		return Result;

	// 시작=목표면 그 한 칸이 곧 경로
	if (Start == Goal)
	{
		Result.Add(Start);
		return Result;
	}

	// 목표 칸이 점유돼 있으면 도착 불가 (GetReachableTiles와 동일 규칙)
	if (IsOccupied(Goal))
		return Result;

	// 칸별 직전 칸(부모)을 1차원 인덱스로 기록 — 방문표시(INDEX_NONE=미방문) + 경로 복원을 겸함
	TArray<int32> Parent;
	Parent.Init(INDEX_NONE, mWidth * mHeight);

	// 시작 칸은 자기 자신을 부모로 둬서 방문 표시 겸 복원 종료 기준으로 삼음
	const int32 StartLinear = TileIndexToLinearIndex(Start);
	Parent[StartLinear] = StartLinear;

	// BFS 큐: 가까운 칸부터 한 겹씩 확장 (인덱스 순회로 pop 비용 회피)
	TArray<FTileIndex> Frontier;
	Frontier.Add(Start);

	// 매 칸의 목표 방향 우선 정렬 스텝을 담을 버퍼 (루프 밖 선언해 재할당 회피)
	TArray<FTileIndex> OrderedSteps;

	// 목표 도달 여부 (도달 시 바깥 루프까지 즉시 종료)
	bool bReached = false;

	for (int32 Head = 0; Head < Frontier.Num() && !bReached; ++Head)
	{
		const FTileIndex Current = Frontier[Head];

		// 목표 쪽(먼 축 먼저) → 우회 순으로 4방향 스텝 정렬
		BuildGoalOrderedSteps(Current, Goal, OrderedSteps);

		for (const FTileIndex& Step : OrderedSteps)
		{
			const FTileIndex Next(Current.mX + Step.mX, Current.mY + Step.mY);

			// 맵 밖이면 제외 (INDEX_NONE)
			const int32 NextLinear = TileIndexToLinearIndex(Next);
			if (NextLinear == INDEX_NONE)
				continue;

			// 이미 방문한 칸은 건너뜀 (BFS라 먼저 방문한 경로가 최단)
			if (Parent[NextLinear] != INDEX_NONE)
				continue;

			// 장애물·유닛이 점유한 칸은 통과·도착 불가
			if (IsOccupied(Next))
				continue;

			// 직전 칸을 부모로 기록
			Parent[NextLinear] = TileIndexToLinearIndex(Current);

			// 목표에 닿았으면 더 넓힐 필요 없이 종료 (BFS라 이게 최단)
			if (Next == Goal)
			{
				bReached = true;
				break;
			}

			// 다음 겹 확장을 위해 큐에 추가
			Frontier.Add(Next);
		}
	}

	// 목표에 끝내 닿지 못했으면 경로 없음
	if (Parent[TileIndexToLinearIndex(Goal)] == INDEX_NONE)
		return Result;

	// 목표→시작으로 부모를 거슬러 올라가며 경로를 모음 (역순으로 쌓임)
	for (int32 Linear = TileIndexToLinearIndex(Goal); ; Linear = Parent[Linear])
	{
		// 1차원 인덱스를 (x,y)로 환원 (y*Width + x의 역변환)
		Result.Add(FTileIndex(Linear % mWidth, Linear / mWidth));

		// 시작 칸까지 모았으면 종료 (시작은 부모가 자기 자신)
		if (Linear == StartLinear)
			break;
	}

	// 시작→목표 순서가 되도록 뒤집기
	Algo::Reverse(Result);
	return Result;
}

void UTileMapModel::SetMovePath(const FTileIndex& Start, const FTileIndex& Goal)
{
	// 경로를 계산해 뷰에 표시 요청 (미바인딩=심 복제본이면 표시 없음)
	if (mSetMovePathDelegate.IsBound())
		mSetMovePathDelegate.Execute(FindPath(Start, Goal));
}

void UTileMapModel::ClearMovePath()
{
	// 빈 경로를 넘겨 표시 해제 (뷰의 SetMovePath가 빈 배열을 해제로 처리)
	if (mSetMovePathDelegate.IsBound())
		mSetMovePathDelegate.Execute(TArray<FTileIndex>());
}

void UTileMapModel::SetTileHighlight(const TArray<FTileIndex>& Tiles, ETileHighlightFlag Flag)
{
	// 강조 표시를 뷰에 요청 (미바인딩=심 복제본이면 표시 없음)
	if (mSetTileHighlightDelegate.IsBound())
		mSetTileHighlightDelegate.Execute(Tiles, Flag);
}

void UTileMapModel::ClearTileHighlight(ETileHighlightFlag Flag)
{
	// 강조 해제를 뷰에 요청 (미바인딩=심 복제본이면 표시 없음)
	if (mClearTileHighlightDelegate.IsBound())
		mClearTileHighlightDelegate.Execute(Flag);
}

/**
 * @brief
 * - 1단계에서는 패턴에 따라 후보타일을 수집하고
 * - 2단계에서는 각각의 후보타일에 대해서 장애물 막힘, 타겟 가능, 교체 가능 여부 검사해서 최종 판단
 */
TArray<FTileIndex> UTileMapModel::GetAimableTiles(const FTileIndex& Origin, int32 Range, EAimPattern Pattern, bool bIncludeOccupied, bool bIndirect, const UBoardActorModel* Incoming, const UBoardActorModel* IgnoreBlocker) const
{
	TArray<FTileIndex> Result;

	// 시작점이 맵 밖이면 조준 불가
	if (!IsValidIndex(Origin))
		return Result;

    /*
     * 페이즈1: 패턴에 따라 후보타일 수집
     */

	// Single은 기준 타일 한 칸만 (Range 무시). 그 외 패턴은 기준 타일 제외
	if (Pattern == EAimPattern::Single)
	{
		Result.Add(Origin);
	}
	else
	{
		// Single 제외한 다른 패턴은 사거리가 없으면 후보타일도 없음
		if (Range <= 0)
			return Result;

		// 패턴별 후보 타일 수집 (최종 필터링은 페이즈2에서 수행)
		switch (Pattern)
		{
		case EAimPattern::Cross:
			// 직교 4방향 직선
			for (const FTileIndex& Step : Orthogonal4)
				AppendRayTiles(Origin, Step, Range, Result);
			break;

		case EAimPattern::Star:
			// 직교 + 대각 8방향 직선
			for (const FTileIndex& Step : Orthogonal4)
				AppendRayTiles(Origin, Step, Range, Result);
			for (const FTileIndex& Step : Diagonal4)
				AppendRayTiles(Origin, Step, Range, Result);
			break;

		case EAimPattern::Square:
			// 중심 기준 사각형 범위 (하늘 낙하 개념 — 시야 무시)
			AppendSquareTiles(Origin, Range, Result);
			break;

		default:
			break;
		}
	}

    /**
     * 페이즈2: 각각의 타이레 대해서 장애물 막힘, 포함 여부, 교체 여부 판단해서 필터링
     */

	// 장애물 막힘 검사 여부: 직선패턴 AND 직사공격 (Square 공격은 하늘에서 내리는 공격이니까 장애물 무시)
	const bool bApplyLineOfSight = !bIndirect && (Pattern == EAimPattern::Cross || Pattern == EAimPattern::Star);

	// 후보를 시야/점유 조건으로 거름
    // @note 인덱스를 뒤에서 앞으로 오면서 제거하면 배열 재할당 이슈 없음
	for (int32 Index = Result.Num() - 1; Index >= 0; --Index)
	{
		const FTileIndex& Candidate = Result[Index];

		// 직사인데 시야가 막히면 조준 불가
		if (bApplyLineOfSight && !HasLineOfSight(Origin, Candidate, IgnoreBlocker))
		{
			Result.RemoveAt(Index);
			continue;
		}

		// 점유 타일을 포함하지 않으면 조준 불가
		if (!bIncludeOccupied && IsOccupied(Candidate))
		    // Incoming으로 교체 불가능하면 조준 불가
			if (!((Incoming != nullptr) && GetReplaceableActors(Candidate, Incoming).Num() > 0))
				Result.RemoveAt(Index);
	}

	return Result;
}

TArray<FTileIndex> UTileMapModel::GetEffectTiles(const FTileIndex& Caster, const FTileIndex& Target, EEffectPattern Pattern, int32 Size, bool bPenetrate) const
{
	TArray<FTileIndex> Result;

	// 영향 중심(Target)이 맵 밖이면 영향 없음
	if (!IsValidIndex(Target))
		return Result;

	// 영향 중심은 모든 패턴에서 항상 포함 (광역도 중심은 맞음)
	Result.Add(Target);

	// Single이거나 범위가 없으면 중심 한 칸만 영향
	if (Pattern == EEffectPattern::Single || Size <= 0)
		return Result;

	// 패턴별 영향 타일을 중심 둘레에 덧붙임 (직선은 관통 아니면 점유 칸에서 멈춤)
	switch (Pattern)
	{
	case EEffectPattern::Cross:
		// 중심에서 직교 4방향
		for (const FTileIndex& Step : Orthogonal4)
			AppendBlockableRay(Target, Step, Size, bPenetrate, Result);
		break;

	case EEffectPattern::Star:
		// 중심에서 직교 + 대각 8방향
		for (const FTileIndex& Step : Orthogonal4)
			AppendBlockableRay(Target, Step, Size, bPenetrate, Result);
		for (const FTileIndex& Step : Diagonal4)
			AppendBlockableRay(Target, Step, Size, bPenetrate, Result);
		break;

	case EEffectPattern::Square:
		// 중심 기준 사각형 범위 (하늘 낙하 개념 — 관통 무관)
		AppendSquareTiles(Target, Size, Result);
		break;

	case EEffectPattern::Beam:
		{
			// 시전자→타겟 방향을 부호로 단위 스텝화 (8방향 중에 하나로 강제 매핑)
			const FTileIndex Step(
				FMath::Sign(Target.mX - Caster.mX),
				FMath::Sign(Target.mY - Caster.mY)
			);

			// Target(클릭 지점)을 시작으로 그 방향으로 뻗음 — 빔 길이는 Target 포함 총 Size칸
			// (Target은 상단에서 이미 추가했으므로 너머로 Size-1칸만 더 뻗음)
			if (Step.mX != 0 || Step.mY != 0)
				AppendBlockableRay(Target, Step, Size - 1, bPenetrate, Result);
		}
		break;

	default:
		break;
	}

	return Result;
}

FTileIndex UTileMapModel::GetPushDestination(const FTileIndex& Pusher, const FTileIndex& Pushed, int32 MaxDistance) const
{
	// 밀리는 칸이 맵 밖이거나 밀칠 거리가 없으면 그대로 둔다
	if (!IsValidIndex(Pushed) || MaxDistance <= 0)
		return Pushed;

	// 미는 쪽→밀리는 쪽 방향을 각 축 부호로 8방향 단위 스텝화 (대각 포함)
	const FTileIndex Step(
		FMath::Sign(Pushed.mX - Pusher.mX),
		FMath::Sign(Pushed.mY - Pusher.mY)
	);

	// 같은 칸이라 방향이 없으면 밀 수 없음
	if (Step.mX == 0 && Step.mY == 0)
		return Pushed;

	// 밀리는 칸에서부터 한 칸씩 전진하며 멈출 지점을 찾음
	FTileIndex Current = Pushed;
	for (int32 Distance = 0; Distance < MaxDistance; ++Distance)
	{
		const FTileIndex Next(Current.mX + Step.mX, Current.mY + Step.mY);

		// 맵 밖으로는 밀리지 않음
		if (!IsValidIndex(Next))
			break;

		// 장애물/유닛에 걸리면 더 밀리지 않고 직전 칸에서 멈춤
		if (IsOccupied(Next))
			break;

		// 빈 칸이면 거기까지 밀려남
		Current = Next;
	}

	return Current;
}

TArray<UBoardActorModel*> UTileMapModel::GetActorsOnTile(const FTileIndex& TileIndex, ETileLayerFlag LayerFilter) const
{
	TArray<UBoardActorModel*> Result;

	// 타일 조회 (범위 밖이면 빈 배열)
	const FTile* Tile = GetTile(TileIndex);
	if (Tile == nullptr)
	{
		return Result;
	}

	// 레이어 필터에 걸리는 액터만 수집
	for (const TWeakObjectPtr<UBoardActorModel>& Actor : Tile->mBoardActors)
	{
		if (Actor.IsValid() && EnumHasAnyFlags(Actor->GetTileLayerFlags(), LayerFilter))
		{
			Result.Add(Actor.Get());
		}
	}
	return Result;
}

bool UTileMapModel::CanPlace(const FTileIndex& TileIndex, const UBoardActorModel* Incoming) const
{
	// 막히지 않으면 즉시 배치 가능
	if (IsBlocked(TileIndex, Incoming) == false)
		return true;

	// 막혔어도 막는 액터를 교체할 수 있으면 배치 가능
	return GetReplaceableActors(TileIndex, Incoming).Num() > 0;
}

void UTileMapModel::RotateActor(ETileActorDirection Direction, UBoardActorModel* Actor, TSharedPtr<FPresentationBarrier> Barrier)
{
	checkf(Actor != nullptr, TEXT("Actor가 nullptr"));

	// 같은 방향이면 바로 리턴
	FTileTransform NewTransform = Actor->GetTileTransform();
	if (NewTransform.mDirection == Direction)
	{
		return;
	}

	// 논리 방향 즉시 갱신
	NewTransform.mDirection = Direction;
	Actor->SetTileTransform(NewTransform);

	// 뷰에 회전 연출 요청
	Actor->OnRotate.Broadcast(TileToWorldTransform(NewTransform).Rotator(), Barrier);
}

void UTileMapModel::StartActorMovement(const FTileTransform& NextTransform, UBoardActorModel* Actor)
{
	checkf(Actor != nullptr, TEXT("Actor가 nullptr"));
	checkf(IsBlocked(NextTransform.mIndex, Actor) == false, TEXT("배치할 수 없는 타일"));

	// 이전 타일에서 이탈 오버랩 통지 후 해제 (좌표 전환 전에 현재 위치를 읽음)
	FTile* PrevTile = GetTile(Actor->GetTileTransform().mIndex);
	if (PrevTile != nullptr)
	{
		NotifyEndOverlap(PrevTile, Actor);
		UnregisterActorFromTile(PrevTile, Actor);
	}

	// 논리 좌표 전환 (점유는 즉시, 진입 오버랩 통지는 이동 연출 완료 후 CompleteActorMovement에서)
	Actor->SetTileTransform(NextTransform);
	FTile* NextTile = GetTile(NextTransform.mIndex);
	RegisterActorToTile(NextTile, Actor);
}

void UTileMapModel::CompleteActorMovement(UBoardActorModel* Actor)
{
	checkf(Actor != nullptr, TEXT("Actor가 nullptr"));

	// 이동 연출 완료 후 도착 타일에서 진입 오버랩 통지 (Start에서 좌표는 이미 전환됨)
	FTile* Tile = GetTile(Actor->GetTileTransform().mIndex);
	if (Tile != nullptr)
	{
		NotifyBeginOverlap(Tile, Actor);
	}
}

void UTileMapModel::PlaceActor(const FTileTransform& NextTransform, UBoardActorModel* Actor)
{
	checkf(Actor != nullptr, TEXT("Actor가 nullptr"));
	checkf(CanPlace(NextTransform.mIndex, Actor), TEXT("배치할 수 없는 타일"));

	FTile* Tile = GetTile(NextTransform.mIndex);

	// 교체 대상을 타일에서 밀어냄 (등록 해제 → 교체 통지 순서)
	for (UBoardActorModel* Victim : GetReplaceableActors(NextTransform.mIndex, Actor))
	{
		RemoveActor(Victim);
		Victim->OnReplaced(Tile, Actor);
	}

	// 논리 좌표 갱신
	Actor->SetTileTransform(NextTransform);
	Actor->OnPlaceTileTransform.Broadcast(NextTransform, TileToWorldTransform(NextTransform));

	// 다음 타일에 등록 후 진입 오버랩 통지 (등록 → Begin 순서)
	RegisterActorToTile(Tile, Actor);
	NotifyBeginOverlap(Tile, Actor);
}

void UTileMapModel::RemoveActor(UBoardActorModel* Actor)
{
	checkf(Actor != nullptr, TEXT("Actor가 nullptr"));

	// 현재 타일에서 이탈 오버랩 통지 후 해제 (End → 해제 순서)
	if (FTile* Tile = GetTile(Actor->GetTileTransform().mIndex))
	{
		NotifyEndOverlap(Tile, Actor);
		UnregisterActorFromTile(Tile, Actor);
	}

	// 논리 좌표 무효화
	Actor->SetTileTransform(FTileTransform::Invalid);
}

void UTileMapModel::RefreshActorPlacements()
{
	// 모든 타일의 보드 액터를 순회하며 현재 논리 좌표를 새 기준으로 변환해 브로드캐스팅
	for (const FTile& Tile : mTiles)
	{
		for (const TWeakObjectPtr<UBoardActorModel>& Actor : Tile.mBoardActors)
		{
			if (Actor.IsValid())
			{
				const FTileTransform& TileTransform = Actor->GetTileTransform();
				Actor->OnPlaceTileTransform.Broadcast(TileTransform, TileToWorldTransform(TileTransform));
			}
		}
	}
}
