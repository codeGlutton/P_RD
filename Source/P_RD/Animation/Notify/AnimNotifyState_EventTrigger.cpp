#include "Animation/Notify/AnimNotifyState_EventTrigger.h"
#include "Animation/BoardActorAnimInstance.h"

#if WITH_EDITOR
#include "Logging/MessageLog.h"
#include "Misc/UObjectToken.h"
#endif

#include UE_INLINE_GENERATED_CPP_BY_NAME(AnimNotifyState_EventTrigger)

UAnimNotifyState_EventTrigger::UAnimNotifyState_EventTrigger()
{
#if WITH_EDITORONLY_DATA
	NotifyColor = FColor(200, 200, 200, 255);
#endif // WITH_EDITORONLY_DATA
}

FString UAnimNotifyState_EventTrigger::GetNotifyName_Implementation() const
{
	if (mEventPayload.IsValid() == false)
	{
		return Super::GetNotifyName_Implementation();
	}
	else
	{
		FString NotifyName = mEventPayload.GetScriptStruct()->GetName();
		NotifyName.ReplaceInline(TEXT("TriggerPayload"), TEXT(""), ESearchCase::CaseSensitive);
		return NotifyName;
	}
}

FLinearColor UAnimNotifyState_EventTrigger::GetEditorColor()
{
	if (mEventPayload.IsValid() == true)
	{
		return mEventPayload.Get().GetEditorColor();
	}
	return Super::GetEditorColor();
}

void UAnimNotifyState_EventTrigger::NotifyBegin(class USkeletalMeshComponent* MeshComp, class UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (mEventPayload.IsValid() == true)
	{
		mEventPayload.GetMutable().OnEndDurationEventTrigger.Unbind();
		TriggerEvent(MeshComp, Animation, mEventPayload.GetMutable().OnEndDurationEventTrigger);
	}
}

void UAnimNotifyState_EventTrigger::NotifyEnd(class USkeletalMeshComponent* MeshComp, class UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	if (mEventPayload.IsValid() == true)
	{
		mEventPayload.GetMutable().OnEndDurationEventTrigger.ExecuteIfBound();
	}

	Super::NotifyEnd(MeshComp, Animation, EventReference);
}

#if WITH_EDITOR
void UAnimNotifyState_EventTrigger::ValidateAssociatedAssets()
{
	static const FName NAME_AssetCheck("AssetCheck");

	if (mTargetEventTag == FGameplayTag::EmptyTag)
	{
		FMessageLog AssetCheckLog(NAME_AssetCheck);

		const FText MessageLooping = FText::Format(
			NSLOCTEXT("AnimNotify", "EventTrigger", "호출할 이벤트 태그 {1}가 유효하지 않음."),
			FText::AsCultureInvariant(mTargetEventTag.ToString())
		);
		AssetCheckLog.Warning()->AddToken(FTextToken::Create(MessageLooping));

		if (GIsEditor == true)
		{
			AssetCheckLog.Notify(MessageLooping, EMessageSeverity::Warning, true);
		}
	}
}
#endif

void UAnimNotifyState_EventTrigger::TriggerEvent(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, FOnEndDurationEventTrigger& EndDurationEvent)
{
	if (MeshComp != nullptr)
	{
		UBoardActorAnimInstance* BoardActorAnimInst = Cast<UBoardActorAnimInstance>(MeshComp->GetAnimInstance());
		if (BoardActorAnimInst != nullptr)
		{
			BoardActorAnimInst->TriggerMontageTagEvent(mTargetEventTag, mEventPayload.GetPtr());
		}
	}
}

