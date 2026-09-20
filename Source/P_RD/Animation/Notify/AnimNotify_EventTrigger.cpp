#include "Animation/Notify/AnimNotify_EventTrigger.h"
#include "Animation/BoardActorAnimInstance.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AnimNotify_EventTrigger)

#if WITH_EDITOR
#include "Logging/MessageLog.h"
#include "Misc/UObjectToken.h"
#endif

UAnimNotify_EventTrigger::UAnimNotify_EventTrigger()
{
#if WITH_EDITORONLY_DATA
	NotifyColor = FColor(200, 200, 200, 255);
#endif // WITH_EDITORONLY_DATA
}

FString UAnimNotify_EventTrigger::GetNotifyName_Implementation() const
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

FLinearColor UAnimNotify_EventTrigger::GetEditorColor()
{
	if (mEventPayload.IsValid() == true)
	{
		return mEventPayload.Get().GetEditorColor();
	}
	return Super::GetEditorColor();
}

void UAnimNotify_EventTrigger::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	TriggerEvent(MeshComp, Animation);
}

#if WITH_EDITOR

void UAnimNotify_EventTrigger::ValidateAssociatedAssets()
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

void UAnimNotify_EventTrigger::TriggerEvent(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
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

