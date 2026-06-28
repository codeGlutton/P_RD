#include "Pawn/UnitModel.h"
#include "Setting/GameTeamType.h"

#include "Component/AttributeComponent/AttributeSetComponentModel.h"
#include "Component/SkillComponent/SkillComponentModel.h"
#include "Component/PassiveComponent/PassiveComponentModel.h"
#include "Component/EquipmentComponent/EquipmentComponentModel.h"

#include "DataAsset/UnitSpawnData/StaticUnitSpawnData.h"

#include "Singleton/WorldSubsystem/TacticalFrameworkModel.h"
#include "Singleton/WorldSubsystem/TacticalFrameworkSubsystem.h"

UUnitModel::UUnitModel() : mTeamId(EGameTeamType::AllNeutral)
{
	mAttributeCompModel = CreateDefaultSubobject<UAttributeSetComponentModel>(TEXT("AttributeSetComponentModel"));
	mSkillCompModel = CreateDefaultSubobject<USkillComponentModel>(TEXT("SkillComponentModel"));
	mPassiveCompModel = CreateDefaultSubobject<UPassiveComponentModel>(TEXT("PassiveComponentModel"));
	mEquipmentCompModel = CreateDefaultSubobject<UEquipmentComponentModel>(TEXT("EquipmentComponentModel"));

	mTileLayerFlags = StaticCast<int32>(ETileLayerFlag::Unit);
	mBlockLayerFlags = StaticCast<int32>(ETileLayerFlag::Unit | ETileLayerFlag::Obstacle);
	mReplaceLayerFlags = StaticCast<int32>(ETileLayerFlag::None);
	mOverlayLayerPriority = 0;
}

void UUnitModel::PostInitializeComponentModels()
{
	Super::PostInitializeComponentModels();

	UTacticalFrameworkSubsystem* TacticalFrameworkSubsystem = GetWorld()->GetSubsystem<UTacticalFrameworkSubsystem>();
	checkf(TacticalFrameworkSubsystem != nullptr, TEXT("전략 프레임워크 서브시스템 nullptr"));

	UTacticalFrameworkModel* TacticalFrameworkModel = TacticalFrameworkSubsystem->GetModel<UTacticalFrameworkModel>();
	checkf(TacticalFrameworkModel != nullptr, TEXT("전략 프레임워크 모델 nullptr"));

	TacticalFrameworkModel->GetAttributeSetInitter()->InitAttributeSetDefaults(GetAttributeComponentModel(), GetBoardActorKeyName(), GetDifficulty(), true);

	// 스폰 데이터에 지정된 장비를 일괄 장착
	if (UStaticUnitSpawnData* UnitSpawn = Cast<UStaticUnitSpawnData>(mStaticSpawnData))
	{
		GetEquipmentComponentModel()->EquipFrom(UnitSpawn->mEquipmentDatas);
	}
}

void UUnitModel::SetGenericTeamId(const FGenericTeamId& TeamID)
{
	mTeamId = TeamID;
}

FGenericTeamId UUnitModel::GetGenericTeamId() const
{
	return mTeamId;
}

UAttributeSetComponentModel* UUnitModel::GetAttributeComponentModel() const
{
	return mAttributeCompModel;
}

USkillComponentModel* UUnitModel::GetSkillComponentModel() const
{
	return mSkillCompModel;
}

UPassiveComponentModel* UUnitModel::GetPassiveComponentModel() const
{
	return mPassiveCompModel;
}

UEquipmentComponentModel* UUnitModel::GetEquipmentComponentModel() const
{
	return mEquipmentCompModel;
}

