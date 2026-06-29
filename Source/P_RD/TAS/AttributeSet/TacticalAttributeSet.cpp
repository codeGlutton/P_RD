#include "TAS/AttributeSet/TacticalAttributeSet.h"
#include "UObject/UObjectIterator.h"

#include "Engine/CurveTable.h"

#include "Singleton/WorldSubsystem/TacticalFrameworkModel.h"

#include "Net/Core/PushModel/PushModel.h"
#include "UObject/UObjectThreadContext.h"

#include "TAS/Aggregator/TacticalAggregator.h"

#include "Component/AttributeComponent/AttributeSetComponentModel.h"
#include "Actor/ActorModel.h"
#include "Actor/BoardActor/BoardCombatTarget.h"

float FTacticalAttributeData::GetCurrentValue() const
{
	return mCurrentValue;
}

void FTacticalAttributeData::SetCurrentValue(float NewValue)
{
	mCurrentValue = NewValue;
}

float FTacticalAttributeData::GetBaseValue() const
{
	return mBaseValue;
}

void FTacticalAttributeData::SetBaseValue(float NewValue)
{
	mBaseValue = NewValue;
}

FTacticalAttribute::FTacticalAttribute(FProperty *NewProperty)
{
	if (FTacticalAttribute::IsSupportedProperty(NewProperty) == true)
	{
		mAttribute = NewProperty;
		mAttributeOwner = mAttribute->GetOwnerStruct();
		mAttribute->GetName(mAttributeName);
	}
	else
	{
		ensureMsgf(NewProperty == nullptr, TEXT("Tactical Attribute로 지정된 속성 '%s'이 '%s' 타입에 해당하지 않음"), *NewProperty->GetName(), *NewProperty->GetClass()->GetName());

		mAttribute = nullptr;
		mAttributeOwner = nullptr;
		mAttributeName = "";
	}
}

void FTacticalAttribute::SetNumericValueChecked(float& NewValue, class UTacticalAttributeSet* Dest) const
{
	check(Dest != nullptr);

	FNumericProperty* NumericProperty = CastField<FNumericProperty>(mAttribute.Get());
	float OldValue = 0.f;
	if (NumericProperty != nullptr)
	{
		void* ValuePtr = NumericProperty->ContainerPtrToValuePtr<void>(Dest);
		OldValue = *static_cast<float*>(ValuePtr);
		Dest->PreAttributeChange(*this, NewValue);
		NumericProperty->SetFloatingPointPropertyValue(ValuePtr, NewValue);
		Dest->PostAttributeChange(*this, OldValue, NewValue);
	}
	else if (IsTacticalAttributeDataProperty(mAttribute.Get()) == true)
	{
		FStructProperty* StructProperty = CastField<FStructProperty>(mAttribute.Get());
		check(StructProperty != nullptr);
		FTacticalAttributeData* DataPtr = StructProperty->ContainerPtrToValuePtr<FTacticalAttributeData>(Dest);
		check(DataPtr != nullptr);
		OldValue = DataPtr->GetCurrentValue();
		Dest->PreAttributeChange(*this, NewValue);
		DataPtr->SetCurrentValue(NewValue);
		Dest->PostAttributeChange(*this, OldValue, DataPtr->GetCurrentValue());
	}
	else
	{
		check(false);
	}
}

float FTacticalAttribute::GetNumericValue(const UTacticalAttributeSet* Src) const
{
	const FNumericProperty* const NumericProperty = CastField<FNumericProperty>(mAttribute.Get());
	if (NumericProperty != nullptr)
	{
		const void* ValuePtr = NumericProperty->ContainerPtrToValuePtr<void>(Src);
		return NumericProperty->GetFloatingPointPropertyValue(ValuePtr);
	}
	else if (IsTacticalAttributeDataProperty(mAttribute.Get()) == true)
	{
		const FStructProperty* StructProperty = CastField<FStructProperty>(mAttribute.Get());
		check(StructProperty != nullptr);
		const FTacticalAttributeData* DataPtr = StructProperty->ContainerPtrToValuePtr<FTacticalAttributeData>(Src);
		if (ensure(DataPtr != nullptr) == true)
		{
			return DataPtr->GetCurrentValue();
		}
	}

	return 0.f;
}

