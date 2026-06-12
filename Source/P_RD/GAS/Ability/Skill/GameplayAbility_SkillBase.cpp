// Fill out your copyright notice in the Description page of Project Settings.


#include "GameplayAbility_SkillBase.h"
#include "Pawn/SkillComponent.h"
#include "Pawn/Unit.h"
#include "Pawn/SkillComponent/SkillCommitResultHolder.h"
#include "../../Effect/GameplayEffect_Damage.h"

UGameplayAbility_SkillBase::UGameplayAbility_SkillBase()
{	
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	FAbilityTriggerData	TriggerData;

	TriggerData.TriggerTag = AbilityTags::GameplayAbility_Skill;
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

	/*
	
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
	*/

	if (const USkillCommitResultHolder* ResultHolder = Cast<USkillCommitResultHolder>(TriggerEventData->OptionalObject))
	{
		UE_LOG(LogTemp, Warning, TEXT("전달된 데이터 있음"));

		// 전달된 FSkillCommitResult 데이터 활용
		FSkillCommitResult RecievedResult = ResultHolder->CommitResult;

		// TODO: 갱신기(Updater)에 RecievedResult 전달
		// 
		for (int i = 0; i < RecievedResult.mEffectCommitResult.Num(); ++i)
		{
			UE_LOG(LogTemp, Warning, TEXT("EffectCommit 있음"));

			const FEffectCommitResult& EffectResult = RecievedResult.mEffectCommitResult[i];

			// 효과 적용 시 모든 대상에게 한꺼번에 적용
			for (int j = 0; j < EffectResult.mTileCommitResult.Num(); ++j)
			{
				UE_LOG(LogTemp, Warning, TEXT("TileCommit 있음"));

				const FTileActorCommitResult& TileResult = EffectResult.mTileCommitResult[j];

				// 타일 인덱스에서 유닛을 가져온다.
				TScriptInterface<const ITileActor> TileActor = TileResult.mTileActor;

				const AUnit* TargetActor = Cast<AUnit>(TileActor.GetObject()); // 예시: TileUnit이 실제 AUnit 포인터를 담고 있다고 가정

				CommitActorToEffect(Handle, ActorInfo, TargetActor, TileResult.mUnitCommitResult);
			}
		}
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

bool UGameplayAbility_SkillBase::CommitActorToEffect(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const AUnit* CUnit, const struct FUnitCommitResult& CommitResult)
{
	UE_LOG(LogTemp, Warning, TEXT("UnitCommit %d"), CommitResult.mEffect.Num());

	for (const TPair<FGameplayTag, float>& EffectPair : CommitResult.mEffect)
	{
		UE_LOG(LogTemp, Warning, TEXT("UnitCommit 있음"));

		FGameplayTag EffectTag = EffectPair.Key;
		float EffectValue = EffectPair.Value;

		// 예시: 데미지 관련 태그인지 확인 (프로젝트에서 사용하는 데미지 태그명으로 변경)
		FGameplayTag DamageTag = FGameplayTag::RequestGameplayTag(TEXT("GameplayEffect.Skill.Damage"));

		if (EffectTag == DamageTag)
		{
			UE_LOG(LogTemp, Warning, TEXT("Damage : %f"), EffectValue);

			
			FGameplayEffectContextHandle Context = MakeEffectContext(Handle, ActorInfo);

			FGameplayEffectSpecHandle DamageSpec = MakeOutgoingGameplayEffectSpec(
				UGameplayEffect_Damage::StaticClass(), GetAbilityLevel());


			DamageSpec.Data->SetContext(Context);

			DamageSpec.Data->SetSetByCallerMagnitude(DamageTag, EffectValue);

			// const를 강제로 푼다.
			AUnit* Unit = const_cast<AUnit*>(CUnit);

			// 4. 타겟 ASC를 찾아 이펙트를 적용합니다.
			if (UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Unit))
			{
				TargetASC->ApplyGameplayEffectSpecToSelf(*DamageSpec.Data.Get());
			}
			
		}
	}

	return true;
}
