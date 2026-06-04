/*****************************************************************//**
 * @file   EnemyAIController.h
 * @brief  Enemy Unit의 AI 컨트롤러 정의 헤더
 * @author 모호재
 * @date   2026-05-26
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "AIController.h"
#include "EnemyAIController.generated.h"

class UStateTree;
class UStateTreeAIComponent;

/**
 * @brief  Enemy Unit의 AI 컨트롤러
 */
UCLASS()
class P_RD_API AEnemyAIController : public AAIController
{
	GENERATED_BODY()
public:
	AEnemyAIController();

	// GenericTeamAgentInterface 상속 
public:
	void SetGenericTeamId(const FGenericTeamId& NewTeamID) override;
	FGenericTeamId GetGenericTeamId() const override;

public:
	void StartLogic(UStateTree* StateTree);

public:
	UStateTreeAIComponent* GetStateTreeComponent() const;

private:
	// [Var] 현재 Enemy의 AI 동작 구조 담당 컴포넌트
	UPROPERTY(Category = AI, EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true", DisplayName = "StateTreeAIComponent"))
	TObjectPtr<UStateTreeAIComponent> mStateTreeAIComponent;
};
