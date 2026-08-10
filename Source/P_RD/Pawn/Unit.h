/*****************************************************************//**
 * @file   Unit.h
 * @brief  턴을 소유할 수 있는 베이스 폰 클래스 정의 파일
 * @author 모호재, 이문환
 * @date   2026-04-25
 *********************************************************************/

#pragma once

#include "RDMinimal.h"

#include "SRPGFramework/SRPGFrameworkType.h"
#include "Actor/ActorView.h"
#include "Actor/BoardActor/BoardCombatTargetView.h"
#include "Actor/BoardActor/BoardSelectionTargetView.h"
#include "GameFramework/Pawn.h"

#include "Unit.generated.h"

class UUnitModel;
struct FTileTransform;
struct FPresentationBarrier;

class USkeletalMeshComponent;
class UFloatingPawnMovement;
class UCapsuleComponent;
class UArrowComponent;
class USkeletonSkillAnimationComponent;

/**
 * @brief  턴을 소유할 수 있는 베이스 폰 클래스
 */
UCLASS(abstract)
class P_RD_API AUnit : public APawn, public IActorView, public IBoardCombatTargetView, public IBoardSelectionTargetView
{
	GENERATED_BODY()

public:
	AUnit();

	/* APawn 상속 */
public:
	// @brief 이동 연출
	// @note 플레이어 및 몹 모두 이동해야 하니까 베이스 클래스에서 구현
	void Tick(float DeltaSeconds) override;

	// @brief 이동 연출할 때 현재속도
	// @note
	// 현재는 이동을 무브먼트컴포넌트 없이 직접 하므로 자체 값을 반환
	// TODO: 나중에 무브먼트컴포넌트 사용하면 이 코드는 수정 요망
	FVector GetVelocity() const override;

	/* IActorView 상속 */
public:
	// @brief 이동 델리게이트 구독
	void BindModel(UObjectModel* Model) override;
	// @brief 이동 델리게이트 구독 해제
	void UnbindModel(UObjectModel* Model) override;

protected:
	UObjectModel* GetModel_Internal() const override;

	/* IBoardCombatTargetView 상속 */
public:
	USkillAnimationComponent* GetSkillAnimationComponent() const override;
	UCombatTargetVFXTimelineComponent* GetCombatTargetVFXTimelineComponent() const override;
	UPrimitiveComponent* GetTargetMeshComponent() const override;

public:
	// @brief 배치 요청을 수신
	virtual void OnPlaceTileTransform(const FTileTransform& TileTransform, const FTransform& Transform);

	/**
	 * @brief 이동 시작 시 전체 경로를 받아서, 코너링 곡선을 포함한 폴리라인 정보 생성
     * @details
     * - OnStartMoveStep()은 타일 하나씩 이동 가능
     * - 부드러운 코너링을 하려면 베지어곡선이 필요한데 이때는 진입, 중간, 진출 타일 정보가 필요함
     * - 전체 이동경로가 기존 타일 중점들의 집합 -> 베지어곡선 부분을 포함한 모든 점들의 집합(폴리라인)으로 변경
     */
	virtual void OnStartMovePath(const TArray<FVector>& PathWorldLocations);

	// @brief 이동 시작 요청을 수신해서 이동 시작
	virtual void OnStartMoveStep(
		const FTileTransform& NextTileTransform,
		const FTransform& TargetWorldTransform,
		TSharedPtr<FPresentationBarrier> Barrier,
		float RemainingPathDistance);

	// @brief 방향 전환 요청을 수신해서 제자리 회전 시작
	virtual void OnRotate(
		const FRotator& TargetWorldRotation,
		TSharedPtr<FPresentationBarrier> Barrier);

public:
	/**
	 * @brief 타일 중점 경로를, 베지어곡선 위의 점들(폴리라인)로 변환
	 * @details
	 * 모서리 타일의 중점을 기존에는 도착점으로 사용했지만,
	 * 베지어곡선변환에는 컨트롤 포인트로 사용해서 모서리에 곡선을 만들고,
	 * 경로 전체에 대해서 직선/곡선 모두를 포함한 점들의 집합(폴리라인)을 만듦.
	 * 이동할 때는 폴리라인의 점들 사이를 이동하는 게 됨
	 * @param PathPoints 경로에 있는 타일 중점들의 배열
	 * @param CornerCutRatio 코너 컷 비율. 0이면 직각, 커질수록 커브가 부드러워지며 최대는 0.5 (이 값을 넘어가면 연속 코너에서 커브가 서로 겹치므로)
	 * @param CornerTension 코너 텐션. 곡선을 코너 중점 쪽으로 당기는 정도 (0=대각선 직진, 1=표준 베지어, 2=코너 중점 통과)
	 * @param OutPoints 폴리라인을 구성하는 포인트 배열
	 * @param OutDistances 폴리라인의 각 점들의 누적 거리. 누적거리를 사용하는 이유는, 폴리라인의 점과 그 점이 어떤 타일에 있는지를 매핑하기 위함. 그래야 Tick()에서 타일간 이동이 발생했을 때 베리어를 시작하거나 종료할 수 있음
	 * @param OutStepMarkerDistances 타일순서와 이동거리를 매핑. 가령 이동거리=300 이면 어떤 타일이지? 같은 걸 매핑해놔야 도착하거나 벗어나는 타일을 알 수 있음
	 */
	static void BakePolyLinePoints(
		const TArray<FVector>& PathPoints,
		float CornerCutRatio,
		float CornerTension,
		TArray<FVector>& OutPoints,
		TArray<float>& OutDistances,
		TArray<float>& OutStepMarkerDistances);

private:
	// @brief 폴리라인이 있을 경우 폴리라인의 점들을 따라가는 이동 연출
	void TickPolyLine(float DeltaSeconds);
	// @brief 진행거리에 해당하는 폴리라인 점과 접선(바라보는 방향) 반환
	bool GetPolyLinePoint(float Distance, FVector& OutLocation, FVector& OutTangent) const;
	// @brief 폴리라인 이동상태 초기화 (직선 이동모드로 전환)
	void ResetPolyLineState();

public:
	UCapsuleComponent* GetCapsuleComponent() const;
	USkeletalMeshComponent* GetMesh() const;
	UFloatingPawnMovement* GetCharacterMovement() const;

