#pragma once

/**
 * @brief 전투 시뮬레이션 미리보기 전용 뷰모델. 실전(UCombatUIModel)과 저장 자리를 나눈다.
 *
 * @details
 * 예측은 실제와 다를 수 있고, 무르면(취소/새 조준) 실전 표시를 건드리지 않고 통째로
 * 버려져야 한다. 한 모델에 실전과 예측을 같이 담으면 "버린다"가 곧 실전 상태 오염이
 * 된다. 게임플레이 층이 USimulationSubsystem에서 mGameRoomContext/mSimulationRoomContext
 * 로 컨텍스트를 이원화한 것과 같은 이유다 — ClearPreview()는
 * mSimulationRoomContext.mRoomInstance = nullptr 과 같은 모양으로, 예측 쪽만 비운다.
 *
 * 세대(mGeneration)는 낡은 예측 결과가 새 예측 자리를 덮는 것을 막는다. BeginPreview()
 * 가 세대를 올려 돌려주고, Set*()는 그 세대가 아직 현재일 때만 반영한다.
 */

#include "RDMinimal.h"
#include "UI/Combat/CombatUITypes.h"

#include "SimulationPreviewUIModel.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSimulationPreviewChanged, ESimulationPreviewDomainUI, Domain);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSimulationPreviewEventBatch, FCombatEventBatchUI, Batch);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSimulationPreviewCleared);

/** @brief 시뮬레이션 미리보기 표시값의 단일 소스. UCombatUIModel이 하나 소유한다. */
UCLASS(BlueprintType)
class P_RD_API USimulationPreviewUIModel : public UObject
{
	GENERATED_BODY()

	/* ───────── 위젯이 구독하는 알림 ───────── */
public:
	/** @brief 미리보기 도메인 하나가 갱신됐음을 알림. 위젯은 자기 도메인만 다시 그린다. */
	UPROPERTY(BlueprintAssignable, Category = "Combat|SimulationPreview")
	FOnSimulationPreviewChanged OnPreviewChanged;

	/** @brief 미리보기 플로팅 로그 배치가 교체됐음을 알림. HUD가 미리보기 로그를 갈아 끼운다. */
	UPROPERTY(BlueprintAssignable, Category = "Combat|SimulationPreview")
	FOnSimulationPreviewEventBatch OnPreviewEventBatch;

	/** @brief 미리보기가 통째로 버려졌음을 알림. HUD는 미리보기 표시만 걷는다(실전은 그대로). */
	UPROPERTY(BlueprintAssignable, Category = "Combat|SimulationPreview")
	FOnSimulationPreviewCleared OnPreviewCleared;

	/* ───────── gameplay → UI : 표시값을 밀어넣는다 ───────── */
public:
	/**
	 * @brief 새 미리보기를 연다. 세대를 올리고 이전 payload를 비운다.
	 * @return 이번 미리보기의 세대. 이어지는 Set*() 호출에 그대로 넘긴다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Combat|SimulationPreview") int32 BeginPreview();

	/** @brief 미리보기 플로팅 로그 배치를 교체한다. Generation이 낡았으면 버린다. */
	UFUNCTION(BlueprintCallable, Category = "Combat|SimulationPreview")
	void SetPreviewEventBatch(int32 Generation, const TArray<FCombatFloatingLogRequest>& Requests);

	/** @brief 유닛별 예측 요약(HP 증감·사망·도착 타일)을 교체한다. Generation이 낡았으면 버린다. */
	UFUNCTION(BlueprintCallable, Category = "Combat|SimulationPreview")
	void SetPredictedUnits(int32 Generation, const TArray<FUnitPredictionUI>& Predictions);

	/** @brief 미리보기 중인 행동의 AP 예정 소모를 교체한다. Generation이 낡았으면 버린다. */
	UFUNCTION(BlueprintCallable, Category = "Combat|SimulationPreview")
	void SetPreviewPendingAction(int32 Generation, const FCombatPendingActionUI& PendingAction);

	/**
	 * @brief 미리보기를 통째로 버린다. 실전 모델은 어떤 것도 건드리지 않는다.
	 * @details 열려 있지 않으면 아무 일도 없다 — 취소 경로가 겹쳐 두 번 불려도 알림은 한 번만 나간다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Combat|SimulationPreview") void ClearPreview();

	/* ───────── 위젯이 읽는다 ───────── */
public:
	UFUNCTION(BlueprintPure, Category = "Combat|SimulationPreview") bool IsPreviewActive() const { return mIsActive; }
	UFUNCTION(BlueprintPure, Category = "Combat|SimulationPreview") int32 GetGeneration() const { return mGeneration; }
	UFUNCTION(BlueprintPure, Category = "Combat|SimulationPreview") const FCombatEventBatchUI& GetPreviewEventBatch() const { return mPreviewEventBatch; }
	UFUNCTION(BlueprintPure, Category = "Combat|SimulationPreview") const TArray<FUnitPredictionUI>& GetPredictedUnits() const { return mPredictedUnits; }
	UFUNCTION(BlueprintPure, Category = "Combat|SimulationPreview") const FCombatPendingActionUI& GetPreviewPendingAction() const { return mPreviewPendingAction; }

private:
	/** @brief payload만 초기값으로 되돌린다. 세대/리비전은 단조 증가라 건드리지 않는다. */
	void ResetPreviewPayloads();

	/** @brief 지금 미리보기가 열려 있는가. BeginPreview~ClearPreview 사이만 참. */
	UPROPERTY(Transient) bool mIsActive = false;
	/** @brief 미리보기 세대. Begin/Clear마다 올라 낡은 Set*() 결과를 걸러낸다. */
	UPROPERTY(Transient) int32 mGeneration = 0;
	/** @brief 미리보기 플로팅 로그 스냅샷. mSource는 항상 SimulationPreview다. */
	UPROPERTY(Transient) FCombatEventBatchUI mPreviewEventBatch;
	/** @brief 유닛별 예측 요약 스냅샷. */
	UPROPERTY(Transient) TArray<FUnitPredictionUI> mPredictedUnits;
	/** @brief 미리보기 중인 행동의 AP 예정 소모 스냅샷. */
	UPROPERTY(Transient) FCombatPendingActionUI mPreviewPendingAction;
};
