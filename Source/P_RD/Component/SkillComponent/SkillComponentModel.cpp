// Fill out your copyright notice in the Description page of Project Settings.


#include "Component/SkillComponent/SkillComponentModel.h"

#include "Singleton/WorldSubsystem/SRPGCombatModel.h"
#include "Actor/TileMap/TileMapModel.h"
#include "Actor/BoardActor/BoardActorModel.h"
#include "AttributeSet/UnitAttributeSet.h"
#include "Component/AttributeComponent/AttributeSetComponentModel.h"

#include "Pawn/UnitModel.h"


// Sets default values for this component's properties
USkillComponentModel::USkillComponentModel()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	

	// ...

}


void USkillComponentModel::Initialize()
{
	Super::Initialize();

}

// Called when the game starts
void USkillComponentModel::BeginPlay()
{
	Super::BeginPlay();

	// ...

}

void USkillComponentModel::GetSkillData(int In_SkillIndex, OUT TSoftObjectPtr<UStaticSkillData>& Out_SkillData)
{
	checkf(mSkillData.IsValidIndex(In_SkillIndex), TEXT("잘못된 배열 범위"))

	Out_SkillData = mSkillData[In_SkillIndex];
}

void USkillComponentModel::SetSkillData(int SkillIndex, IN const TSoftObjectPtr<UStaticSkillData>& SkillData)
{
	checkf(mSkillData.IsValidIndex(SkillIndex), TEXT("잘못된 배열 범위"))

		mSkillData[SkillIndex] = SkillData;

	if (OnSkillChange.IsBound())
		OnSkillChange.Broadcast(SkillIndex, SkillData);
}


void USkillComponentModel::AddSkillData(IN const TSoftObjectPtr<UStaticSkillData>& SkillData)
{
	mSkillData.Add(SkillData);

	if (OnSkillChange.IsBound())
		OnSkillChange.Broadcast(mSkillData.Num() - 1, SkillData);

}

bool USkillComponentModel::ActivateSkill(int32 SkillIndex, const TArray<FTileIndex> TargetTiles, int32 SkillPoint)
{

	// 테스트 Initialize
	// @Note 제거 할 것
	{
		FString AssetPath = TEXT("/Game/BP/DataAsset/Skill/Attack/DA_TestAttack_Common.DA_TestAttack_Common");
		UStaticSkillData* LoadedData = Cast<UStaticSkillData>(StaticLoadObject(UStaticSkillData::StaticClass(), nullptr, *AssetPath));

		if (LoadedData)
		{
			// 바로 객체를 사용
			AddSkillData(LoadedData);
		}
	}

	checkf(IsValid(GetOwnerModel()), TEXT("OwnerModel 없음"));
	checkf(mSkillData.IsValidIndex(SkillIndex), TEXT("잘못된 스킬 인덱스"));

	// 스킬을 기반으로 효과를 계산한다.
	TSoftObjectPtr<UStaticSkillData> SkillData = mSkillData[SkillIndex];
	checkf(SkillData.IsValid(), TEXT("잘못된 스킬"));
	checkf(!SkillData.IsPending(), TEXT("로드되어있지 않는 스킬"));

	TWeakObjectPtr<UBoardActorModel> Owner = Cast<UBoardActorModel>(GetOwnerModel());
	checkf(Owner.IsValid(), TEXT("보드 액터"));

	for (int32 i = 0; i < SkillData.Get()->mSkillMotionLayers.Num(); ++i)
	{
		const FSkillMotionLayer& SkillMotionLayer = SkillData.Get()->mSkillMotionLayers[i];

		// 스킬 포인트를 기반으로 Owner의 AS를 변경 시켜준다.
		int DamagePoint = SkillMotionLayer.mStaticSkillEffectLayers->GetPoint(Owner, SkillPoint);

		// 타겟을 가져옵니다.
		TArray<TWeakObjectPtr<UBoardActorModel>> TargetActors;
		ExtractTarget(TargetTiles, ETileLayerFlag::Unit, ETargetFilter::All, TargetActors);

		// 효과를 적용한다.
		for (int j = 0; j < TargetActors.Num(); ++j)
		{
			ApplyEffect(Owner, TargetActors[i], DamagePoint);
		}
	}

	return true;
}

void USkillComponentModel::HandelMovePoint(int32 MovePoint)
{
}

void USkillComponentModel::ApplyEffect(FTacticalEffectRequestContainer& TacticalEffectRequestContainer)
{
	TWeakObjectPtr<UBoardActorModel> BoardActor = Cast<UBoardActorModel>(GetOwnerModel());
	checkf(BoardActor.IsValid(), TEXT("보드 액터"));

	for (TMap<TObjectPtr<UBoardActorModel>, FBoardCombatTargetSnapshotData>::TIterator It = TacticalEffectRequestContainer.mTargetRequests.CreateIterator(); It; ++It)
	{
		// It.Key()와 It.Value()로 접근
		TObjectPtr<UBoardActorModel> Actor = It.Key();
		FBoardCombatTargetSnapshotData& Data = It.Value();

		// 적용
		// 각각에게 적용합니다.
		// @Note 나중에 TacticalEffect를 사용하여 효과를 적용하자.

		UE_LOG(LogTemp, Warning, TEXT("TestActor : %d"), Actor->GetUniqueID());


		for (int i = 0; i < Data.mTags.Num(); ++i)
		{
			for (TMap<FGameplayTag, int32>::TIterator iter = Data.mTags.CreateIterator(); iter; ++iter)
			{
				//float BaseValue = Actor->FindComponentModelByClass<UAttributeSetComponentModel>()->GetAttributeCurrentValue(iter.Key());
				//UE_LOG(LogTemp, Warning, TEXT("BaseValue : %f"), BaseValue);
				int32 Value = iter.Value();
				UE_LOG(LogTemp, Warning, TEXT("Value : %d"), Value);
				FName Name = iter.Key().GetTagName();
				UE_LOG(LogTemp, Warning, TEXT("TagName : %s"), *Name.ToString());
			}
		}
	}
}

