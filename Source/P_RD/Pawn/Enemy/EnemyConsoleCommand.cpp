/*****************************************************************//**
 * @file   EnemyConsoleCommand.cpp
 * @brief  적 유닛 테스트용 콘솔 명령
 * @details
 * RD.KillEnemies  현재 월드의 살아 있는 적 전원 HP를 0으로 만듦 (턴 종료 시 승리 판정)
 * @author 이문환
 * @date   2026-09-10
 *********************************************************************/

#if !UE_BUILD_SHIPPING

#include "RDMinimal.h"
#include "HAL/IConsoleManager.h"
#include "Engine/World.h"
#include "UObject/UObjectIterator.h"

#include "Pawn/Enemy/EnemyUnitModel.h"
#include "Component/AttributeComponent/AttributeSetComponentModel.h"
#include "AttributeSet/CombatTargetAttributeSet.h"

namespace
{
	// 살아 있는 적 전원의 HP를 0으로 덮어씀 (속성 세트가 사망 이펙트를 적용)
	void KillEnemies(const TArray<FString>& Args, UWorld* World)
	{
		if (World == nullptr)
		{
			return;
		}

		int32 KilledNum = 0;
		for (TObjectIterator<UEnemyUnitModel> It; It; ++It)
		{
			UEnemyUnitModel* Enemy = *It;
			if (Enemy->GetWorld() != World || Enemy->IsDead() == true)
			{
				continue;
			}

			UAttributeSetComponentModel* AttrComp = Enemy->GetAttributeComponentModel();
			if (AttrComp == nullptr)
			{
				continue;
			}

			AttrComp->ApplyModToAttribute(UCombatTargetAttributeSet::GetHPAttribute(), ETacticalModOp::Override, 0.f);
			++KilledNum;
		}

		UE_LOG(LogRD, Log, TEXT("적 제거: %d명 (턴을 종료하면 승리 판정)"), KilledNum);
	}

	FAutoConsoleCommandWithWorldAndArgs KillEnemiesCommand(
		TEXT("RD.KillEnemies"),
		TEXT("살아 있는 적 전원의 HP를 0으로 만든다. 턴을 종료하면 전투가 승리로 끝난다."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&KillEnemies));
}

#endif
