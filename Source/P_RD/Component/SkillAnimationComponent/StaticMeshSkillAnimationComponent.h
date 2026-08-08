/*****************************************************************//**
 * @file   StaticMeshSkillAnimationComponent.h
 * @brief  스태틱 메시 기반 액터 뷰의 스킬 애니메이션 연출 컴포넌트 정의 헤더
 * @author 모호재
 * @date   2026-08-09
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "Component/SkillAnimationComponent/SkillAnimationComponent.h"
#include "StaticMeshSkillAnimationComponent.generated.h"

class UStaticMeshComponent;

/**
 * @brief 스태틱 메시 기반 액터 뷰의 스킬 애니메이션 연출 컴포넌트
 */
UCLASS(ClassGroup = (SkillAnimation), meta = (BlueprintSpawnableComponent))
class P_RD_API UStaticMeshSkillAnimationComponent : public USkillAnimationComponent
{
	GENERATED_BODY()

	/* USkillAnimationComponent 상속 */
protected:
	void PlayApplyAnimation(const FBoardActorAnimationContext& Context) override;
	void PlayHitAnimation(TSharedPtr<FPresentationBarrier> SkillEndBarrier, FGameplayTag MontageTag, ETileActorDirection MontageDir) const override;

public:
	UStaticMeshComponent* GetTargetMeshComponent() const;

protected:
	// @brief 애니메이션을 수행할 대상 스태틱 메시 컴포넌트
	mutable TWeakObjectPtr<UStaticMeshComponent> mTargetMeshComponent = nullptr;
};