float FTacticalAttribute::GetNumericValueChecked(const UTacticalAttributeSet* Src) const
{
	FNumericProperty* NumericProperty = CastField<FNumericProperty>(mAttribute.Get());
	if (NumericProperty != nullptr)
	{
		const void* ValuePtr = NumericProperty->ContainerPtrToValuePtr<void>(Src);
		return NumericProperty->GetFloatingPointPropertyValue(ValuePtr);
	}
	else if (IsTacticalAttributeDataProperty(mAttribute.Get()) == true)
	{
		const FStructProperty* StructProperty = CastField<FStructProperty>(mAttribute.Get());
		check(StructProperty != nullptr);
		const FTacticalAttributeData* DataPtr = StructProperty->ContainerPtrToValuePtr<FTacticalAttributeData>(Src);
		if (ensure(DataPtr != nullptr) == true)
		{
			return DataPtr->GetCurrentValue();
		}
	}

	check(false);
	return 0.f;
}

const FTacticalAttributeData* FTacticalAttribute::GetTacticalAttributeData(const UTacticalAttributeSet* Src) const
{
	if (Src != nullptr && IsTacticalAttributeDataProperty(mAttribute.Get()) == true)
	{
		FStructProperty* StructProperty = CastField<FStructProperty>(mAttribute.Get());
		check(StructProperty != nullptr);
		return StructProperty->ContainerPtrToValuePtr<FTacticalAttributeData>(Src);
	}

	return nullptr;
}

const FTacticalAttributeData* FTacticalAttribute::GetTacticalAttributeDataChecked(const UTacticalAttributeSet* Src) const
{
	if (Src != nullptr && IsTacticalAttributeDataProperty(mAttribute.Get()) == true)
	{
		FStructProperty* StructProperty = CastField<FStructProperty>(mAttribute.Get());
		check(StructProperty != nullptr);
		return StructProperty->ContainerPtrToValuePtr<FTacticalAttributeData>(Src);
	}

	check(false);
	return nullptr;
}

FTacticalAttributeData* FTacticalAttribute::GetTacticalAttributeData(UTacticalAttributeSet* Src) const
{
	if (Src != nullptr && IsTacticalAttributeDataProperty(mAttribute.Get()) == true)
	{
		FStructProperty* StructProperty = CastField<FStructProperty>(mAttribute.Get());
		check(StructProperty != nullptr);
		return StructProperty->ContainerPtrToValuePtr<FTacticalAttributeData>(Src);
	}

	return nullptr;
}

FTacticalAttributeData* FTacticalAttribute::GetTacticalAttributeDataChecked(UTacticalAttributeSet* Src) const
{
	if (Src != nullptr && IsTacticalAttributeDataProperty(mAttribute.Get()) == true)
	{
		FStructProperty* StructProperty = CastField<FStructProperty>(mAttribute.Get());
		check(StructProperty != nullptr);
		return StructProperty->ContainerPtrToValuePtr<FTacticalAttributeData>(Src);
	}

	check(false);
	return nullptr;
}

bool FTacticalAttribute::IsSystemAttribute() const
{
	return GetAttributeSetClass()->IsChildOf(UAttributeSetComponentModel::StaticClass());
}

bool FTacticalAttribute::IsSupportedProperty(const FProperty* Prop)
{
	return Prop != nullptr && (FTacticalAttribute::IsTacticalAttributeDataProperty(Prop) || (Prop->IsA<FNumericProperty>() && CastField<FNumericProperty>(Prop)->IsFloatingPoint()));
}

