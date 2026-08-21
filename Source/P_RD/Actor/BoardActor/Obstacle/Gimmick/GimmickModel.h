/*****************************************************************//**
 * @file   GimmickModel.h
 * @brief  기믹 공통 모델 정의 헤더
 * @author 이문환
 * @date   2026-08-20
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "Actor/BoardActor/Obstacle/CombatTargetObstacleModel.h"
#include "GimmickModel.generated.h"

struct FActiveSkillContext;
class UStaticSkillData;
struct FPresentationBarrier;

/**
 * @brief  기믹 공통 모델
 * @details 모든 기믹이 공유하는 동작만 담음:
 *          발동되면 장착된 스킬을 시전하고, 정해진 횟수를 다 쓰면 보드에서 사라짐.
 *          "언제 발동하는가"(밟히면 / 부서지면 / 라운드마다)는 파생 클래스가 결정
 */
UCLASS(abstract)
class P_RD_API UGimmickModel : public UCombatTargetObstacleModel
{
	GENERATED_BODY()

	/* UObstacleModel 상속 */
public:
	// @brief 스폰 데이터에서 발동 설정(횟수, 스킬 슬롯)을 읽음
	void PostInitializeComponentModels() override;

	/* 발동 공통부 */
protected:
	/**
	 * @brief 기믹 발동 시도 (트리거를 감지한 파생 클래스가 호출)
	 * @details 장착된 스킬을 강제 시전하고 수명을 차감.
	 *          수명을 다 쓰면 사망 태그를 붙여 기존 정리(ClearDeadActorModels)가 수거하게 함
	 * @param AimedTileIndex 스킬 조준 타일
	 * @param PresentationBarrier 스킬이 끝날 때까지 붙잡아 둘 배리어 (없으면 발동만 하고 기다리지 않음)
	 * @return 발동 여부 (수명 소진, 사망, 시전 중이면 false)
	 */
	bool TryTriggerGimmick(const FTileIndex& AimedTileIndex, TSharedPtr<FPresentationBarrier> PresentationBarrier = nullptr);

protected:
	// @brief 타일맵 획득 (테스트용 파생 모델이 오버라이드 할 수 있게 가상함수로 선언)
	virtual UTileMapModel* GetTileMap() const;

private:
	// @brief 발동 스킬 종료 콜백 (수명을 다 썼으면 여기서 사망 처리)
	void OnGimmickSkillEnd(const FActiveSkillContext& Context, const UStaticSkillData* SkillData);

protected:
	// @brief 남은 발동 횟수 (음수 = 무제한)
	UPROPERTY(Category = "Gimmick", VisibleInstanceOnly, BlueprintReadOnly, meta = (DisplayName = "RemainingTriggerCount"))
	int32 mRemainingTriggerCount = -1;

	// @brief 발동 시 시전할 스킬 슬롯 인덱스
	UPROPERTY(Category = "Gimmick", VisibleInstanceOnly, BlueprintReadOnly, meta = (DisplayName = "TriggerSkillIndex"))
	int32 mTriggerSkillIndex = 0;
};
