/*****************************************************************//**
 * @file   AnimNotifyCondition.cpp
 * @brief  애니메이션 노티파이 실행 조건 구현부
 * @author 모호재
 * @date   2026-07-15
 *********************************************************************/

#include "Animation/Notify/AnimNotifyCondition.h"
#include "GameMode/RDGameModeBase.h"
#include "Singleton/InstanceSubsystem/PersistentData.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AnimNotifyCondition)

bool FAnimNotifyCondition_EffectOption::EvaluateCondition(const USkeletalMeshComponent* MeshComp) const
{
	if (MeshComp == nullptr)
	{
		return false;
	}

	const UWorld* World = MeshComp->GetWorld();
	if (World == nullptr)
	{
		return false;
	}

	// 게임 모드에서 옵션 캐시 데이터를 획득
	const ARDGameModeBase* GameMode = Cast<ARDGameModeBase>(World->GetAuthGameMode());
	if (GameMode == nullptr)
	{
		return false;
	}

	const UOptionPersistData* OptionData = GameMode->GetOptionPersistData();
	if (OptionData == nullptr)
	{
		return false;
	}

	return OptionData->IsEffectVFXEnabled();
}
