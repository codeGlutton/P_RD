// Fill out your copyright notice in the Description page of Project Settings.


#include "GameplayAbility_SkillBase.h"

#include "../../Effect/GameplayEffect_Damage.h"

UGameplayAbility_SkillBase::UGameplayAbility_SkillBase()
{	
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	FAbilityTriggerData	TriggerData;

	TriggerData.TriggerTag = FGameplayTag::RequestGameplayTag(TEXT("Test.GameplayAbility.Skill"));
	TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;

	AbilityTriggers.Add(TriggerData);
}

void UGameplayAbility_SkillBase::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	UE_LOG(LogTemp, Warning, TEXT("UGameplayAbility_SkillBase::ActivateAbility Start"));

	// 유효한 액터인지 검사한다.
	if (!ActorInfo || !ActorInfo->AvatarActor.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("ActorInfo Failed"));

		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("Ability Attack"));
	
	// Event 데이터가 유효한지 검사한다.
	if (!TriggerEventData || !TriggerEventData->TargetData.Num())
	{
		UE_LOG(LogTemp, Warning, TEXT("TriggerEventData False"));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	// 트리거 이벤트에 있는 액터들을 가져온다.
	FGameplayAbilityTargetData_ActorArray* ActorArrayData =
		(FGameplayAbilityTargetData_ActorArray*)(TriggerEventData->TargetData.Data[0].Get());

	if (!ActorArrayData)
	{
		UE_LOG(LogTemp, Warning, TEXT("ActorArrayData False"));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("ActorArrayData"));


	// 데이터 안에 들어있는 액터 배열을 반복문으로 순회한다.
	for (TWeakObjectPtr<AActor> TargetActorPtr : ActorArrayData->TargetActorArray)
	{
		AActor* TargetActor = TargetActorPtr.Get();
		
		// 유효하지 않은 액터 시 종료
		if (!IsValid(TargetActor))
		{
			UE_LOG(LogTemp, Warning, TEXT("TargetActor is not Valid"));

			EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
			continue;
		}

		UE_LOG(LogTemp, Warning, TEXT("Actor %s"), *TargetActor->GetActorLabel());


		// 피해 주기 테스트 ===========================================
#pragma region Damage Test

		FGameplayEffectContextHandle	Context = MakeEffectContext(Handle, ActorInfo);

		FGameplayEffectSpecHandle	DamageSpec = MakeOutgoingGameplayEffectSpec(
			UGameplayEffect_Damage::StaticClass(), GetAbilityLevel());

		DamageSpec.Data->SetContext(Context);

		float Damage = 10;	// 추후 넘겨받은 값으로 변경해야 함

		// GameplayEffect에 지정된 SetByCaller의 값을 지정한다.
		DamageSpec.Data->SetSetByCallerMagnitude(
			FGameplayTag::RequestGameplayTag(TEXT("Test.GameplayEffect.Data.Battle.Damage")), Damage);

		// 3. 🎯 [핵심] 이 타겟 한 명에게만 즉시 이펙트를 적용합니다.
			// TargetActor로부터 AbilitySystemComponent를 찾아서 직접 적용하는 방식입니다.
		if (UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor))
		{

			// 💡 가장 깔끔한 C++ 직접 적용 방식:
			TargetASC->ApplyGameplayEffectSpecToSelf(*DamageSpec.Data.Get());
		}

#pragma endregion Damage Test
		// 피해 주기 테스트 ===========================================
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