bool FTacticalAttribute::IsTacticalAttributeDataProperty(const FProperty* Property)
{
	const FStructProperty* StructProp = CastField<FStructProperty>(Property);
	if (StructProp != nullptr)
	{
		const UStruct* Struct = StructProp->Struct;
		if (Struct != nullptr && Struct->IsChildOf(FTacticalAttributeData::StaticStruct()) == true)
		{
			return true;
		}
	}

	return false;
}

#if WITH_EDITORONLY_DATA
void FTacticalAttribute::PostSerialize(const FArchive& Ar)
{
	if (Ar.IsLoading() == true && Ar.IsPersistent() == true && Ar.HasAnyPortFlags(PPF_Duplicate | PPF_DuplicateForPIE) == false)
	{
		const FString PathName = mAttribute.ToString();
		const FString RedirectedPathName = FFieldPathProperty::RedirectFieldPathName(PathName);
		if (!RedirectedPathName.Equals(PathName) == true)
		{
			FString NewAttributeOwner;
			FString NewAttributeName;
			if (RedirectedPathName.Split(":", &NewAttributeOwner, &NewAttributeName))
			{
				const UStruct* NewClass = FindObject<UStruct>(nullptr, *NewAttributeOwner);
				mAttribute = FindFProperty<FProperty>(NewClass, *NewAttributeName);

				FUObjectSerializeContext* LoadContext = FUObjectThreadContext::Get().GetSerializeContext();
				const FString AssetName = (LoadContext && LoadContext->SerializedObject) ? LoadContext->SerializedObject->GetPathName() : TEXT("Unknown Object");
			}
		}

		if (mAttribute.Get())
		{
			mAttributeOwner = mAttribute->GetOwnerStruct();
			mAttribute->GetName(mAttributeName);
		}
		else if (mAttributeName.IsEmpty() == false)
		{
			if (mAttributeOwner != nullptr)
			{
				mAttribute = FindFProperty<FProperty>(mAttributeOwner, *mAttributeName);
			}

			if (mAttribute.Get() == nullptr)
			{
				FUObjectSerializeContext* LoadContext = FUObjectThreadContext::Get().GetSerializeContext();
				const FString AssetName = (LoadContext && LoadContext->SerializedObject) ? LoadContext->SerializedObject->GetPathName() : TEXT("Unknown Object");
				const FString OwnerName = mAttributeOwner ? mAttributeOwner->GetName() : TEXT("NONE");
			}
		}
	}
	if (Ar.IsSaving() == true && IsValid() == true)
	{
		Ar.MarkSearchableName(FTacticalAttribute::StaticStruct(), FName(FString::Printf(TEXT("%s.%s"), *GetUProperty()->GetOwnerVariant().GetName(), *GetUProperty()->GetName())));
	}
}
#endif

void FTacticalAttribute::GetAllAttributeProperties(TArray<FProperty*>& OutProperties, FString FilterMetaStr, bool UseEditorOnlyData)
{
	for (TObjectIterator<UClass> ClassIt; ClassIt; ++ClassIt)
	{
		UClass *Class = *ClassIt;
		if (Class->IsChildOf(UTacticalAttributeSet::StaticClass()) == true
#if WITH_EDITORONLY_DATA
			&& Class->ClassGeneratedBy == nullptr
#endif
			)
		{
			if (UseEditorOnlyData == true)
			{
				#if WITH_EDITOR
				if (Class->HasMetaData(TEXT("HideInDetailsView")) == true)
				{
					continue;
				}
				#endif
			}

			for (TFieldIterator<FProperty> PropertyIt(Class, EFieldIteratorFlags::ExcludeSuper); PropertyIt; ++PropertyIt)
			{
				FProperty* Property = *PropertyIt;

				if (UseEditorOnlyData == true)
				{
					#if WITH_EDITOR
					if (FilterMetaStr.IsEmpty() == false && Property->HasMetaData(*FilterMetaStr) == true)
					{
						continue;
					}

					if (Property->HasMetaData(TEXT("HideInDetailsView")) == true)
					{
						continue;
					}
					#endif
				}
				
				OutProperties.Add(Property);
			}
		}

#if WITH_EDITOR
		if (UseEditorOnlyData == true)
		{
			if (Class->IsChildOf(UAttributeSetComponentModel::StaticClass()) && !Class->ClassGeneratedBy)
			{
				for (TFieldIterator<FProperty> PropertyIt(Class, EFieldIteratorFlags::ExcludeSuper); PropertyIt; ++PropertyIt)
				{
					FProperty* Property = *PropertyIt;

					if (Property->HasMetaData(TEXT("SystemGameplayAttribute")) == false)
					{
						continue;
					}
					OutProperties.Add(Property);
				}
			}
		}
#endif
	}
}

