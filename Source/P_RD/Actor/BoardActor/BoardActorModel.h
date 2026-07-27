/*****************************************************************//**
 * @file   BoardActorModel.h
 * @brief  보드에 올라가는 액터 데이터 모델 클래스 정의 헤더
 * @author 이문환
 * @date   2026-06-18
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "Actor/ActorModel.h"
#include "SRPGFramework/SRPGFrameworkType.h"
#include "Actor/TileMap/TileLayer.h"
#include "BoardActorModel.generated.h"

struct FTile;
class UStaticObstacleSpawnData;
struct FPresentationBarrier;
struct FApplyEventTriggerPayload;

DECLARE_DELEGATE_RetVal(const FTransform&, FOnGetBoardActorWorldTransform);

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnPlaceTileTransform, const FTileTransform& /* TileTransform */, const FTransform& /* Transform */);

DECLARE_MULTICAST_DELEGATE_OneParam(FOnStartMovePath, const TArray<FVector>& /* PathWorldLocations */);

DECLARE_MULTICAST_DELEGATE_FourParams(FOnStartMoveStep, const FTileTransform& /* NextTileTransform */, const FTransform& /* TargetWorldTransform */, TSharedPtr<FPresentationBarrier> /* Barrier */, float /* RemainingPathDistance */);

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnRotate, const FRotator& /* TargetWorldRotation */, TSharedPtr<FPresentationBarrier> /* Barrier */);

DECLARE_DELEGATE_OneParam(FOnRequestReceiveAnimation, const FApplyEventTriggerPayload* /*Payload*/);
DECLARE_MULTICAST_DELEGATE_FourParams(FOnPlayApplyAnimationUI, TSharedPtr<FPresentationBarrier> /*MotionEndBarrier*/, FOnRequestReceiveAnimation /*TriggerCallback*/, FGameplayTag /*ApplyMotionTag*/, ETileActorDirection /*LocalDirection*/);
DECLARE_MULTICAST_DELEGATE_FourParams(FOnPlayReceiveAnimationUI, TSharedPtr<FPresentationBarrier> /*MotionEndBarrier*/, const FApplyEventTriggerPayload* /*Payload*/, FGameplayTag /*ReceiveMotionTag*/, ETileActorDirection /*LocalDirection*/);


/**
 * @brief  보드에 올라가는 액터 데이터 모델 클래스
 * @details 보드 위에 올라가는 액터(플레이어, 몬스터 등)의 데이터 모델 베이스 클래스다.
 */
UCLASS(Abstract)
class P_RD_API UBoardActorModel : public UActorModel
{
	GENERATED_BODY()

	// 타일맵 모델이 배치/이동 시 타일 트랜스폼·오버랩 콜백(protected)을 호출
	friend class UTileMapModel;
    // SRPG 전투 모델이 라운드 시작과 종료를 호출
    friend class USRPGCombatModel;

public:
	/**
	 * @brief 스폰 데이터 대입 함수
	 * @param StaticSpawnData 스폰 데이터
	 */
	void SetStaticSpawnData(UStaticObstacleSpawnData* StaticSpawnData);

public:
	FPrimaryAssetId GetStaticSpawnDataId() const;
	FName GetBoardActorKeyName() const;
	const FText& GetBoardActorDisplayName() const;
	virtual int32 GetBoardActorLevel() const;
	UTexture2D* GetBoardActorIcon() const;
	UTexture2D* GetBoardActorPortrait() const;

public:
	/**
	 * @brief 타일 트랜스폼 반환
	 * @return 타일 트랜스폼
	 */
	const FTransform& GetWorldTransform() const;

	/**
	 * @brief 타일 트랜스폼 반환
	 * @return 타일 트랜스폼
	 */
	const FTileTransform& GetTileTransform() const;

	/**
	 * @brief 액터의 레이어 타입을 반환
	 * @return 레이어 타입
	 */
	ETileLayerFlag GetTileLayerFlags() const;

	/**
	 * @brief 타일 배치 시에 블로킹할 타입들 반환
	 * @return 블로킹할 레이어 타입들
	 */
	ETileLayerFlag GetBlockLayerFlags() const;

	/**
	 * @brief 타일 배치 시에 교체할 타입들 반환
	 * @return 교체할 레이어 타입들
	 */
	ETileLayerFlag GetReplaceLayerFlags() const;

	/**
	 * @brief Overlay 레이어 내 교체 우선순위 반환
	 * @details 같은 교체 대상끼리 우열을 가린다. 진입자 우선순위가 같거나 더 높을 때만 기존 액터를 덮어쓴다.
	 * @return 우선순위 (높을수록 우선)
	 */
	int32 GetOverlayLayerPriority() const;

public:
	/**
	 * @brief 오버랩 시작 시 실행될 함수
	 * @param CurTile 현재 위치한 타일 객체
	 * @param Other 반대 대상
	 */
	virtual void OnBeginTileOverlap(FTile* CurTile, UBoardActorModel* Other);
	/**
	 * @brief 오버랩 종료 시 실행될 함수
	 * @param CurTile 현재 위치한 타일 객체
	 * @param Other 반대 대상
	 */
	virtual void OnEndTileOverlap(FTile* CurTile, UBoardActorModel* Other);

