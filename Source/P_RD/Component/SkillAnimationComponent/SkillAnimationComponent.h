/*****************************************************************//**
 * @file   SkillAnimationComponent.h
 * @brief  뷰 액터 전용 스킬 애니메이션 연출 컴포넌트 베이스 헤더
 * @author 모호재
 * @date   2026-07-30
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "Components/ActorComponent.h"
#include "Component/ComponentView.h"
#include "Animation/BoardActorAnimType.h"
#include "Animation/Notify/EventTriggerPayload.h"
#include "SkillAnimationComponent.generated.h"

class UBoardActorModel;
struct FPresentationBarrier;

struct FApplyNiagaraSpawnData;

class UCameraShakeBase;

/**
 * @brief 뷰 액터 전용 스킬 애니메이션 연출 베이스 컴포넌트
 */
UCLASS(Abstract, ClassGroup = (SkillAnimation), meta = (BlueprintSpawnableComponent))
class P_RD_API USkillAnimationComponent : public UActorComponent, public IComponentView
{
	GENERATED_BODY()

public:
	USkillAnimationComponent();

	/* IComponentView 상속 */
public:
	void BindOwnerModel(UObjectModel* Model) override;
	void UnbindOwnerModel(UObjectModel* Model) override;

protected:
	/**
	 * @brief 모델의 애니메이션 실행 요청 수신 핸들러
	 * @param MotionEndBarrier 연출 완료 시기를 제어하는 배리어
	 * @param Context 애니메이션 실행 컨텍스트
	 */
	void HandleToPlayAnimation(TSharedPtr<FPresentationBarrier> MotionEndBarrier, const FBoardActorAnimationContext& Context);
	
private:
	void OnHandleHitAnimationEvent(const FBoardActorAnimationContext& Context, UAnimMontage* EndAnim, const FEventTriggerPayload* Payload);
	void OnHandleHitVFXEvent(const FBoardActorAnimationContext& Context, UAnimMontage* EndAnim, const FEventTriggerPayload* Payload);
	void OnHandleCameraShakeEvent(const FBoardActorAnimationContext& Context, UAnimMontage* EndAnim, const FEventTriggerPayload* Payload);
	void OnHandleCameraZoomEvent(const FBoardActorAnimationContext& Context, UAnimMontage* EndAnim, const FEventTriggerPayload* Payload);
	void OnHandleTimeScaleEvent(const FBoardActorAnimationContext& Context, UAnimMontage* EndAnim, const FEventTriggerPayload* Payload);

	/* 파생 객체 구현 함수 */
protected:
	/**
	 * @brief 모델의 애니메이션 실행
	 * @param Context 애니메이션 실행 컨텍스트
	 */
	virtual void PlayApplyAnimation(const FBoardActorAnimationContext& Context);

	/**
	 * @brief 모델의 애니메이션에서 피격자로의 애니메이션 실행
	 * @param SkillEndBarrier 스킬 연출 완료 시기를 제어하는 배리어
	 * @param MontageTag 히트 애님 태그
	 * @param MontageDir 히트 애님 방향
	 */
	virtual void PlayHitAnimation(TSharedPtr<FPresentationBarrier> SkillEndBarrier, FGameplayTag MontageTag, ETileActorDirection MontageDir) const;
	/**
	 * @brief 모델의 애니메이션에서 피격자로의 VFX 실행
	 * @param SkillEndBarrier 스킬 연출 완료 시기를 제어하는 배리어
	 * @param NiagaraSpawnData 나이아가라 스폰 요청 정보
	 * @param LocalDirection 방향
	 */
	virtual void SpawnHitVFX(TSharedPtr<FPresentationBarrier> SkillEndBarrier, const TArray<FApplyNiagaraSpawnData>& NiagaraSpawnDatas, ETileActorDirection LocalDirection) const;
	
	void ZoomInCamera(FOnEndDurationEventTrigger& EndEvent, float TargetZoom, FVector WorldPosition) const;
	void ZoomInCamera(FOnEndDurationEventTrigger& EndEvent, float TargetZoom, AActor* EmphasisActor) const;

	void ShakeCamera(TSubclassOf<UCameraShakeBase> CameraShakeClass) const;
	
	void RequestTimeScale(FOnEndDurationEventTrigger& EndEvent, UObject* Requester, float TargetTimeScale, int32 Priority, float BlendSpeed, float Duration) const;

protected:
	// @brief 소유 모델 객체
	TWeakObjectPtr<UBoardActorModel> mOwnerModel = nullptr;
};
