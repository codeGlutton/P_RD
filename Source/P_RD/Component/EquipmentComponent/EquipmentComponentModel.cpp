/*****************************************************************//**
 * @file   EquipmentComponentModel.cpp
 * @brief  장비 보유·장착 컴포넌트 모델 구현
 * @author 이문환
 * @date   2026-06-28
 *********************************************************************/

#include "Component/EquipmentComponent/EquipmentComponentModel.h"

#include "Actor/ActorModel.h"
#include "Component/PassiveComponent/PassiveComponentModel.h"
#include "Component/AttributeComponent/AttributeSetComponentModel.h"
#include "DataAsset/EquipmentData/StaticEquipmentData.h"
#include "DataAsset/PassiveData/StaticPassiveData.h"
#include "TAS/Passive/TacticalPassive.h"

void UEquipmentComponentModel::Initialize()
{
	// 부착 직후 초기화 (현재 별도 작업 없음)
}

void UEquipmentComponentModel::Uninitialize()
{
	// 보유한 모든 슬롯 해제 후 정리
	TArray<EEquipmentType> Slots;
	mEquipped.GetKeys(Slots);
	for (EEquipmentType Slot : Slots)
	{
		Unequip(Slot);
	}
	mEquipped.Reset();
}

bool UEquipmentComponentModel::Equip(UStaticEquipmentData* Data)
{
	// 형제 컴포넌트를 오너에서 찾아 코어 로직 위임
	return EquipInternal(Data, GetPassiveComponent(), GetAttributeComponent());
}

bool UEquipmentComponentModel::Equip(const TSoftObjectPtr<UStaticEquipmentData>& DataPtr)
{
	// 소프트 참조를 로드해 장착
	return Equip(DataPtr.LoadSynchronous());
}

void UEquipmentComponentModel::EquipFrom(const TArray<TSoftObjectPtr<UStaticEquipmentData>>& List)
{
	for (const TSoftObjectPtr<UStaticEquipmentData>& DataPtr : List)
	{
		Equip(DataPtr);
	}
}

bool UEquipmentComponentModel::Unequip(EEquipmentType Slot)
{
	// 형제 컴포넌트를 오너에서 찾아 코어 로직 위임
	return UnequipInternal(Slot, GetPassiveComponent(), GetAttributeComponent());
}

const FEquippedEntry* UEquipmentComponentModel::GetEquipped(EEquipmentType Slot) const
{
	return mEquipped.Find(Slot);
}

bool UEquipmentComponentModel::EquipInternal(UStaticEquipmentData* Data, UPassiveComponentModel* PassiveComp, UAttributeSetComponentModel* AttrComp)
{
	if (Data == nullptr)
	{
		return false;
	}

	// 슬롯 결정, 이미 점유 중이면 먼저 해제(교체)
	const EEquipmentType Slot = Data->GetEquipmentType();
	if (Slot == EEquipmentType::Count)
	{
		return false;
	}
	if (mEquipped.Contains(Slot))
	{
		UnequipInternal(Slot, PassiveComp, AttrComp);
	}

	FEquippedEntry Entry;
	Entry.mData = Data;

	// 패시브 설치: 장비가 참조하는 패시브 DA를 로드해 PassiveComponent에 설치
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

	// TODO: [3단계] 장비 고유 스탯(mStatModifiers)을 무한 이펙트로 self 적용 후 Entry.mStatEffectHandle 저장

	mEquipped.Add(Slot, MoveTemp(Entry));
	return true;
}

bool UEquipmentComponentModel::UnequipInternal(EEquipmentType Slot, UPassiveComponentModel* PassiveComp, UAttributeSetComponentModel* AttrComp)
{
	FEquippedEntry* Entry = mEquipped.Find(Slot);
	if (Entry == nullptr)
	{
		return false;
	}

	// 설치한 패시브 해제 (적용 중 이펙트가 있으면 내리고 보유 목록에서 제거)
	if (PassiveComp != nullptr)
	{
		for (const TObjectPtr<UTacticalPassive>& Installed : Entry->mInstalledPassives)
		{
			if (Installed != nullptr)
			{
				Installed->DeactivatePassive();
				PassiveComp->RemovePassive(Installed);
			}
		}
	}

	// 장비 고유 스탯 이펙트 제거 (3단계에서 적용된 경우)
	if (Entry->mStatEffectHandle.IsValid() && AttrComp != nullptr)
	{
		AttrComp->RemoveActiveTacticalEffect(Entry->mStatEffectHandle);
	}
	Entry->mStatEffectHandle.Reset();

	mEquipped.Remove(Slot);
	return true;
}

UPassiveComponentModel* UEquipmentComponentModel::GetPassiveComponent() const
{
	UActorModel* Owner = GetOwnerModel();
	return Owner != nullptr ? Owner->FindComponentModelByClass<UPassiveComponentModel>() : nullptr;
}

UAttributeSetComponentModel* UEquipmentComponentModel::GetAttributeComponent() const
{
	UActorModel* Owner = GetOwnerModel();
	return Owner != nullptr ? Owner->FindComponentModelByClass<UAttributeSetComponentModel>() : nullptr;
}