	/**
	 * @brief 다른 액터에게 교체되어 타일에서 밀려날 때 실행될 함수
	 * @param CurTile 현재 위치한 타일 객체
	 * @param Other 자신을 교체하고 들어오는 액터
	 */
	virtual void OnReplaced(FTile* CurTile, UBoardActorModel* Other);

	/**
	 * @brief 방 진입시마다 실행될 함수
	 */
	virtual void OnBeginRoom();
	virtual void OnEndRoom();

	/**
	 * @brief 라운드 시작마다 실행될 함수 (라운드 : 고정된 턴 기준으로 한바퀴)
	 */
	virtual void OnBeginRound(int32 RoundCount);
	virtual void OnEndRound(int32 RoundCount);

private:
	/**
	 * @brief 타일 트랜스폼 설정. (타일맵 외에는 지정할 수 없도록 private 제한)
	 * @param Transform 설정할 타일 트랜스폼
	 */
	void SetTileTransform(const FTileTransform& Transform);

public:
	/**
	 * @brief 뷰 좌표 질의 델리깃
	 */
	FOnGetBoardActorWorldTransform OnGetBoardActorWorldTransform;

	/**
	 * @brief 타일 트랜스폼이 즉시 변경되었을 때, 뷰에게 전달하는 대리자
	 */
	FOnPlaceTileTransform OnPlaceTileTransform;

	/**
	 * @brief 이동 시작 시 전체 경로 전달
	 * @details 코너링에 베지어곡선을 사용하려면 진입/진출 타일의 정보가 필요하므로 미리 전체 경로 정보 전달
	 */
	FOnStartMovePath OnStartMovePath;

	/**
	 * @brief 이동 스텝 시작 시 뷰에게 한 칸 이동 연출을 요청하는 대리자
	 * @details 뷰는 배리어를 잡고 애니메이션 연출, 끝나면 다음 스텝 진행을 알림.
	 *          시뮬레이션모드에서는 구독자가 없어서 배리어가 즉시 소멸하므로 로직만 동작.
	 */
	FOnStartMoveStep OnStartMoveStep;

	/**
	 * @brief 방향 전환 시 뷰에게 제자리 회전 연출을 요청하는 대리자
	 * @details 뷰는 배리어를 잡고 회전 보간, 끝나면 배리어를 놓아 완료를 알림.
	 *          시뮬레이션모드에서는 구독자가 없어서 배리어가 즉시 소멸하므로 로직만 동작.
	 */
	FOnRotate OnRotate;

	/**
	 * @brief 능동적 행동을 전달하는 대리자
	 */
	FOnPlayApplyAnimationUI OnPlayApplyAnimationUI;
	/**
	 * @brief 피동적 행동을 전달하는 대리자
	 */
	FOnPlayReceiveAnimationUI OnPlayReceiveAnimationUI;

protected:
	// @brief 액터가 속한 레이어 타입
	UPROPERTY(Category = "BoardActor", EditDefaultsOnly, BlueprintReadOnly, meta = (DisplayName = "TileLayerFlags", Bitmask, BitmaskEnum = "/Script/P_RD.ETileLayerFlag"))
	int32 mTileLayerFlags = static_cast<int32>(ETileLayerFlag::None);

	// @brief 타일 배치 시 블로킹할 레이어 타입들
	UPROPERTY(Category = "BoardActor", EditDefaultsOnly, BlueprintReadOnly, meta = (DisplayName = "BlockLayerFlags", Bitmask, BitmaskEnum = "/Script/P_RD.ETileLayerFlag"))
	int32 mBlockLayerFlags = static_cast<int32>(ETileLayerFlag::None);

	// @brief 타일 배치 시 교체할 레이어 타입들
	UPROPERTY(Category = "BoardActor", EditDefaultsOnly, BlueprintReadOnly, meta = (DisplayName = "ReplaceLayerFlags", Bitmask, BitmaskEnum = "/Script/P_RD.ETileLayerFlag"))
	int32 mReplaceLayerFlags = static_cast<int32>(ETileLayerFlag::None);

	// @brief Overlay 레이어 내 교체 우선순위 (높을수록 우선)
	UPROPERTY(Category = "BoardActor", EditDefaultsOnly, BlueprintReadOnly, meta = (DisplayName = "OverlayLayerPriority"))
	int32 mOverlayLayerPriority = 0;

private:
	// @brief 타일 트랜스폼(런타임 배치 상태)
	UPROPERTY(Category = "BoardActor", VisibleInstanceOnly, BlueprintReadOnly, meta = (DisplayName = "TileTransform", AllowPrivateAccess = "true"))
	FTileTransform mTileTransform = FTileTransform::Invalid;

protected:
	// @brief 초기 스폰 데이터
	UPROPERTY(Category = "Spawn", VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "StaticSpawnData"))
	TObjectPtr<UStaticObstacleSpawnData> mStaticSpawnData;
};
