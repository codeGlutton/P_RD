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
class UBoardActorSequencePlayer;

/**
 * @brief 스태틱 메시 기반 액터 뷰의 스킬 애니메이션 연출 컴포넌트
 */
UCLASS(ClassGroup = (SkillAnimation), meta = (BlueprintSpawnableComponent))
class P_RD_API UStaticMeshSkillAnimationComponent : public USkillAnimationComponent
{
	GENERATED_BODY()

public:
	UStaticMeshSkillAnimationComponent();

	/* USkillAnimationComponent 상속 */
protected:
	void OnRegister() override;

public:
	void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	void PlayApplyAnimation(const FBoardActorAnimationContext& Context) override;
	void PlayHitAnimation(TSharedPtr<FPresentationBarrier> SkillEndBarrier, FGameplayTag MontageTag, ETileActorDirection MontageDir) const override;

public:
	UBoardActorSequencePlayer* GetTargetSequencePlayer() const;
	UStaticMeshComponent* GetTargetMeshComponent() const;

private:
	// @brief 애니메이션을 재생할 플레이어 클래스
	UPROPERTY(Category = Sequence, EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "SequencePlayerClass", AllowPrivateAccess = "true"))
	TSubclassOf<UBoardActorSequencePlayer> mSequencePlayerClass;

	// @brief 애니메이션을 재생할 플레이어 객체
	UPROPERTY(Category = Sequence, transient, VisibleAnywhere, meta = (DisplayName = "SequencePlayer"))
	TObjectPtr<UBoardActorSequencePlayer> mSequencePlayer;

protected:
	// @brief 애니메이션을 수행할 대상 스태틱 메시 컴포넌트
	mutable TWeakObjectPtr<UStaticMeshComponent> mTargetMeshComponent = nullptr;
};
