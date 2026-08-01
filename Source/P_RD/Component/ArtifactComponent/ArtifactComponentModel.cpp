/*****************************************************************//**
 * @file   ArtifactComponentModel.cpp
 * @brief  아티펙트 컴포넌트 모델 구현
 * @author 이문환
 * @date   2026-07-22
 *********************************************************************/

#include "Component/ArtifactComponent/ArtifactComponentModel.h"

#include "Actor/ActorModel.h"
#include "Component/PassiveComponent/PassiveComponentModel.h"
#include "Component/AttributeComponent/AttributeSetComponentModel.h"
#include "DataAsset/ArtifactData/StaticArtifactData.h"
#include "DataAsset/PassiveData/StaticPassiveData.h"
#include "TAS/Passive/TacticalPassive.h"
#include "TAS/Effect/TacticalEffect.h"
#include "TAS/Effect/TacticalEffectContext.h"

void UArtifactComponentModel::Initialize()
{
	// 컴포넌트 등록 직후 호출되는 초기화 (현재 작업 없음)
}

void UArtifactComponentModel::Uninitialize()
{
	// 장착된 모든 아티펙트를 역순으로 해제 후 정리
	UPassiveComponentModel* PassiveComp = GetPassiveComponent();
	UAttributeSetComponentModel* AttrComp = GetAttributeComponent();
	for (int32 Index = mArtifacts.Num() - 1; Index >= 0; --Index)
	{
		UnequipAt(Index, PassiveComp, AttrComp);
	}
	mArtifacts.Reset();
}

bool UArtifactComponentModel::Equip(UStaticArtifactData* Data)
{
	// 다른 컴포넌트를 오너에서 찾아 내부 장착 함수에 위임
	return EquipInternal(Data, GetPassiveComponent(), GetAttributeComponent());
}

bool UArtifactComponentModel::Equip(const TSoftObjectPtr<UStaticArtifactData>& DataPtr)
{
	// 소프트 참조를 로드해서 장착
	return Equip(DataPtr.LoadSynchronous());
}

void UArtifactComponentModel::EquipFrom(const TArray<TSoftObjectPtr<UStaticArtifactData>>& List)
{
	for (const TSoftObjectPtr<UStaticArtifactData>& DataPtr : List)
	{
		Equip(DataPtr);
	}
}

void UArtifactComponentModel::EquipFrom(const TArray<FPrimaryAssetId>& List)
{
	for (const FPrimaryAssetId& AssetId : List)
	{
		UStaticArtifactData* StaticArtifactData = UStaticArtifactData::LoadByAssetId(AssetId);
		if (StaticArtifactData != nullptr)
		{
			Equip(StaticArtifactData);
		}
	}
}

bool UArtifactComponentModel::Unequip(UStaticArtifactData* Data)
{
	// 다른 컴포넌트를 오너에서 찾아 내부 장착해제 함수에 위임
	return UnequipInternal(Data, GetPassiveComponent(), GetAttributeComponent());
}

const FArtifactEntry* UArtifactComponentModel::GetEquipped(const UStaticArtifactData* Data) const
{
	// 동일 데이터로 장착된 첫 엔트리 탐색
	return mArtifacts.FindByPredicate([Data](const FArtifactEntry& Entry) { return Entry.mData == Data; });
}