UTacticalAttributeSet::UTacticalAttributeSet(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{

}

void UTacticalAttributeSet::GetAttributesFromSetClass(const TSubclassOf<UTacticalAttributeSet>& AttributeSetClass, TArray<FTacticalAttribute>& Attributes)
{
	for (TFieldIterator<FProperty> It(AttributeSetClass); It; ++It)
	{
		if (FTacticalAttribute::IsSupportedProperty(*It) == true)
		{
			Attributes.Add(FTacticalAttribute(*It));
		}
	}
}

void UTacticalAttributeSet::InitFromMetaDataTable(const UDataTable* DataTable)
{
	static const FString Context = FString(TEXT("UTacticalAttributeSet::InitFromMetaDataTable"));

	for (TFieldIterator<FProperty> It(GetClass(), EFieldIteratorFlags::IncludeSuper); It; ++It)
	{
		FProperty* Property = *It;

		if (FTacticalAttribute::IsSupportedProperty(Property) == true)
		{
			FString RowNameStr = FString::Printf(TEXT("%s.%s"), *Property->GetOwnerVariant().GetName(), *Property->GetName());
			if (FAttributeMetaData* MetaData = DataTable->FindRow<FAttributeMetaData>(FName(*RowNameStr), Context, false))
			{
				FNumericProperty* NumericProperty = CastField<FNumericProperty>(Property);
				if (NumericProperty != nullptr)
				{
					check(NumericProperty->IsFloatingPoint() == true);
					void* Data = NumericProperty->ContainerPtrToValuePtr<void>(this);
					NumericProperty->SetFloatingPointPropertyValue(Data, MetaData->BaseValue);
				}
				else if (FTacticalAttribute::IsTacticalAttributeDataProperty(Property) == true)
				{
					FStructProperty* StructProperty = CastField<FStructProperty>(Property);
					check(StructProperty != nullptr);
					FTacticalAttributeData* DataPtr = StructProperty->ContainerPtrToValuePtr<FTacticalAttributeData>(this);
					check(DataPtr != nullptr);
					DataPtr->SetBaseValue(MetaData->BaseValue);
					DataPtr->SetCurrentValue(MetaData->BaseValue);
				}
			}
		}
	}
}

UActorModel* UTacticalAttributeSet::GetOwningActor() const
{
	return Cast<UActorModel>(GetOuter());
}

UAttributeSetComponentModel* UTacticalAttributeSet::GetOwningAttributeSetComponentModel() const
{
	UActorModel* ActorModel = GetOwningActor();

	const IBoardCombatTarget* ASI = Cast<IBoardCombatTarget>(ActorModel);
	if (ASI != nullptr)
	{
		return ASI->GetAttributeComponentModel();
	}

	return ActorModel ? ActorModel->FindComponentModelByClass<UAttributeSetComponentModel>() : nullptr;
}

UAttributeSetComponentModel* UTacticalAttributeSet::GetOwningAttributeSetComponentModelChecked() const
{
	UAttributeSetComponentModel* Result = GetOwningAttributeSetComponentModel();
	check(Result != nullptr);
	return Result;
}

void UTacticalAttributeSet::CaptureAllAttributes(FBoardCombatTargetSnapshotData& Snapshot) const
{
	for (TFieldIterator<FProperty> It(GetClass()); It; ++It)
	{
		FProperty* Property = *It;

		if (FTacticalAttribute::IsTacticalAttributeDataProperty(Property) == true)
		{
			FStructProperty* StructProperty = CastField<FStructProperty>(Property);
			check(StructProperty);
			const FTacticalAttributeData* DataPtr = StructProperty->ContainerPtrToValuePtr<FTacticalAttributeData>(this);
			check(DataPtr);

			FTacticalAttribute Attribute(Property);

			// @김준형 크래시가 나여 Snapshot에 추가하는 형태로 변경했습니다.
			//Snapshot.mAttributes[Attribute] = DataPtr->GetCurrentValue();

			Snapshot.mAttributes.Add(Attribute, DataPtr->GetCurrentValue());

		}
	}
}

bool FTacticalAttribute::operator==(const FTacticalAttribute& Other) const
{
	return ((Other.mAttribute == mAttribute));
}

bool FTacticalAttribute::operator!=(const FTacticalAttribute& Other) const
{
	return ((Other.mAttribute != mAttribute));
}

TSubclassOf<UTacticalAttributeSet> FindBestAttributeClass(TArray<TSubclassOf<UTacticalAttributeSet> >& ClassList, FString PartialName)
{
	// 1순위: 클래스명 정확 일치.
	// 부분 포함만으로 고르면 "UnitAttributeSet"가 "PlayerUnitAttributeSet"에도 포함되어
	// 상속 속성(MaxHP 등)이 파생 클래스로 잘못 매핑된다(반복 순서에 따라 비결정적). 정확 일치를 먼저 본다.
	for (auto Class : ClassList)
	{
		if (Class->GetName() == PartialName)
		{
			return Class;
		}
	}

	// 2순위: 부분 포함(기존 동작 폴백).
	for (auto Class : ClassList)
	{
		if (Class->GetName().Contains(PartialName) == true)
		{
			return Class;
		}
	}
	return nullptr;
}

void FTacticalAttributeSetInitterDiscreteLevels::PreloadAttributeSetData(const TArray<UCurveTable*>& CurveData)
{
	if (ensure(CurveData.Num() > 0) == false)
	{
		return;
	}

	TArray<TSubclassOf<UTacticalAttributeSet> >	ClassList;
	for (TObjectIterator<UClass> ClassIt; ClassIt; ++ClassIt)
	{
		UClass* TestClass = *ClassIt;
		if (TestClass->IsChildOf(UTacticalAttributeSet::StaticClass()) == true)
		{
			ClassList.Add(TestClass);
		}
	}

	for (const UCurveTable* CurTable : CurveData)
	{
		for (const TPair<FName, FRealCurve*>& CurveRow : CurTable->GetRowMap())
		{
			FString RowName = CurveRow.Key.ToString();
			FString ClassName;
			FString SetName;
			FString AttributeName;
			FString Temp;

			RowName.Split(TEXT("."), &ClassName, &Temp);
			Temp.Split(TEXT("."), &SetName, &AttributeName);

			if (ensure(!ClassName.IsEmpty() && !SetName.IsEmpty() && !AttributeName.IsEmpty()) == false)
			{
				continue;
			}

			TSubclassOf<UTacticalAttributeSet> Set = FindBestAttributeClass(ClassList, SetName);
			if (Set == nullptr)
			{
				continue;
			}

			FProperty* Property = FindFProperty<FProperty>(*Set, *AttributeName);
			if (Property == nullptr)
			{
				continue;
			}
			else if (!FTacticalAttribute::IsSupportedProperty(Property) == true)
			{
				continue;
			}

			FRealCurve* Curve = CurveRow.Value;
			FName ClassFName = FName(*ClassName);
			FAttributeSetDefaultsCollection& DefaultCollection = mDefaults.FindOrAdd(ClassFName);

			float FirstLevelFloat = 0.f;
			float LastLevelFloat = 0.f;
			Curve->GetTimeRange(FirstLevelFloat, LastLevelFloat);

			int32 FirstLevel = FMath::RoundToInt32(FirstLevelFloat);
			int32 LastLevel = FMath::RoundToInt32(LastLevelFloat);

			if (FirstLevel != 1)
			{
				UE_LOG(LogTacticalFramework, Warning, TEXT("커브의 초기 레벨은 항상 1이여야 함"));
				continue;
			}

			DefaultCollection.mLevelData.SetNum(FMath::Max(LastLevel, DefaultCollection.mLevelData.Num()));

			for (int32 Level = 1; Level <= LastLevel; ++Level)
			{
				float Value = Curve->Eval(float(Level));

				FAttributeSetDefaults& SetDefaults = DefaultCollection.mLevelData[Level-1];

				FAttributeDefaultValueList* DefaultDataList = SetDefaults.mDataMap.Find(Set);
				if (DefaultDataList == nullptr)
				{
					DefaultDataList = &SetDefaults.mDataMap.Add(Set);
				}

				check(DefaultDataList != nullptr);
				DefaultDataList->AddPair(Property, Value);
			}
		}
	}
}

void FTacticalAttributeSetInitterDiscreteLevels::InitAttributeSetDefaults(UAttributeSetComponentModel* AttributeSetComponentModel, FName GroupName, int32 Level, bool InitialInit) const
{
	check(AttributeSetComponentModel != nullptr);
	
	const FAttributeSetDefaultsCollection* Collection = mDefaults.Find(GroupName);
	if (Collection == nullptr)
	{
		UE_LOG(LogTacticalFramework, Warning, TEXT("'%s' 그룹의 속성 기본값을 찾을 수 없음. Default 그룹으로 폴백."), *GroupName.ToString());
		Collection = mDefaults.Find(FName(TEXT("Default")));
	}
	if (Collection == nullptr)
	{
		// 그룹/Default 둘 다 없으면 하드 크래시 대신 초기화를 건너뛴다(원래 GAS 동작 복원).
		UE_LOG(LogTacticalFramework, Error, TEXT("'%s'(및 Default) 그룹의 속성 기본값이 없어 초기화를 건너뜁니다."), *GroupName.ToString());
		return;
	}

	if (Collection->mLevelData.IsValidIndex(Level - 1) == false)
	{
		UE_LOG(LogTacticalFramework, Warning, TEXT("%d 레벨의 속성 기본 값을 찾을 수 없음"), Level);
		return;
	}

	const FAttributeSetDefaults& SetDefaults = Collection->mLevelData[Level - 1];
	for (const UTacticalAttributeSet* Set : AttributeSetComponentModel->GetSpawnedAttributes())
	{
		if (Set == nullptr)
		{
			continue;
		}
		// 커브의 기본값 목록은 속성을 "선언한" 클래스 단위로 키가 잡힌다.
		// 파생 AttributeSet(예: UPlayerUnitAttributeSet)은 베이스(UUnitAttributeSet)에서 상속한 속성도 가지므로,
		// 스폰된 셋의 클래스부터 UTacticalAttributeSet까지 거슬러 올라가며 각 단계의 기본값을 모두 적용한다.
		for (UClass* SetClass = Set->GetClass();
			SetClass != nullptr && SetClass->IsChildOf(UTacticalAttributeSet::StaticClass());
			SetClass = SetClass->GetSuperClass())
		{
			const FAttributeDefaultValueList* DefaultDataList = SetDefaults.mDataMap.Find(SetClass);
			if (!DefaultDataList)
			{
				continue;
			}

			UE_LOG(LogTacticalFramework, Log, TEXT("%s 초기화 중 (%s의 기본값 참조)"), *Set->GetName(), *SetClass->GetName());

			for (auto& DataPair : DefaultDataList->mList)
			{
				check(DataPair.mProperty);

				if (Set->ShouldInitProperty(InitialInit, DataPair.mProperty))
				{
					FTacticalAttribute AttributeToModify(DataPair.mProperty);
					AttributeSetComponentModel->SetAttributeBaseValue(AttributeToModify, DataPair.mValue);
				}
			}
		}
	}
}

void FTacticalAttributeSetInitterDiscreteLevels::ApplyAttributeDefault(UAttributeSetComponentModel* AttributeSetComponentModel, FTacticalAttribute& Attribute, FName GroupName, int32 Level) const
{
	const FAttributeSetDefaultsCollection* Collection = mDefaults.Find(GroupName);
	if (Collection == nullptr)
	{
		UE_LOG(LogTacticalFramework, Warning, TEXT("'%s' 그룹의 속성 기본값을 찾을 수 없음. Default 그룹으로 폴백."), *GroupName.ToString());
		Collection = mDefaults.Find(FName(TEXT("Default")));
	}
	if (Collection == nullptr)
	{
		// 그룹/Default 둘 다 없으면 하드 크래시 대신 초기화를 건너뛴다(원래 GAS 동작 복원).
		UE_LOG(LogTacticalFramework, Error, TEXT("'%s'(및 Default) 그룹의 속성 기본값이 없어 초기화를 건너뜁니다."), *GroupName.ToString());
		return;
	}

	if (Collection->mLevelData.IsValidIndex(Level - 1) == false)
	{
		UE_LOG(LogTacticalFramework, Warning, TEXT("%d 레벨의 속성 기본 값을 찾을 수 없음"), Level);
		return;
	}

	const FAttributeSetDefaults& SetDefaults = Collection->mLevelData[Level - 1];
	for (const UTacticalAttributeSet* Set : AttributeSetComponentModel->GetSpawnedAttributes())
	{
		if (Set == nullptr)
		{
			continue;
		}

		const FAttributeDefaultValueList* DefaultDataList = SetDefaults.mDataMap.Find(Set->GetClass());
		if (DefaultDataList != nullptr)
		{
			UE_LOG(LogTacticalFramework, Log, TEXT("%s 초기화 중"), *Set->GetName());

			for (auto& DataPair : DefaultDataList->mList)
			{
				check(DataPair.mProperty);

				if (DataPair.mProperty == Attribute.GetUProperty())
				{
					FTacticalAttribute AttributeToModify(DataPair.mProperty);
					AttributeSetComponentModel->SetAttributeBaseValue(AttributeToModify, DataPair.mValue);
				}
			}
		}
	}
}

TArray<float> FTacticalAttributeSetInitterDiscreteLevels::GetAttributeSetValues(UClass* AttributeSetClass, FProperty* AttributeProperty, FName GroupName) const
{
	TArray<float> AttributeSetValues;
	const FAttributeSetDefaultsCollection* Collection = mDefaults.Find(GroupName);
	checkf(Collection != nullptr, TEXT("해당 속성 기본 값을 찾을 수 없음"));

	for (const FAttributeSetDefaults& SetDefaults : Collection->mLevelData)
	{
		const FAttributeDefaultValueList* DefaultDataList = SetDefaults.mDataMap.Find(AttributeSetClass);
		if (DefaultDataList != nullptr)
		{
			for (auto& DataPair : DefaultDataList->mList)
			{
				check(DataPair.mProperty);
				if (DataPair.mProperty == AttributeProperty)
				{
					AttributeSetValues.Add(DataPair.mValue);
				}
			}
		}
	}
	return AttributeSetValues;
}
