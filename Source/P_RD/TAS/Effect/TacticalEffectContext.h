/*****************************************************************//**
 * @file   TacticalEffectContext.h
 * @brief  이펙트 적용 시 참고되는 공용 메타 데이터 구현 헤더
 * @author 모호재, 김준형
 * @date   2026-06-25
 *********************************************************************/

#pragma once

#include "AttributeSet/AttributeSetMinimal.h"
#include "UObject/Object.h"
#include "TacticalEffectContext.generated.h"

class UActorModel;
class UTacticalAbility;
class UAttributeSetComponentModel;

/**
 * @brief  이펙트 적용 시 참고되는 공용 메타 데이터 구현 헤더
 * @details
 * 기본적으로 Spec 내에서 모든 Effect가 참조할 수 있는 메타 데이터를 의미
 * 특정 Effect에 대한 정보가 아닌 이를 구성한 환경에 대한 정보를 담는다.
 */
UCLASS()
class P_RD_API UTacticalEffectContext : public UObject
{
	GENERATED_BODY()

public:
	UTacticalEffectContext() = default;
	UTacticalEffectContext(UActorModel* Instigator, UObject* EffectCauser);

public:
	void SetInstigator(UActorModel* Instigator);
	void SetEffectCauser(UObject* EffectCauser);
	void SetSourceObject(const UObject* SourceObject);
	void AddTargetActor(UActorModel* TargetActor);
	void SetAbility(const UTacticalAbility* Ability);
	void SetAttributeSetComponentModel(const UAttributeSetComponentModel* Model);

public:
	UActorModel* GetInstigator() const
	{
		return mInstigator.Get();
	}

	UObject* GetEffectCauser() const
	{
		return mEffectCauser.Get();
	}

	UObject* GetSourceObject() const
	{
		return mSourceObject.Get();
	}

	UTacticalAbility* GetAbility() const
	{
		return mAbilityInstance.Get();
	}

	UAttributeSetComponentModel* GetAttributeSetComponentModel() const
	{
		return mAttributeSetCompModelInstance.Get();
	}

protected:
	// @brief 피해 유발자 (IBoardCombatTarget을 상속해야함) (ex 유닛)
	UPROPERTY(Category = "Instigator", VisibleAnywhere, meta = (DisplayName = "Instigator"))
	TWeakObjectPtr<UActorModel> mInstigator = nullptr;

	// @brief 타격 시 사용된 객체 (ex 투사체, 무기)
	UPROPERTY(Category = "Causer", VisibleAnywhere, meta = (DisplayName = "EffectCauser"))
	TWeakObjectPtr<UObject> mEffectCauser = nullptr;

	// @brief Effect를 제작한 객체 (ex Skill Component)
	UPROPERTY(Category = "Source", VisibleAnywhere, meta = (DisplayName = "SourceObject"))
	TWeakObjectPtr<UObject> mSourceObject = nullptr;

protected:
	// @brief Effect를 호출한 어빌리티
	UPROPERTY(Category = "Instance", VisibleAnywhere, meta = (DisplayName = "AbilityInstance"))
	TWeakObjectPtr<UTacticalAbility> mAbilityInstance = nullptr;

	// @brief 피해 유발자의 어빌리티
	UPROPERTY(Category = "Instance", VisibleAnywhere, meta = (DisplayName = "AttributeSetCompModelInstance"))
	TWeakObjectPtr<UAttributeSetComponentModel> mAttributeSetCompModelInstance = nullptr;
};