bool UArtifactComponentModel::EquipInternal(UStaticArtifactData* Data, UPassiveComponentModel* PassiveComp, UAttributeSetComponentModel* AttrComp)
{
	if (Data == nullptr)
	{
		return false;
	}

	FArtifactEntry Entry;
	Entry.mData = Data;

	// 패시브 설치: 아티펙트가 참조하는 패시브 DA를 로드해 PassiveComponent에 설치
	if (PassiveComp != nullptr)
	{
		for (const TSoftObjectPtr<UStaticPassiveData>& PassivePtr : Data->mStaticPassiveData)
		{
			UStaticPassiveData* PassiveData = PassivePtr.LoadSynchronous();
			if (PassiveData == nullptr)
			{
				continue;
			}
			if (UTacticalPassive* Installed = PassiveComp->AddPassiveFromData(PassiveData))
			{
				Entry.mInstalledPassives.Add(Installed);
			}
		}
	}

	// 아티펙트 고유 스탯이 있으면 무한 이펙트로 자신에게 적용 (해제 시 핸들로 제거)
	if (AttrComp != nullptr && Data->mStatModifiers.Num() > 0)
	{
		// 데이터의 수정자들을 런타임 이펙트 인스턴스에 담아 무한 정책으로 구성
		UTacticalEffect* StatEffect = NewObject<UTacticalEffect>(this);
		StatEffect->mModifiers = Data->mStatModifiers;
		StatEffect->mDurationPolicy = ETacticalEffectDurationType::Infinite;

		// 소유자를 시전자로 컨텍스트 구성 후 self 적용, 인스턴스·핸들 보관
		UTacticalEffectContext* EffectContext = AttrComp->MakeEffectContext();
		EffectContext->SetAttributeSetComponentModel(AttrComp);

		FTacticalEffectSpec Spec(StatEffect, EffectContext);
		Entry.mStatEffect = StatEffect;
		Entry.mStatEffectHandle = AttrComp->ApplyTacticalEffectSpecToSelf(Spec);
	}

	mArtifacts.Add(MoveTemp(Entry));
	return true;
}

bool UArtifactComponentModel::UnequipInternal(UStaticArtifactData* Data, UPassiveComponentModel* PassiveComp, UAttributeSetComponentModel* AttrComp)
{
	// 동일 데이터로 장착된 첫 엔트리를 찾아 인덱스 해제에 위임
	const int32 Index = mArtifacts.IndexOfByPredicate([Data](const FArtifactEntry& Entry) { return Entry.mData == Data; });
	if (Index == INDEX_NONE)
	{
		return false;
	}
	return UnequipAt(Index, PassiveComp, AttrComp);
}

bool UArtifactComponentModel::UnequipAt(int32 Index, UPassiveComponentModel* PassiveComp, UAttributeSetComponentModel* AttrComp)
{
	if (mArtifacts.IsValidIndex(Index) == false)
	{
		return false;
	}
	FArtifactEntry& Entry = mArtifacts[Index];

	// 설치한 패시브 해제 (적용 중 이펙트가 있으면 내리고 보유 목록에서 제거)
	if (PassiveComp != nullptr)
	{
		for (const TObjectPtr<UTacticalPassive>& Installed : Entry.mInstalledPassives)
		{
			if (Installed != nullptr)
			{
				Installed->DeactivatePassive();
				PassiveComp->RemovePassive(Installed);
			}
		}
	}

	// 아티펙트 고유 스탯 이펙트 제거 (적용돼 있던 경우)
	if (Entry.mStatEffectHandle.IsValid() && AttrComp != nullptr)
	{
		AttrComp->RemoveActiveTacticalEffect(Entry.mStatEffectHandle);
	}
	Entry.mStatEffectHandle.Reset();

	mArtifacts.RemoveAt(Index);
	return true;
}

UPassiveComponentModel* UArtifactComponentModel::GetPassiveComponent() const
{
	// 생성 순서를 가정하지 않도록 캐시 없이 매번 오너에서 조회
	UActorModel* Owner = GetOwnerModel();
	return Owner != nullptr ? Owner->FindComponentModelByClass<UPassiveComponentModel>() : nullptr;
}

UAttributeSetComponentModel* UArtifactComponentModel::GetAttributeComponent() const
{
	// 생성 순서를 가정하지 않도록 캐시 없이 매번 오너에서 조회
	UActorModel* Owner = GetOwnerModel();
	return Owner != nullptr ? Owner->FindComponentModelByClass<UAttributeSetComponentModel>() : nullptr;
}
