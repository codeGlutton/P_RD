/*****************************************************************//**
 * @file   SRPGMoveBuildAction.h
 * @brief  이동 생성 액션 객체 구현 헤더
 * @author 이문환
 * @date   2026-06-22
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "SRPGFramework/SRPGAction.h"
#include "SRPGFramework/SRPGCommand.h"
#include "SRPGMoveBuildAction.generated.h"

class USRPGMoveBuildAction;
class UStaticSkillData;
class UTileMapModel;

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnChangeMoveBuildPhase, const USRPGMoveBuildAction* /*Action*/, ESRPGMoveBuildPhase /*Phase*/);


// @brief 이동 빌드 진입 명령 (이동 액션 생성을 요청)
USTRUCT(BlueprintType)
struct FSRPGMoveSelectCommand : public FSRPGCommand
{
	GENERATED_BODY()

public:
	FSRPGMoveSelectCommand();

public:
	FOnChangeMoveBuildPhase OnChangeMoveBuildPhase;
};

/**
 * @brief  이동 생성 액션 객체
 *
 * @details
 * 스킬 생성 액션(USRPGSkillBuildAction)을 본떠 만든 이동 전용 빌드 액션이다.
 * 유닛의 이동 스킬을 얻어와 주사위로 도달 거리를 정하고, 도달 가능 타일에서
 * 목적지를 골라 경로를 프리뷰한 뒤, 확정 시 이동 액션 생성 명령을 발행한다.
 * 스킬과 달리 사용할 스킬은 이동 스킬 하나로 고정이라 스킬 인덱스를 두지 않는다.
 */
UCLASS()
class USRPGMoveBuildAction : public USRPGAction
{
	GENERATED_BODY()

public:
	USRPGMoveBuildAction();

	/* FSRPGAction 상속 */
protected:
	void OnBeginAction() override;
	void OnEndAction() override;

protected:
	ESRPGCommandResult HandleCommand(const TInstancedStruct<FSRPGCommand>& Command) override;

protected:
	ESRPGCommandResult HandleWorldTraceCommand(const TInstancedStruct<FSRPGCommand>& Command);

	/* 빌드 로직 처리 */
private:
	void ChangeDices(int32 RequestedDiceIndex);

	void SetMoveSkill();
	void ResetMoveSkill();
	void SetTargetTile(const FTileIndex& TileIndex);
	void ResetTargetTile();
	void BuildMove();

private:
	void SetBuildPhase(ESRPGMoveBuildPhase BuildPhase);

	/* 헬퍼 */
private:
	// @brief 턴 컨텍스트 → 전투 모델 → 타일 맵 모델을 꺼내온다 (없으면 checkf)
	UTileMapModel* GetTileMap() const;
	// @brief 선택된 주사위들의 눈금 합을 실제 주사위 풀에서 합산
	int32 GetSelectedDiceSum() const;
	// @brief 현재 이동 스킬·주사위합 기준으로 도달 범위를 재계산하고 강조 갱신
	void RefreshReachableTiles();

protected:
	FOnChangeMoveBuildPhase OnChangeMoveBuildPhase;

protected:
	UPROPERTY(Category = Build, EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "MoveBuildPhase"))
	ESRPGMoveBuildPhase mMoveBuildPhase = ESRPGMoveBuildPhase::None;

protected:
	UPROPERTY(Category = Build, EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "ReachableTileIndexes"))
	TArray<FTileIndex> mReachableTileIndexes;
	UPROPERTY(Category = Build, EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "SelectedSkill"))
	TObjectPtr<UStaticSkillData> mSelectedSkill;

	UPROPERTY(Category = Build, EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "SelectedDices"))
	TArray<int32> mSelectedDices;
	UPROPERTY(Category = Build, EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "SelectedDiceSum"))
	int32 mSelectedDiceSum = 0;

	UPROPERTY(Category = Build, EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "PathTileIndexes"))
	TArray<FTileIndex> mPathTileIndexes;
	UPROPERTY(Category = Build, EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "TargetIndex"))
	FTileIndex mTargetIndex = FTileIndex::Invalid;
};
