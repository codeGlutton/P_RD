/*****************************************************************//**
 * @file   PassiveComponentModel.h
 * @brief  패시브를 보유·관리하는 컴포넌트 모델
 * @author 이문환
 * @date   2026-06-24
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "Component/ComponentModel.h"
#include "PassiveComponentModel.generated.h"

class UTacticalPassive;

// 패시브 추가/제거 알림 (대상 패시브 인스턴스 전달)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPassiveChanged, UTacticalPassive*, Passive);

/**
 * @brief 패시브 보유 컴포넌트 모델
 *
 * @details
 * 액터 모델이 소유하는 여러 패시브를 1:N로 보관·관리.
 * 드라이버가 발동 시점 태그로 패시브를 모아 호출할 수 있도록 조회 API 제공.
 *
 * @par 부착
 * 액터 모델의 PreInitializeComponentModels에서 AddComponentModelByClass로 등록,
 * FindComponentModelByClass로 조회.
 */
UCLASS()
class P_RD_API UPassiveComponentModel : public UComponentModel
{
	GENERATED_BODY()

public:
	virtual void Initialize() override;
	virtual void Uninitialize() override;

public:
	/**
	 * @brief 패시브를 생성해 보유 목록에 추가
	 * @param PassiveClass 생성할 패시브 클래스
	 * @return 생성된 패시브 인스턴스 (PassiveClass가 없으면 nullptr)
	 */
	UTacticalPassive* AddPassive(TSubclassOf<UTacticalPassive> PassiveClass);

	/**
	 * @brief 보유 목록에서 패시브를 제거
	 * @param Passive 제거할 패시브 인스턴스
	 * @return 제거 성공 여부
	 */
	bool RemovePassive(UTacticalPassive* Passive);

	/**
	 * @brief 발동 시점 태그가 일치하는 패시브를 모아 반환
	 * @details 현재는 보유 목록 전체를 순회하는 단순 구현. 성능 문제 시 시점별 버킷화.
	 * @param TriggerTiming 조회할 발동 시점 태그
	 * @return 시점이 일치하는 패시브 목록
	 */
	TArray<UTacticalPassive*> GetPassivesByTiming(const FGameplayTag& TriggerTiming) const;

	// 보유한 패시브 전체 반환
	const TArray<TObjectPtr<UTacticalPassive>>& GetPassives() const { return mPassives; }

public:
	// 패시브 추가 알림
	FOnPassiveChanged OnPassiveAdded;
	// 패시브 제거 알림
	FOnPassiveChanged OnPassiveRemoved;

protected:
	// 보유한 패시브 인스턴스 목록
	UPROPERTY(VisibleAnywhere, meta = (DisplayName = "Passives"))
	TArray<TObjectPtr<UTacticalPassive>> mPassives;
};
