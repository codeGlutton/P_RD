/*****************************************************************//**
 * @file   EnemyUnitModel.h
 * @brief  적 베이스 유닛 모델 정의 헤더
 * @author 모호재, 이문환
 * @date   2026-05-15
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "Pawn/UnitModel.h"
#include "DataAsset/UnitSpawnData/StaticEnemyUnitSpawnData.h" // EMoveTendency
#include "EnemyUnitModel.generated.h"

class UEnemyUnitAttributeSet;

/** @brief 당기기 후 던지기 거리를 줄이고 착지 연출의 무게를 설명하는 3단계 체급. */
UENUM(BlueprintType)
enum class ESRPGDisplacementWeight : uint8
{
	Invalid = 0,
	Light = 1,
	Medium = 2,
	Heavy = 3,
};

/** @brief 목적지 선택을 서로 다르게 만드는 적 전장 이동 역할. */
UENUM(BlueprintType)
enum class ESRPGEnemyMovementRole : uint8
{
	Standard,
	Anchor,
	Flanker,
	Slider,
	Bulwark,
	Lancer,
	Bomber,
};

/**
 * @brief 적 베이스 유닛 모델
 */
UCLASS(abstract)
class P_RD_API UEnemyUnitModel : public UUnitModel
{
	GENERATED_BODY()

public:
	UEnemyUnitModel();

	/* UUnitModel 상속 */
public:
	void PostInitializeComponentModels() override;
	void OnBeginTurn() override;

public:
	int32 GetBoardActorLevel() const override;

public:
	/**
	 * @brief 스폰 전 난이도 대입 함수
	 * @param Difficulty 난이도
	 */
	void SetDifficulty(int32 Difficulty);

public:
	// @brief 난이도
	int32 GetDifficulty() const override;
	// @brief 플레이어유닛 여부
	bool IsPlayerUnitModel() const override { return false; }

	// @brief 이동 성향
	EMoveTendency GetMoveTendency() const;
	/** @brief 첫 전투 적의 체형을 안정적으로 분류한다. 데이터 미지정 적은 중형으로 취급한다. */
	ESRPGDisplacementWeight GetDisplacementWeight() const;
	/** @brief Mushroom/Spider/Slime의 전투 실루엣을 목적지 점수에 연결한다. */
	ESRPGEnemyMovementRole GetMovementRole() const;
	/** @brief 생존전 증원에 기존 메시와 별개의 전투 역할을 부여한다. */
	void SetMovementRoleOverride(ESRPGEnemyMovementRole Role);
	FText GetCombatRoleDisplayName() const;

private:
	/**
	 * @brief 적 공통 AttributeSet (HP/방어 등).
	 * @details 속성 컴포넌트는 오너 모델의 자식 UObject 중 AttributeSet을 자동 수집하므로, 생성자에서
	 *          서브오브젝트로 만들어 두기만 하면 등록된다(플레이어와 동일 패턴). 이 서브오브젝트가 없으면
	 *          수집되는 세트가 0개라 스탯 커브 초기화가 무경고로 건너뛰어지고 모든 스탯이 0으로 남는다.
	 */
	UPROPERTY(Category = AttributeSet, VisibleAnywhere, meta = (DisplayName = "UnitAttributeSet"))
	TObjectPtr<UEnemyUnitAttributeSet> mUnitAttributeSet;

protected:
	// @brief 초기 스텟에 반영되는 난이도 수치
	// 스탯 커브의 레벨 축은 1부터 시작하고 초기화기가 (레벨-1) 인덱스로 조회하므로, 0(미설정)이면
	// 조회가 실패해 스탯이 전부 0으로 남는다 — 최소 유효 난이도 1을 기본값으로 둔다.
	UPROPERTY(Category = Enemy, EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "Difficulty"))
	int32 mDifficulty = 1;

	// @brief 이동 성향
	UPROPERTY(Category = AI, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "MoveTendency"))
	EMoveTendency mMoveTendency = EMoveTendency::HoldRange;

	bool mHasMovementRoleOverride = false;
	ESRPGEnemyMovementRole mMovementRoleOverride = ESRPGEnemyMovementRole::Standard;
};
