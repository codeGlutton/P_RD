// Fill out your copyright notice in the Description page of Project Settings.


#include "GameplayAbility_Base.h"

#include "../Effect/GameplayEffect_Damage.h"

UGameplayAbility_Base::UGameplayAbility_Base()
{
}

void UGameplayAbility_Base::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	UE_LOG(UELOG, Warning, TEXT("UGameplayAbility_Base::ActivateAbility Start"));

	if (!ActorInfo || !ActorInfo->AvatarActor.IsValid())
	{
		UE_LOG(UELOG, Warning, TEXT("ActorInfo Failed"));

		mAbilityActive = false;
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	mAbilityActive = true;

	UE_LOG(UELOG, Warning, TEXT("Ability Attack"));

	if (!mAbilityActive || !TriggerEventData || !TriggerEventData->Target)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	// 트리거 이벤트에 있는 액터들을 가져온다.
	FGameplayAbilityTargetData_ActorArray* ActorArrayData =
		(FGameplayAbilityTargetData_ActorArray*)(TriggerEventData->TargetData.Data[0].Get());

	if (!ActorArrayData)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}


	// 2. 데이터 안에 들어있는 액터 배열을 반복문으로 순회합니다.
	for (TWeakObjectPtr<AActor> TargetActorPtr : ActorArrayData->TargetActorArray)
	{
		// 약참조(WeakPtr)이므로 실제로 존재하는지 확인
		AActor* TargetActor = TargetActorPtr.Get();
		
		if (!TargetActor)
		{
			EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
			return;
		}

		// 피해 주기 테스트 ===========================================
		FGameplayEffectContextHandle	Context = MakeEffectContext(Handle, ActorInfo);

		FGameplayEffectSpecHandle	DamageSpec = MakeOutgoingGameplayEffectSpec(
			UGameplayEffect_Damage::StaticClass(), GetAbilityLevel());

		DamageSpec.Data->SetContext(Context);

		float Damage = 10;	// 추후 넘겨받은 값으로 변경해야 함

		// GameplayEffect에 지정된 SetByCaller의 값을 지정한다.
		DamageSpec.Data->SetSetByCallerMagnitude(
			FGameplayTag::RequestGameplayTag(TEXT("Test.GameplayEffect.Data.Battle.Damage")), -Damage);
	
		// 3. 🎯 [핵심] 이 타겟 한 명에게만 즉시 이펙트를 적용합니다.
			// TargetActor로부터 AbilitySystemComponent를 찾아서 직접 적용하는 방식입니다.
		if (UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor))
		{

			// 💡 가장 깔끔한 C++ 직접 적용 방식:
			TargetASC->ApplyGameplayEffectSpecToSelf(*DamageSpec.Data.Get());
		}

		// 피해 주기 테스트 ===========================================
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
