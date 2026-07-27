/*****************************************************************//**
 * @file   PartyArtifactComponentModel.cpp
 * @brief  파티 보유 아티펙트 원본/배포 컴포넌트 모델 정의 헤더
 * @author 이문환
 * @date   2026-07-23
 *********************************************************************/

#include "Component/ArtifactComponent/PartyArtifactComponentModel.h"

#include "Component/ArtifactComponent/ArtifactComponentModel.h"
#include "DataAsset/ArtifactData/StaticArtifactData.h"

#include "Pawn/Player/PlayerUnitModel.h"
#include "Actor/Party/PartyModel.h"

bool UPartyArtifactComponentModel::AddArtifact(UStaticArtifactData* Data)
{
	if (Data == nullptr)
	{
		return false;
	}

	// 파티 목록에 추가
	mPartyArtifacts.Add(Data);

	// 파티 구성원 전원에 장착
	TArray<TObjectPtr<UPlayerUnitModel>>& PartyMembers = GetPartyMembers();
	for (TObjectPtr<UPlayerUnitModel>& PartyMember : PartyMembers)
	{
		if (PartyMember == nullptr)
		{
			continue;
		}
		if (UArtifactComponentModel* ArtifactComp = PartyMember->GetArtifactComponentModel())
		{
			ArtifactComp->Equip(Data);
		}
	}
	return true;
}

bool UPartyArtifactComponentModel::AddArtifact(const FPrimaryAssetId& ArtifactId)
{
	UStaticArtifactData* Data = UStaticArtifactData::LoadByAssetId(ArtifactId);
	if (Data == nullptr)
	{
		return false;
	}

	return AddArtifact(Data);
}

bool UPartyArtifactComponentModel::RemoveArtifact(UStaticArtifactData* Data)
{
	// 파티 목록에서 하나만 제거 (미보유면 실패)
	const int32 Index = mPartyArtifacts.Find(Data);
	if (Index == INDEX_NONE)
	{
		return false;
	}
	mPartyArtifacts.RemoveAt(Index);

	// 파티 구성원 전원에서 해제
	TArray<TObjectPtr<UPlayerUnitModel>>& PartyMembers = GetPartyMembers();
	for (UPlayerUnitModel* PartyMember : PartyMembers)
	{
		if (PartyMember == nullptr)
		{
			continue;
		}
		if (UArtifactComponentModel* ArtifactComp = PartyMember->GetArtifactComponentModel())
		{
			ArtifactComp->Unequip(Data);
		}
	}
	return true;
}

void UPartyArtifactComponentModel::EquipArtifactsTo(UPlayerUnitModel* PartyMember) const
{
	if (PartyMember == nullptr)
	{
		return;
	}

	UArtifactComponentModel* ArtifactComp = PartyMember->GetArtifactComponentModel();
	if (ArtifactComp == nullptr)
	{
		return;
	}

	checkf(ArtifactComp->GetArtifacts().Num() == 0, TEXT("새로 합류한 유닛은 아티팩트를 갖고 있으면 안 됨"));

	// 파티 보유 아티펙트 전체를 순서대로 장착
	for (const TObjectPtr<UStaticArtifactData>& Data : mPartyArtifacts)
	{
		ArtifactComp->Equip(Data.Get());
	}
}

void UPartyArtifactComponentModel::RestoreFrom(const TArray<FPrimaryAssetId>& ArtifactIds)
{
	checkf(mPartyArtifacts.Num() == 0, TEXT("초기화된 경우에만 로드 가능"));

	for (const FPrimaryAssetId& ArtifactId : ArtifactIds)
	{
		UStaticArtifactData* Data = UStaticArtifactData::LoadByAssetId(ArtifactId);
		if (Data == nullptr)
		{
			UE_LOG(LogRD, Warning, TEXT("아티팩트 복원 실패: %s (에셋 없음)"), *ArtifactId.ToString());
			continue;
		}
		mPartyArtifacts.Add(Data);
	}
}

TArray<FPrimaryAssetId> UPartyArtifactComponentModel::GetPartyArtifactIds() const
{
	TArray<FPrimaryAssetId> ArtifactIds;
	ArtifactIds.Reserve(mPartyArtifacts.Num());

	for (const TObjectPtr<UStaticArtifactData>& Data : mPartyArtifacts)
	{
		checkf(Data != nullptr, TEXT("파티 아티팩트 목록에 nullptr 존재"));
		ArtifactIds.Add(Data->GetPrimaryAssetId());
	}
	return ArtifactIds;
}

TArray<TObjectPtr<UPlayerUnitModel>>& UPartyArtifactComponentModel::GetPartyMembers()
{
	return GetOwnerModel<UPartyModel>()->GetPlayerUnitModels();
}

const TArray<TObjectPtr<UPlayerUnitModel>>& UPartyArtifactComponentModel::GetPartyMembers() const
{
	return GetOwnerModel<UPartyModel>()->GetPlayerUnitModels();
}
