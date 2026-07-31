/*****************************************************************//**
 * @file   SkeletonSkillAnimationComponent.h
 * @brief  스켈레탈 메시 기반 액터 뷰의 스킬 애니메이션 연출 컴포넌트 정의 헤더
 * @author 모호재
 * @date   2026-07-30
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "Component/SkillAnimationComponent/SkillAnimationComponent.h"
#include "SkeletonSkillAnimationComponent.generated.h"

class USkeletalMeshComponent;
class UBoardActorAnimInstance;

/**
 * @brief 스켈레탈 메시 기반 액터 뷰의 스킬 애니메이션 연출 컴포넌트
 */
UCLASS(ClassGroup = (SkillAnimation), meta = (BlueprintSpawnableComponent))
class P_RD_API USkeletonSkillAnimationComponent : public USkillAnimationComponent
{
	GENERATED_BODY()

	/* USkillAnimationComponent 상속 */
protected:
	void OnRegister() override;

protected:
	void PlayApplyAnimation(const FBoardActorAnimationContext& Context) override;
	void PlayHitAnimation(TSharedPtr<FPresentationBarrier> SkillEndBarrier, FGameplayTag MontageTag, ETileActorDirection MontageDir) const override;
	void SpawnHitVFX(TSharedPtr<FPresentationBarrier> SkillEndBarrier, const TArray<FApplyNiagaraSpawnData>& NiagaraSpawnDatas, ETileActorDirection LocalDirection) const override;

public:
	/**
	 * @brief 대상 스켈레탈 메시 컴포넌트 설정
	 * @param TargetMesh 대상 메시 컴포넌트
	 */
	void SetTargetMeshComponent(USkeletalMeshComponent* TargetMesh);

public:
	USkeletalMeshComponent* GetTargetMeshComponent() const;
	UBoardActorAnimInstance* GetTargetAnimInstance() const;

protected:
	// @brief 애니메이션을 수행할 대상 스켈레탈 메시 컴포넌트
	TWeakObjectPtr<USkeletalMeshComponent> mTargetMeshComponent = nullptr;
	// @brief 애니메이션을 수행할 애님 인스턴스
	TWeakObjectPtr<UBoardActorAnimInstance> mTargetAnimInstance = nullptr;
};
