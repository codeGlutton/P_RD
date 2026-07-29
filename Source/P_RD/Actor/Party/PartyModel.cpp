#include "Actor/Party/PartyModel.h"
#include "Pawn/Player/PlayerUnitModel.h"

#include "Singleton/WorldSubsystem/TacticalFrameworkModel.h"
#include "AttributeSet/PartyAttributeSet.h"

#include "Component/AttributeComponent/AttributeSetComponentModel.h"
#include "Component/ArtifactComponent/PartyArtifactComponentModel.h"

UPartyModel::UPartyModel()
{
	mPlayerUnitModels.Init(nullptr, 3);

	mPartyAttributeSet = CreateDefaultSubobject<UPartyAttributeSet>(TEXT("PartyAttributeSet"));

	mAttributeCompModel = CreateDefaultSubobject<UAttributeSetComponentModel>(TEXT("AttributeCompModel"));
	mPartyArtifactCompModel = CreateDefaultSubobject<UPartyArtifactComponentModel>(TEXT("PartyArtifactCompModel"));
}

void UPartyModel::SetPlayerUnitModel(int32 PlayerIndex, UPlayerUnitModel* PlayerUnitModel)
{
	checkf(mPlayerUnitModels.IsValidIndex(PlayerIndex) == true, TEXT("적절하지 않은 Player Index 대입"));

	UPlayerUnitModel* PrePlayerUnitModel = mPlayerUnitModels[PlayerIndex];
	mPlayerUnitModels[PlayerIndex] = PlayerUnitModel;
	if (PlayerUnitModel != nullptr)
	{
		PlayerUnitModel->SetOwnerParty(this);
	}

	OnChangePartyPlayer.Broadcast(PlayerIndex, PrePlayerUnitModel, PlayerUnitModel);
}

void UPartyModel::SetDifficulty(int32 Difficulty)
{
	mDifficulty = Difficulty;

	UTacticalFrameworkModel* TacticalFrameworkModel = GetWorldSubsystemModel<UTacticalFrameworkModel>(this);
	checkf(TacticalFrameworkModel != nullptr, TEXT("전략 프레임워크 모델 nullptr"));

	TacticalFrameworkModel->GetAttributeSetInitter()->InitAttributeSetDefaults(GetAttributeComponentModel(), UPartyAttributeSet::KeyName, GetDifficulty(), true);
}

TArray<TObjectPtr<UPlayerUnitModel>>& UPartyModel::GetPlayerUnitModels()
{
	return mPlayerUnitModels;
}

const TArray<TObjectPtr<UPlayerUnitModel>>& UPartyModel::GetPlayerUnitModels() const
{
	return mPlayerUnitModels;
}

UPlayerUnitModel* UPartyModel::GetPlayerUnitModel(int32 PlayerIndex)
{
	if (mPlayerUnitModels.IsValidIndex(PlayerIndex) == false)
	{
		return nullptr;
	}
	return mPlayerUnitModels[PlayerIndex];
}

const UPlayerUnitModel* UPartyModel::GetPlayerUnitModel(int32 PlayerIndex) const
{
	if (mPlayerUnitModels.IsValidIndex(PlayerIndex) == false)
	{
		return nullptr;
	}
	return mPlayerUnitModels[PlayerIndex];
}

UAttributeSetComponentModel* UPartyModel::GetAttributeComponentModel() const
{
	return mAttributeCompModel;
}
UPartyArtifactComponentModel* UPartyModel::GetPartyArtifactComponentModel() const
{
	return mPartyArtifactCompModel;
}

int32 UPartyModel::GetDifficulty() const
{
	return mDifficulty;
}

