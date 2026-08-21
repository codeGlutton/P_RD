/*****************************************************************//**
 * @file   PuddleGimmickModel.h
 * @brief  장판 기믹 모델 정의 헤더
 * @author 이문환
 * @date   2026-08-21
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "Actor/BoardActor/Obstacle/Gimmick/OverlapGimmickModel.h"
#include "SRPGFramework/SRPGFrameworkType.h"
#include "PuddleGimmickModel.generated.h"

class USRPGCombatModel;

/**
 * @brief  장판 기믹 모델 (불장판, 독장판 등)
 * @details 진입 트리거 기믹(트랩)에 라운드 축을 더한 것:
 *          - 밟으면 즉시 발동 (부모 그대로)
 *          - 라운드가 끝날 때 위에 서 있는 유닛에게도 발동 (FPuddleRoundEndEvent가 호출)
 *          - 발동 횟수 대신 라운드 수로 수명 관리
 *          - 같은 타일에 새 장판이 오면 이 장판은 제거됨 (나중 장판이 덮어씀)
 */
UCLASS(Blueprintable)
class P_RD_API UPuddleGimmickModel : public UOverlapGimmickModel
{
	GENERATED_BODY()

public:
	// @brief 같은 Overlay 레이어 액터(새 장판)가 이 장판을 교체할 수 있게 허용
	UPuddleGimmickModel();

	/* UObstacleModel 상속 */
public:
	// @brief 스폰 데이터에서 라운드 수명을 읽고, 장판 일괄 발동 이벤트를 전투 모델에 등록
	void PostInitializeComponentModels() override;

	/* 라운드 끝 처리 */
public:
	/**
	 * @brief 라운드 끝 발동 처리 (FPuddleRoundEndEvent가 호출)
	 * @details 위에 발동 대상이 서 있으면 스킬을 시전하고, 라운드 수명을 차감.
	 *          수명을 다 쓰면 사망 태그를 붙여 기존 정리(ClearDeadActorModels)가 수거하게 함
	 * @param PresentationBarrier 스킬이 끝날 때까지 붙잡아 둘 배리어 (모든 장판이 공유 = 동시 연출)
	 */
	void TriggerRoundEnd(TSharedPtr<FPresentationBarrier> PresentationBarrier);

protected:
	// @brief 남은 라운드 수 (음수 = 무제한)
	UPROPERTY(Category = "Gimmick", VisibleInstanceOnly, BlueprintReadOnly, meta = (DisplayName = "RemainingRoundCount"))
	int32 mRemainingRoundCount = -1;
};

/**
 * @brief  장판 일괄 발동 이벤트
 * @details 라운드가 끝날 때 보드 위 모든 장판을 찾아 한꺼번에 발동시키는 관리자.
 *          전투당 1개만 등록됨 (장판마다 스폰 시 AddUniqueRoundEndEvent로 등록을 시도하고, 첫 시도만 등록됨).
 *          장판 목록을 따로 들고 있지 않고 매번 타일맵을 훑으므로,
 *          장판이 중간에 생기거나 없어져도 따로 관리하지 않아도 됨
 */
USTRUCT()
struct P_RD_API FPuddleRoundEndEvent : public FSRPGCombatRoundEvent
{
	GENERATED_BODY()

public:
	// @brief 전투 내내 반복 발동하도록 설정
	FPuddleRoundEndEvent();

private:
	// @brief 보드 위 모든 장판에게 라운드 끝 처리를 지시 (배리어 공유로 동시 연출)
	ESRPGCombatRoundEventResult Trigger_Internal(TSharedPtr<FPresentationBarrier> RoundBarrier, USRPGCombatModel* Model) override;
};
