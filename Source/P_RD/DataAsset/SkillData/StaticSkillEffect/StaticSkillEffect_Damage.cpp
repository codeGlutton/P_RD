#include "DataAsset/SkillData/StaticSkillEffect/StaticSkillEffect_Damage.h"
#include "Component/AttributeComponent/AttributeSetComponentModel.h"
#include "Actor/BoardActor/BoardActorModel.h"
#include "TAS/Effect/Stat/TacticalEffect_Damage.h"
#include "TAS/Effect/Stat/TacticalEffect_DamagePoint.h"
#include "TAS/Effect/TacticalEffectContext.h"

#include "Actor/BoardActor/BoardCombatTarget.h"

#include "AttributeSet/UnitAttributeSet.h"

bool UStaticSkillEffect_Damage::ApplyOtherPointToSkillPoint(float SkillPoint,
	TWeakObjectPtr<class UBoardActorModel> SourceActor,
	OUT FActiveTacticalEffectHandle& EffectHandle)
{
	IBoardCombatTarget* SourceActorTarget = Cast<IBoardCombatTarget>(SourceActor);
	if (SourceActorTarget == nullptr)
	{
		// 전투 가능한 대상이 아님
		return false;
	}

	// 시전자의 정보를 가져온다.
	TWeakObjectPtr<UAttributeSetComponentModel> SourceASC = SourceActorTarget->GetAttributeComponentModel();
	if (!SourceASC.IsValid())
	{
		// 포인트를 올려줄 수 없습니다.
		return false;
	}

	// Effect를 사용하여 스킬 포인트를 AttackPoint로 변환한다.
	UTacticalEffectContext* EffectContext = SourceASC->MakeEffectContext();
	EffectContext->SetInstigator(SourceActor.Get());
	EffectContext->SetAttributeSetComponentModel(SourceASC.Get());

	UE_LOG(LogTemp, Warning, TEXT("기본 값 + (계수) * 주사위 포인트 = 결과값"), SourceASC->GetAttributeCurrentValue(UUnitAttributeSet::GetDamagePointAttribute()));
	UE_LOG(LogTemp, Warning, TEXT("%d + (%f) * %f = %f"), mEffectDefaultValue, mEffectRatioValue, SkillPoint, mEffectDefaultValue + mEffectRatioValue * SkillPoint);


	TSharedPtr<FTacticalEffectSpec> EffectSpec = SourceASC->MakeOutgoingSpec(UTacticalEffect_DamagePoint::StaticClass(), EffectContext);
	EffectSpec->mDynamicMagnitude = mEffectDefaultValue + mEffectRatioValue * SkillPoint;

	UE_LOG(LogTemp, Warning, TEXT("PreDamagekPoint : %f"), SourceASC->GetAttributeCurrentValue(UUnitAttributeSet::GetDamagePointAttribute()));

	EffectHandle = SourceASC->ApplyTacticalEffectSpecToSelf(*EffectSpec);

	UE_LOG(LogTemp, Warning, TEXT("CurDamagePoint : %f"), SourceASC->GetAttributeCurrentValue(UUnitAttributeSet::GetDamagePointAttribute()));

	return true;
}

void UStaticSkillEffect_Damage::ApplySkillEffect(
	TWeakObjectPtr<UBoardActorModel> SourceActor,
	TWeakObjectPtr<UBoardActorModel> TargetActor,
	FBoardCombatTargetSnapshotData* SourceSnapShot,
	FBoardCombatTargetSnapshotData* TargetSnapShot)
{
	// 유효한 타겟인지 검사합니다.
	checkf(SourceActor.IsValid(), TEXT("시전자 유효하지 않음"));
	checkf(TargetActor.IsValid(), TEXT("타겟 유효하지 않음"));

	IBoardCombatTarget* SourceActorTarget = Cast<IBoardCombatTarget>(SourceActor);
	if (SourceActorTarget == nullptr)
	{
		// 전투 가능한 대상이 아님
		return;
	}
	IBoardCombatTarget* TargetActorTarget = Cast<IBoardCombatTarget>(TargetActor);
	if (TargetActorTarget == nullptr)
	{
		// 전투 가능한 대상이 아님
		return;
	}

	// 시전자의 정보를 가져온다.
	TWeakObjectPtr<UAttributeSetComponentModel> SourceASC = SourceActorTarget->GetAttributeComponentModel();
	if (!SourceASC.IsValid())
		return;
	UE_LOG(LogTemp, Warning, TEXT("SourceID : %d"), SourceASC->GetUniqueID());

	// 피격자의 정보를 가져온다.
	TWeakObjectPtr<UAttributeSetComponentModel> TargetASC = TargetActorTarget->GetAttributeComponentModel();
	if (!TargetASC.IsValid())
		return;
	UE_LOG(LogTemp, Warning, TEXT("TargetID : %d"), TargetASC->GetUniqueID());

	// Effect를 적용한다.
	UTacticalEffectContext* EffectContext = TargetASC->MakeEffectContext();
	EffectContext->SetInstigator(SourceActor.Get());
	EffectContext->SetAttributeSetComponentModel(TargetASC.Get());

	TSharedPtr<FTacticalEffectSpec> EffectSpec = TargetASC->MakeOutgoingSpec(UTacticalEffect_Damage::StaticClass(), EffectContext);
	// 공격 포인트를 가져옵니다.
	EffectSpec->mDynamicMagnitude = SourceASC->GetAttributeCurrentValue(UUnitAttributeSet::GetDamagePointAttribute());

	UE_LOG(LogTemp, Warning, TEXT("PreHP : %f"), TargetASC->GetAttributeCurrentValue(UUnitAttributeSet::GetHPAttribute()));

	SourceASC->ApplyTacticalEffectSpecToTarget(*EffectSpec, TargetASC.Get());

	UE_LOG(LogTemp, Warning, TEXT("CurHP : %f"), TargetASC->GetAttributeCurrentValue(UUnitAttributeSet::GetHPAttribute()));
}