	UArrowComponent* GetArrowComponent() const;

protected:
	// @brief 최대 이동 속도 (cm/초)
	UPROPERTY(Category = Move, EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "MaxMoveSpeed"))
	float mMaxMoveSpeed = 600.0f;

	// @brief 가속도 (cm/초^2). 출발할 때 속도를 올리는 데 사용
	UPROPERTY(Category = Move, EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Acceleration", ClampMin = "0.0"))
	float mAcceleration = 900.0f;

	// @brief 감속도 (cm/초^2). 도착할 때 속도를 내리는 데 사용. 낮을수록 제동거리가 길어져 부드럽게 멈춤
	UPROPERTY(Category = Move, EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Deceleration", ClampMin = "0.0"))
	float mDeceleration = 450.0f;

	// @brief 코너에서 바라보는 방향이 바뀌는 회전 속도 (도/초)
	UPROPERTY(Category = Move, EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "RotationSpeed"))
	float mRotationSpeed = 480.0f;

	/**
	 * @brief 코너 컷 비율 (코너 중점으로부터 타일 간격 대비 거리)
	 * @details
	 *   0: 직각
	 *   0.5: 부드러운 커브
	 * @note
	 *   0.5를 넘어가면 다른 타일을 침범하기 시작하므로 좀 더 복잡한 계산이 필요함
	 */
	UPROPERTY(Category = Move, EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "CornerCutRatio", ClampMin = "0.0", ClampMax = "0.5"))
	float mCornerCutRatio = 0.5f;

	/**
	 * @brief 코너 텐션 (곡선을 코너 중점 쪽으로 당기는 정도)
	 * @details
	 * 0: 대각선으로 바로 가로질러 감
	 * 1: 표준 베지어: 부드러운 곡선
	 * 2: 코너 중점에 가까워지므로 각진 코너링
	 */
	UPROPERTY(Category = Move, EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "CornerTension", ClampMin = "0.0", ClampMax = "2.0"))
	float mCornerTension = 0.5f;

private:
	// @brief 이번 이동의 목표 타일의 월드트랜스폼 (각 스텝마다 다음 타일이 목표 타일이 됨)
	FTransform mMoveTargetTransform = FTransform::Identity;
	// @brief 진행 중인 이동스텝의 연출 배리어
	TSharedPtr<FPresentationBarrier> mMoveBarrier;
	// @brief 현재 이동 속도 (cm/초).
	// @details
	// 가속/감속 계산에 쓰이는 스칼라 값.
	// 계산이 끝나면 mCurrentMoveVelocity에 벡터 값을 넣어줌
	float mCurrentMoveSpeed = 0.0f;
	// @brief 현재 이동 속도 벡터 (cm/초). GetVelocity()로 사용
	FVector mCurrentMoveVelocity = FVector::ZeroVector;
	// @brief 이번 스텝 목표 도착 후 최종 목적지까지 남은 경로 거리 (cm). 0이면 이번 목표가 최종 목적지
	float mRemainingPathDistance = 0.0f;

	// @brief 폴리라인 점 배열 (비어 있으면 직선 이동모드)
	TArray<FVector> mPolyLinePoints;
	// @brief 폴리라인 각 점까지의 누적 거리 (cm). mPolyLinePoints와 인덱스 1:1 대응
	TArray<float> mPolyLineDistances;
	// @brief 경로 순번별 도착 판정 거리 (cm). 진행거리가 이 값을 넘으면 해당 스텝의 배리어를 해제
	TArray<float> mStepMarkerDistances;
	// @brief 현재 진행 중인 경로 순번 (OnStartMoveStep 수신마다 증가)
	int32 mCurrentMoveStep = 0;
	// @brief 폴리라인 시작점부터 잰 현재 진행거리 (cm)
	float mPolyLineTraveledDistance = 0.0f;

private:
	UPROPERTY(Category = Unit, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "CapsuleComp", AllowPrivateAccess = "true"))
	TObjectPtr<UCapsuleComponent> mCapsuleComp;

	UPROPERTY(Category = Unit, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "MeshComp", AllowPrivateAccess = "true"))
	TObjectPtr<USkeletalMeshComponent> mMeshComp;

	UPROPERTY(Category = Unit, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "MovementComp", AllowPrivateAccess = "true"))
	TObjectPtr<UFloatingPawnMovement> mMovementComp;

	UPROPERTY(Category = Unit, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "ArrowComp", AllowPrivateAccess = "true"))
	TObjectPtr<UArrowComponent> mArrowComp;

	UPROPERTY(Category = Unit, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "SkillAnimationComp", AllowPrivateAccess = "true"))
	TObjectPtr<USkeletonSkillAnimationComponent> mSkillAnimationComp;
	UPROPERTY(Category = Unit, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "CombatTargetVFXTimelineComp", AllowPrivateAccess = "true"))
	TObjectPtr<UCombatTargetVFXTimelineComponent> mCombatTargetVFXTimelineComp;

protected:
	TWeakObjectPtr<UUnitModel> mUnitModel;
};
