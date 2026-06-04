#include "AIController/EnemyAIController.h"
#include "Components/StateTreeAIComponent.h"

AEnemyAIController::AEnemyAIController()
{
	mStateTreeAIComponent = CreateDefaultSubobject<UStateTreeAIComponent>(TEXT("StateTreeAIComponent"));
	mStateTreeAIComponent->SetStartLogicAutomatically(false);
}

void AEnemyAIController::SetGenericTeamId(const FGenericTeamId& NewTeamID)
{
	auto* TeamAgent = GetPawn<IGenericTeamAgentInterface>();
	if (TeamAgent != nullptr)
	{
		TeamAgent->SetGenericTeamId(NewTeamID);
	}
}

FGenericTeamId AEnemyAIController::GetGenericTeamId() const
{
	auto* TeamAgent = GetPawn<IGenericTeamAgentInterface>();
	return TeamAgent != nullptr ? TeamAgent->GetGenericTeamId() : FGenericTeamId();
}

void AEnemyAIController::StartLogic(UStateTree* StateTree)
{
	mStateTreeAIComponent->SetStateTree(StateTree);
	mStateTreeAIComponent->StartLogic();
}

UStateTreeAIComponent* AEnemyAIController::GetStateTreeComponent() const
{
	return mStateTreeAIComponent;
}