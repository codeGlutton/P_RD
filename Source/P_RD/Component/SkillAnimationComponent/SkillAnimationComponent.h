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

struct FNiagaraSpawnData;

class UCameraShakeBase;
class UOptionPersistData;

namespace RDSkillAnimation
{
	/** @brief 옵션 데이터가 준비된 경우 카메라 흔들림 설정을 따른다. */
	P_RD_API bool ShouldStartCameraShake(const UOptionPersistData* OptionData);
}

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
	void OnHandleHitAnimationEvent(const FBoardActorAnimationContext& Context, UAnimMontage* EndAnim, const FEventTriggerPayloadBase* Payload);
	void OnHandleHitVFXEvent(const FBoardActorAnimationContext& Context, UAnimMontage* EndAnim, const FEventTriggerPayloadBase* Payload);
	void OnHandleCameraShakeEvent(const FBoardActorAnimationContext& Context, UAnimMontage* EndAnim, const FEventTriggerPayloadBase* Payload);
	void OnHandleCameraZoomEvent(const FBoardActorAnimationContext& Context, UAnimMontage* EndAnim, const FEventTriggerPayloadBase* Payload);
	void OnHandleTimeScaleEvent(const FBoardActorAnimationContext& Context, UAnimMontage* EndAnim, const FEventTriggerPayloadBase* Payload);

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
	void SpawnHitVFXOnSelf(TSharedPtr<FPresentationBarrier> SkillEndBarrier, const TArray<FNiagaraSpawnData>& NiagaraSpawnDatas, ETileActorDirection LocalDirection) const;
	void SpawnHitVFXOnTile(TSharedPtr<FPresentationBarrier> SkillEndBarrier, const TArray<FNiagaraSpawnData>& NiagaraSpawnDatas, const FTransform& Transform) const;
	/**
	 * @brief 모델의 애니메이션에서 카메라 줌 처리
	 * @param EndEvent 종료시 호출될 대리자
	 * @param TargetZoom 줌 스케일
	 * @param WorldPosition 위치
	 */
	void ZoomInCamera(FOnEndDurationEventTrigger& EndEvent, float TargetZoom, FVector WorldPosition) const;
	/**
	 * @brief 모델의 애니메이션에서 카메라 줌 처리
	 * @param EndEvent 종료시 호출될 대리자
	 * @param TargetZoom 줌 스케일
	 * @param EmphasisActor 대상 액터
	 */
	void ZoomInCamera(FOnEndDurationEventTrigger& EndEvent, float TargetZoom, AActor* EmphasisActor) const;
	/**
	 * @brief 모델의 애니메이션에서 카메라 흔들기 처리
	 * @param CameraShakeClass 카메라 흔들기 클래스
	 */
	void ShakeCamera(TSubclassOf<UCameraShakeBase> CameraShakeClass) const;
	/**
	 * @brief 모델의 애니메이션에서 시간 조정
	 * @param EndEvent 종료시 호출될 대리자
	 * @param Requester 요청자
	 * @param TargetTimeScale 시간 스케일
	 * @param Priority 시간 조정 우선순위
	 * @param BlendSpeed 시간 블렌딩 스케일
	 * @param Duration 유지 시간
	 */
	void RequestTimeScale(FOnEndDurationEventTrigger& EndEvent, UObject* Requester, float TargetTimeScale, float BlendSpeed, float Duration) const;

private:
	/** @brief 현재 월드의 저장된 카메라 흔들림 옵션을 조회한다. */
	bool IsCameraShakeEnabled() const;

protected:
	// @brief 소유 모델 객체
	TWeakObjectPtr<UBoardActorModel> mOwnerModel = nullptr;
};