void USkillComponentModel::ApplyEffect(TWeakObjectPtr<UBoardActorModel> BoardActor, TWeakObjectPtr<UBoardActorModel> Target, float DamagePoint)
{
	TestCalcualateDamage(BoardActor.Get()->FindComponentModelByClass<UAttributeSetComponentModel>(), Target.Get()->FindComponentModelByClass<UAttributeSetComponentModel>(), DamagePoint);
}

void USkillComponentModel::ExtractTarget(const TArray<FTileIndex>& TargetTile, ETileLayerFlag ActorFlag, ETargetFilter TargetFilter, OUT TArray<TWeakObjectPtr<UBoardActorModel>>& TargetActors)
{
	// @Note 서브시스템이 작동이 가능하다면 서브시스템에서 가져오자.


	// CombatModel을 가져옵니다.
	//USRPGCombatModel* CombatModel = GetWorldSubsystemModel<USRPGCombatModel>(this);
	//checkf(IsValid(CombatModel), TEXT("전투 모델이 비어있습니다."));

	// 서브 시스템에 접근하여 타일맵 모델을 가져온다.
	//TWeakObjectPtr<UTileMapModel> TMModel = CombatModel->GetTileMap();
	//checkf(TMModel.IsValid(), TEXT("타일맵 모델이 존재하지 않습니다."));
	// ===============================================================

	// @Note 유닛 모델 생성이 아닌 참조로 바꾸기
	for (int i = -2; i < TargetTile.Num(); ++i)
	{
		// 해당 타일에 유닛 모델을 가져온다.
		//TWeakObjectPtr<UBoardActorModel> TargetBoardModel = TMModel->GetActorOnTile<UBoardActorModel>(TargetTile[i], ActorFlag);

		// 더미 모델
		TWeakObjectPtr<UBoardActorModel> TargetBoardModel = NewObject<UUnitModel>();


		// 유닛이 없다면 반환한다.
		if (!TargetBoardModel.IsValid())
			continue;

		TargetActors.Add(TargetBoardModel);
	}
}

void USkillComponentModel::TestCalcualateDamage(TWeakObjectPtr<class UAttributeSetComponentModel> Attacker, TWeakObjectPtr<class UAttributeSetComponentModel> Defenser, float DamagePoint)
{
	// 공격자, 피격자의 AS가 모두 유효할 시 
	if (!(Attacker.IsValid() && Defenser.IsValid()))
		return;

	UE_LOG(LogTemp, Warning, TEXT("DamagePoint : %f"), DamagePoint);


	float MyDamage = DamagePoint; //Attacker.Get()->GetAttributeCurrentValue(UUnitAttributeSet::GetDamagePointAttribute());
	//float MyStrenth = Attacker.Get()->GetAttributeCurrentValue(UUnitAttributeSet::GetSkillPointAttribute());
	//float MyVulnerable = Attacker.Get()->GetAttributeCurrentValue(UUnitAttributeSet::GetSkillPointAttribute());

	float YouHP = 100; // Defenser.Get()->GetAttributeCurrentValue(UUnitAttributeSet::GetHPAttribute());
	float YouDefense = 5; // Defenser.Get()->GetAttributeCurrentValue(UUnitAttributeSet::GetDefensePointAttribute());
	//float YouWeak = Defenser.Get()->GetAttributeCurrentValue(UUnitAttributeSet::GetSkillPointAttribute());

	// 공격자의 스탯으로 계산
	float MyCaluDamage = MyDamage; //FMath::Max(0, (MyDamage + MyStrenth) * (MyVulnerable ? 0.75f : 1.0f));

	// 방어자의 스탯으로 재계산
	float YouDamage = FMath::Max(0, MyCaluDamage  - YouDefense);

	// 체력 감소
	// 추후 TacticalEffect로 변경
	//Defenser.Get()->SetAttributeBaseValue(UUnitAttributeSet::GetHPAttribute(), YouHP - YouDamage);
	UE_LOG(LogTemp, Warning, TEXT("CurHP : %f"), YouHP);

	YouHP = FMath::Max(0, YouHP - YouDamage);
	UE_LOG(LogTemp, Warning, TEXT("NextHP : %f"), YouHP);


	// 경감 감소
	// 추후 TacticalEffect로 변경
	//Defenser.Get()->SetAttributeBaseValue(UUnitAttributeSet::GetDefensePointAttribute(), YouDefense - MyCaluDamage);
	UE_LOG(LogTemp, Warning, TEXT("CurDefense : %f"), YouDefense);

	YouDefense = FMath::Max(0, YouDefense - MyCaluDamage);
	UE_LOG(LogTemp, Warning, TEXT("NextDefense : %f"), YouDefense);

	UE_LOG(LogTemp, Warning, TEXT("End"));

}
