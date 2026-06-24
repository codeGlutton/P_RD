/*****************************************************************//**
 * @file   PassiveComponentModel.cpp
 * @brief  패시브 보유 컴포넌트 모델 구현
 * @author 이문환
 * @date   2026-06-24
 *********************************************************************/

#include "Component/PassiveComponent/PassiveComponentModel.h"
#include "TAS/Passive/TacticalPassive.h"

void UPassiveComponentModel::Initialize()
{
	// 부착 직후 초기화 (현재 별도 작업 없음)
}

void UPassiveComponentModel::Uninitialize()
{
	// 보유 패시브 목록 정리
	mPassives.Reset();
}

UTacticalPassive* UPassiveComponentModel::AddPassive(TSubclassOf<UTacticalPassive> PassiveClass)
{
	// 클래스가 없으면 생성 불가
	if (PassiveClass == nullptr)
	{
		return nullptr;
	}

	// 컴포넌트 모델을 Outer로 패시브 인스턴스 생성 후 보유 목록에 추가
	UTacticalPassive* Passive = NewObject<UTacticalPassive>(this, PassiveClass);
	mPassives.Add(Passive);

	// 추가 알림
	if (OnPassiveAdded.IsBound())
	{
		OnPassiveAdded.Broadcast(Passive);
	}

	return Passive;
}

bool UPassiveComponentModel::RemovePassive(UTacticalPassive* Passive)
{
	// 대상이 없으면 제거 불가
	if (Passive == nullptr)
	{
		return false;
	}

	// 보유 목록에 없으면 실패
	if (mPassives.Remove(Passive) <= 0)
	{
		return false;
	}

	// 제거 알림
	if (OnPassiveRemoved.IsBound())
	{
		OnPassiveRemoved.Broadcast(Passive);
	}

	return true;
}

TArray<UTacticalPassive*> UPassiveComponentModel::GetPassivesByTiming(const FGameplayTag& TriggerTiming) const
{
	TArray<UTacticalPassive*> Result;

	// 보유 목록 전체를 순회하며 시점 태그 일치분 수집
	for (const TObjectPtr<UTacticalPassive>& Passive : mPassives)
	{
		if (Passive != nullptr && Passive->GetTriggerTiming() == TriggerTiming)
		{
			Result.Add(Passive);
		}
	}

	return Result;
}
