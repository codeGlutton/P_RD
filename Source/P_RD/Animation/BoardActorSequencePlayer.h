/*****************************************************************//**
 * @file   BoardActorSequencePlayer.h
 * @brief  스태틱 메시를 가진 보드 액터의 레벨 시퀀스 플레이어 정의 헤더
 * @author 모호재
 * @date   2026-08-14
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "Animation/BoardActorAnimType.h"

#include "IMovieScenePlaybackClient.h"
#include "LevelSequenceCameraSettings.h"
#include "MovieSceneSequencePlaybackSettings.h"
#include "MovieSceneBindingOwnerInterface.h"
#include "MovieSceneBindingOverrides.h"

#include "BoardActorSequencePlayer.generated.h"

class ULevelSequence;
class ULevelSequencePlayer;

/**
 * @brief  스태틱 메시를 가진 보드 액터의 레벨 시퀀스 플레이어
 */
UCLASS(BlueprintType, Blueprintable, abstract)
class P_RD_API UBoardActorSequencePlayer : public UObject, public IMovieScenePlaybackClient
{
	GENERATED_BODY()

public:
	UBoardActorSequencePlayer();

	/* UObject 상속 */
public:
	UWorld* GetWorld() const override;

protected:
	void PostInitProperties() override;

	/* IMovieScenePlaybackClient 상속 */
protected:
	bool RetrieveBindingOverrides(const FGuid& InBindingId, FMovieSceneSequenceID InSequenceID, TArray<UObject*, TInlineAllocator<1>>& OutObjects) const override;
	UObject* GetInstanceData() const override;

	/* 애니메이션 업데이트 */
public:
	virtual void NativeUpdateSequence(float DeltaSeconds);

	/* 애니메이션 실행 */
public:
	bool PlaySequenceUsingTag(const FGameplayTag& MontageTag, ETileActorDirection LocalDirection);
	bool PlaySequenceUsingTag(const FBoardActorAnimationContext& Context);
	void StopSequenceUsingTag();

protected:
	bool PlaySequenceUsingTag_Internal(FBoardActorAnimationContext Context);
	void OnEndSequenceUsingTag(bool IsInterrupted);

	/* 이벤트 등록 및 트리거 */
public:
	bool TriggerMontageTagEvent(const FGameplayTag& EventTag, const FEventTriggerPayloadBase* Payload);

	bool RegisterTagEventOnMontage(const FGameplayTag& EventTag, FBoardActorAnimationEvent&& Event);
	bool UnregisterTagEventOnMontage(const FGameplayTag& EventTag);

	bool RegisterTagEventOnAllMontage(const FGameplayTag& EventTag, FBoardActorAllAnimationEvent&& Event);
	bool UnregisterTagEventOnAllMontage(const FGameplayTag& EventTag);

	/* 동적 바인딩 지정 API */
public:
	void SetDynamicBinding(FName BindingTag, AActor* Actor);
	void SetDynamicBindings(FName BindingTag, const TArray<AActor*>& Actors);
	void AddDynamicBinding(FName BindingTag, AActor* Actor);
	void RemoveDynamicBinding(FName BindingTag, AActor* Actor);
	void ClearDynamicBindings(FName BindingTag);
	void ClearAllDynamicBindings();

protected:
	void SetBindingOnSequence(FMovieSceneObjectBindingID Binding, const TArray<AActor*>& Actors, bool AllowBindingsFromAsset = false);
	void SetBindingOnSequenceByTag(const ULevelSequence* Sequence, FName BindingTag, const TArray<AActor*>& Actors, bool AllowBindingsFromAsset = false);
	void ApplyDynamicBindings(const ULevelSequence* Sequence);

	/* 재생 상태 조회 */
public:
	bool IsPlayingSequenceUsingTag() const;
	ULevelSequence* GetPlayingSequenceUsingTag() const;
	const FTagLevelSequenceSet* GetPlayingSequenceSetUsingTag() const;
	ULevelSequencePlayer* GetSequencePlayer() const;

	/* 컴포넌트 캐싱 */
public:
	UActorComponent* GetOwningComponent() const;
	AActor* GetOwningActor() const;

protected:
	UActorComponent* GetOwningComponentChecked() const;
	UActorComponent* GetOwningComponentUnchecked() const;

	/* 시퀀스 등록 */
protected:
	UPROPERTY(Category = Sequence, EditDefaultsOnly, BlueprintReadOnly, meta = (DisplayName = "LevelSequenceTags"))
	TMap<FGameplayTag, FTagLevelSequenceSet> mTagSequenceSets;

	UPROPERTY(Category = Sequence, EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "AutoBindOwnerActor"))
	bool mAutoBindOwnerActor = true;

	UPROPERTY(Category = Sequence, EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "DefaultOwnerActorBindingTag"))
	FName mDefaultOwnerActorBindingTag = TEXT("OwnerActor");

	/* 시퀀스 실행 정보 */
protected:
	TMap<FGameplayTag, FBoardActorAllAnimationEvent> mAllMontageEvents;
	FBoardActorAnimationContext mActiveAnimationContext;

	TMap<FName, TArray<TWeakObjectPtr<AActor>>> mDynamicActorBindings;

protected:
	UPROPERTY(Category = Sequence, Instanced, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "LevelSequencePlayer"))
	TObjectPtr<ULevelSequencePlayer> mLevelSequencePlayer;

	/* 계산 값 */
protected:
	UPROPERTY(Category = Data, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "ActorVelocity"))
	FVector2D mActorVelocity = FVector2D::ZeroVector;
	UPROPERTY(Category = Data, EditDefaultsOnly, BlueprintReadOnly, meta = (DisplayName = "MinWalkSquareVelocity"))
	float mMinWalkSquareVelocity = 10.f;

	/* 설정 값 */
protected:
	UPROPERTY(Category = Setting, EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "CameraSettings", ShowOnlyInnerProperties, ExposeOnSpawn))
	FLevelSequenceCameraSettings mCameraSettings;

	UPROPERTY(Category = Setting, EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "PlaybackSettings", ShowOnlyInnerProperties, ExposeOnSpawn))
	FMovieSceneSequencePlaybackSettings mPlaybackSettings;

	UPROPERTY(Category = Setting, Instanced, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "BindingOverrides"))
	TObjectPtr<UMovieSceneBindingOverrides> mBindingOverrides;

	UPROPERTY(Category = Setting, EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "OverrideInstanceData"))
	uint8 mOverrideInstanceData : 1 = true;

	UPROPERTY(Category = Setting, Instanced, EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "DefaultInstanceData"))
	TObjectPtr<UObject> mDefaultInstanceData;
};
