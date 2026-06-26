#include "TAS/AttributeSet/TacticalAttributeSet.h"
#include "AbilitySystemLog.h"
#include "AbilitySystemStats.h"
#include "UObject/UObjectIterator.h"

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
	if (FTacticalAttribute::IsSupportedProperty(NewProperty))
	{
		mAttribute = NewProperty;
		mAttributeOwner = mAttribute->GetOwnerStruct();
		mAttribute->GetName(mAttributeName);
	}
	else
	{
		ensureMsgf(!NewProperty, TEXT("Tried to construct FTacticalAttribute with invalid property '%s' of type '%s'. Treating as invalid attribute."), *NewProperty->GetName(), *NewProperty->GetClass()->GetName());

		mAttribute = nullptr;
		mAttributeOwner = nullptr;
		mAttributeName = "";
	}
}

void FTacticalAttribute::SetNumericValueChecked(float& NewValue, class UTacticalAttributeSet* Dest) const
{
	check(Dest);

	FNumericProperty* NumericProperty = CastField<FNumericProperty>(mAttribute.Get());
	float OldValue = 0.f;
	if (NumericProperty)
	{
		void* ValuePtr = NumericProperty->ContainerPtrToValuePtr<void>(Dest);
		OldValue = *static_cast<float*>(ValuePtr);
		Dest->PreAttributeChange(*this, NewValue);
		NumericProperty->SetFloatingPointPropertyValue(ValuePtr, NewValue);
		Dest->PostAttributeChange(*this, OldValue, NewValue);

		// MARK_PROPERTY_DIRTY(Dest, NumericProperty);
	}
	else if (IsTacticalAttributeDataProperty(mAttribute.Get()))
	{
		FStructProperty* StructProperty = CastField<FStructProperty>(mAttribute.Get());
		check(StructProperty);
		FTacticalAttributeData* DataPtr = StructProperty->ContainerPtrToValuePtr<FTacticalAttributeData>(Dest);
		check(DataPtr);
		OldValue = DataPtr->GetCurrentValue();
		Dest->PreAttributeChange(*this, NewValue);
		DataPtr->SetCurrentValue(NewValue);
		Dest->PostAttributeChange(*this, OldValue, DataPtr->GetCurrentValue());

		// MARK_PROPERTY_DIRTY(Dest, StructProperty);
	}
	else
	{
		check(false);
	}
}

float FTacticalAttribute::GetNumericValue(const UTacticalAttributeSet* Src) const
{
	const FNumericProperty* const NumericProperty = CastField<FNumericProperty>(mAttribute.Get());
	if (NumericProperty)
	{
		const void* ValuePtr = NumericProperty->ContainerPtrToValuePtr<void>(Src);
		return NumericProperty->GetFloatingPointPropertyValue(ValuePtr);
	}
	else if (IsTacticalAttributeDataProperty(mAttribute.Get()))
	{
		const FStructProperty* StructProperty = CastField<FStructProperty>(mAttribute.Get());
		check(StructProperty);
		const FTacticalAttributeData* DataPtr = StructProperty->ContainerPtrToValuePtr<FTacticalAttributeData>(Src);
		if (ensure(DataPtr))
		{
			return DataPtr->GetCurrentValue();
		}
	}

	return 0.f;
}

float FTacticalAttribute::GetNumericValueChecked(const UTacticalAttributeSet* Src) const
{
	FNumericProperty* NumericProperty = CastField<FNumericProperty>(mAttribute.Get());
	if (NumericProperty)
	{
		const void* ValuePtr = NumericProperty->ContainerPtrToValuePtr<void>(Src);
		return NumericProperty->GetFloatingPointPropertyValue(ValuePtr);
	}
	else if (IsTacticalAttributeDataProperty(mAttribute.Get()))
	{
		const FStructProperty* StructProperty = CastField<FStructProperty>(mAttribute.Get());
		check(StructProperty);
		const FTacticalAttributeData* DataPtr = StructProperty->ContainerPtrToValuePtr<FTacticalAttributeData>(Src);
		if (ensure(DataPtr))
		{
			return DataPtr->GetCurrentValue();
		}
	}

	check(false);
	return 0.f;
}

const FTacticalAttributeData* FTacticalAttribute::GetTacticalAttributeData(const UTacticalAttributeSet* Src) const
{
	if (Src && IsTacticalAttributeDataProperty(mAttribute.Get()))
	{
		FStructProperty* StructProperty = CastField<FStructProperty>(mAttribute.Get());
		check(StructProperty);
		// MARK_PROPERTY_DIRTY(Src, StructProperty);
		return StructProperty->ContainerPtrToValuePtr<FTacticalAttributeData>(Src);
	}

	return nullptr;
}

const FTacticalAttributeData* FTacticalAttribute::GetTacticalAttributeDataChecked(const UTacticalAttributeSet* Src) const
{
	if (Src && IsTacticalAttributeDataProperty(mAttribute.Get()))
	{
		FStructProperty* StructProperty = CastField<FStructProperty>(mAttribute.Get());
		check(StructProperty);
		// MARK_PROPERTY_DIRTY(Src, StructProperty);
		return StructProperty->ContainerPtrToValuePtr<FTacticalAttributeData>(Src);
	}

	check(false);
	return nullptr;
}

FTacticalAttributeData* FTacticalAttribute::GetTacticalAttributeData(UTacticalAttributeSet* Src) const
{
	if (Src && IsTacticalAttributeDataProperty(mAttribute.Get()))
	{
		FStructProperty* StructProperty = CastField<FStructProperty>(mAttribute.Get());
		check(StructProperty);
		// MARK_PROPERTY_DIRTY(Src, StructProperty);
		return StructProperty->ContainerPtrToValuePtr<FTacticalAttributeData>(Src);
	}

	return nullptr;
}

FTacticalAttributeData* FTacticalAttribute::GetTacticalAttributeDataChecked(UTacticalAttributeSet* Src) const
{
	if (Src && IsTacticalAttributeDataProperty(mAttribute.Get()))
	{
		FStructProperty* StructProperty = CastField<FStructProperty>(mAttribute.Get());
		check(StructProperty);
		// MARK_PROPERTY_DIRTY(Src, StructProperty);
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
	return Prop && (FTacticalAttribute::IsTacticalAttributeDataProperty(Prop) || (Prop->IsA<FNumericProperty>() && CastField<FNumericProperty>(Prop)->IsFloatingPoint()));
}

bool FTacticalAttribute::IsTacticalAttributeDataProperty(const FProperty* Property)
{
	const FStructProperty* StructProp = CastField<FStructProperty>(Property);
	if (StructProp)
	{
		const UStruct* Struct = StructProp->Struct;
		if (Struct && Struct->IsChildOf(FTacticalAttributeData::StaticStruct()))
		{
			return true;
		}
	}

	return false;
}

#if WITH_EDITORONLY_DATA
void FTacticalAttribute::PostSerialize(const FArchive& Ar)
{
	if (Ar.IsLoading() && Ar.IsPersistent() && !Ar.HasAnyPortFlags(PPF_Duplicate | PPF_DuplicateForPIE))
	{
		const FString PathName = mAttribute.ToString();
		const FString RedirectedPathName = FFieldPathProperty::RedirectFieldPathName(PathName);
		if (!RedirectedPathName.Equals(PathName))
		{
			FString NewAttributeOwner;
			FString NewAttributeName;
			if (RedirectedPathName.Split(":", &NewAttributeOwner, &NewAttributeName))
			{
				const UStruct* NewClass = FindObject<UStruct>(nullptr, *NewAttributeOwner);
				mAttribute = FindFProperty<FProperty>(NewClass, *NewAttributeName);

				FUObjectSerializeContext* LoadContext = FUObjectThreadContext::Get().GetSerializeContext();
				const FString AssetName = (LoadContext && LoadContext->SerializedObject) ? LoadContext->SerializedObject->GetPathName() : TEXT("Unknown Object");
				ABILITY_LOG(Verbose, TEXT("FTacticalAttribute::PostSerialize redirected an attribute '%s' -> '%s'. (Asset: %s)"), *PathName, *RedirectedPathName, *AssetName);
			}
		}

		if (mAttribute.Get())
		{
			mAttributeOwner = mAttribute->GetOwnerStruct();
			mAttribute->GetName(mAttributeName);
		}
		else if (!mAttributeName.IsEmpty())
		{
			if (mAttributeOwner != nullptr)
			{
				mAttribute = FindFProperty<FProperty>(mAttributeOwner, *mAttributeName);
			}

			if (!mAttribute.Get())
			{
				FUObjectSerializeContext* LoadContext = FUObjectThreadContext::Get().GetSerializeContext();
				const FString AssetName = (LoadContext && LoadContext->SerializedObject) ? LoadContext->SerializedObject->GetPathName() : TEXT("Unknown Object");
				const FString OwnerName = mAttributeOwner ? mAttributeOwner->GetName() : TEXT("NONE");
				ABILITY_LOG(Warning, TEXT("FTacticalAttribute::PostSerialize called on an invalid attribute with owner %s and name %s. (Asset: %s)"), *OwnerName, *mAttributeName, *AssetName);
			}
		}
	}
	if (Ar.IsSaving() && IsValid())
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
		if (Class->IsChildOf(UTacticalAttributeSet::StaticClass()) 
#if WITH_EDITORONLY_DATA
			&& !Class->ClassGeneratedBy
#endif
			)
		{
			if (UseEditorOnlyData)
			{
				#if WITH_EDITOR
				if (Class->HasMetaData(TEXT("HideInDetailsView")))
				{
					continue;
				}
				#endif
			}

			for (TFieldIterator<FProperty> PropertyIt(Class, EFieldIteratorFlags::ExcludeSuper); PropertyIt; ++PropertyIt)
			{
				FProperty* Property = *PropertyIt;

				if (UseEditorOnlyData)
				{
					#if WITH_EDITOR
					if (!FilterMetaStr.IsEmpty() && Property->HasMetaData(*FilterMetaStr))
					{
						continue;
					}

					if (Property->HasMetaData(TEXT("HideInDetailsView")))
					{
						continue;
					}
					#endif
				}
				
				OutProperties.Add(Property);
			}
		}

#if WITH_EDITOR
		if (UseEditorOnlyData)
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

UTacticalAttributeSet::UTacticalAttributeSet(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

void UTacticalAttributeSet::GetAttributesFromSetClass(const TSubclassOf<UTacticalAttributeSet>& AttributeSetClass, TArray<FTacticalAttribute>& Attributes)
{
	for (TFieldIterator<FProperty> It(AttributeSetClass); It; ++It)
	{
		if (FTacticalAttribute::IsSupportedProperty(*It))
		{
			Attributes.Add(FTacticalAttribute(*It));
		}
	}
}

void UTacticalAttributeSet::SetNetAddressable()
{
	mNetAddressable = true;
}

void UTacticalAttributeSet::InitFromMetaDataTable(const UDataTable* DataTable)
{
	static const FString Context = FString(TEXT("UTacticalAttributeSet::InitFromMetaDataTable"));

	for (TFieldIterator<FProperty> It(GetClass(), EFieldIteratorFlags::IncludeSuper); It; ++It)
	{
		FProperty* Property = *It;

		if (FTacticalAttribute::IsSupportedProperty(Property))
		{
			FString RowNameStr = FString::Printf(TEXT("%s.%s"), *Property->GetOwnerVariant().GetName(), *Property->GetName());
			if (FAttributeMetaData* MetaData = DataTable->FindRow<FAttributeMetaData>(FName(*RowNameStr), Context, false))
			{
				FNumericProperty* NumericProperty = CastField<FNumericProperty>(Property);
				if (NumericProperty)
				{
					check(NumericProperty->IsFloatingPoint());
					void* Data = NumericProperty->ContainerPtrToValuePtr<void>(this);
					NumericProperty->SetFloatingPointPropertyValue(Data, MetaData->BaseValue);
				}
				else if (FTacticalAttribute::IsTacticalAttributeDataProperty(Property))
				{
					FStructProperty* StructProperty = CastField<FStructProperty>(Property);
					check(StructProperty);
					FTacticalAttributeData* DataPtr = StructProperty->ContainerPtrToValuePtr<FTacticalAttributeData>(this);
					check(DataPtr);
					DataPtr->SetBaseValue(MetaData->BaseValue);
					DataPtr->SetCurrentValue(MetaData->BaseValue);
				}
			}
		}
	}

	PrintDebug();
}

UActorModel* UTacticalAttributeSet::GetOwningActor() const
{
	return Cast<UActorModel>(GetOuter());
}

UAttributeSetComponentModel* UTacticalAttributeSet::GetOwningAttributeSetComponentModel() const
{
	UActorModel* ActorModel = GetOwningActor();

	const IBoardCombatTarget* ASI = Cast<IBoardCombatTarget>(ActorModel);
	if (ASI)
	{
		return ASI->GetAttributeComponentModel();
	}

	return ActorModel ? ActorModel->FindComponentModelByClass<UAttributeSetComponentModel>() : nullptr;
}

UAttributeSetComponentModel* UTacticalAttributeSet::GetOwningAttributeSetComponentModelChecked() const
{
	UAttributeSetComponentModel* Result = GetOwningAttributeSetComponentModel();
	check(Result);
	return Result;
}

void UTacticalAttributeSet::PrintDebug()
{
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
	for (auto Class : ClassList)
	{
		if (Class->GetName().Contains(PartialName))
		{
			return Class;
		}
	}

	return nullptr;
}

void FTacticalAttributeSetInitterDiscreteLevels::PreloadAttributeSetData(const TArray<UCurveTable*>& CurveData)
{
	if (!ensure(CurveData.Num() > 0))
	{
		return;
	}

	TArray<TSubclassOf<UTacticalAttributeSet> >	ClassList;
	for (TObjectIterator<UClass> ClassIt; ClassIt; ++ClassIt)
	{
		UClass* TestClass = *ClassIt;
		if (TestClass->IsChildOf(UTacticalAttributeSet::StaticClass()))
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

			if (!ensure(!ClassName.IsEmpty() && !SetName.IsEmpty() && !AttributeName.IsEmpty()))
			{
				UE_LOG(LogTacticalFramework, Verbose, TEXT("FTacticalAttributeSetInitterDiscreteLevels::PreloadAttributeSetData Unable to parse row %s in %s"), *RowName, *CurTable->GetName());
				continue;
			}

			TSubclassOf<UTacticalAttributeSet> Set = FindBestAttributeClass(ClassList, SetName);
			if (!Set)
			{
				UE_LOG(LogTacticalFramework, Verbose, TEXT("FTacticalAttributeSetInitterDiscreteLevels::PreloadAttributeSetData Unable to match AttributeSet from %s (row: %s)"), *SetName, *RowName);
				continue;
			}

			FProperty* Property = FindFProperty<FProperty>(*Set, *AttributeName);
			if (!Property)
			{
				UE_LOG(LogTacticalFramework, Verbose, TEXT("FTacticalAttributeSetInitterDiscreteLevels::PreloadAttributeSetData Unable to match Attribute from %s (row: %s): Property not found"), *AttributeName, *RowName);
				continue;
			}
			else if (!FTacticalAttribute::IsSupportedProperty(Property))
			{
				UE_LOG(LogTacticalFramework, Verbose, TEXT("FTacticalAttributeSetInitterDiscreteLevels::PreloadAttributeSetData Unable to match Attribute from %s (row: %s): Property type '%s' incompatible. Must be FTacticalAttributeData or a floating point type."), *AttributeName, *RowName, *Property->GetClass()->GetName());
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
				UE_LOG(LogTacticalFramework, Warning, TEXT("FTacticalAttributeSetInitterDiscreteLevels::PreloadAttributeSetData First level should be 1"));
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
					UE_LOG(LogTacticalFramework, Verbose, TEXT("Initializing new default set for %s[%d]. PropertySize: %d.. DefaultSize: %d"), *Set->GetName(), Level, Set->GetPropertiesSize(), UTacticalAttributeSet::StaticClass()->GetPropertiesSize());

					DefaultDataList = &SetDefaults.mDataMap.Add(Set);
				}

				check(DefaultDataList);
				DefaultDataList->AddPair(Property, Value);
			}
		}
	}
}

void FTacticalAttributeSetInitterDiscreteLevels::InitAttributeSetDefaults(UAttributeSetComponentModel* AttributeSetComponentModel, FName GroupName, int32 Level, bool bInitialInit) const
{
	// SCOPE_CYCLE_COUNTER(STAT_InitAttributeSetDefaults);
	check(AttributeSetComponentModel != nullptr);
	
	const FAttributeSetDefaultsCollection* Collection = mDefaults.Find(GroupName);
	if (!Collection)
	{
		UE_LOG(LogTacticalFramework, Warning, TEXT("Unable to find DefaultAttributeSet Group %s. Falling back to Defaults"), *GroupName.ToString());
		Collection = mDefaults.Find(FName(TEXT("Default")));
		if (!Collection)
		{
			UE_LOG(LogTacticalFramework, Error, TEXT("FTacticalAttributeSetInitterDiscreteLevels::InitAttributeSetDefaults Default DefaultAttributeSet not found! Skipping Initialization"));
			return;
		}
	}

	if (!Collection->mLevelData.IsValidIndex(Level - 1))
	{
		UE_LOG(LogTacticalFramework, Warning, TEXT("Attribute defaults for Level %d are not defined! Skipping"), Level);
		return;
	}

	const FAttributeSetDefaults& SetDefaults = Collection->mLevelData[Level - 1];
	for (const UTacticalAttributeSet* Set : AttributeSetComponentModel->GetSpawnedAttributes())
	{
		if (!Set)
		{
			continue;
		}
		const FAttributeDefaultValueList* DefaultDataList = SetDefaults.mDataMap.Find(Set->GetClass());
		if (DefaultDataList)
		{
			UE_LOG(LogTacticalFramework, Log, TEXT("Initializing Set %s"), *Set->GetName());

			for (auto& DataPair : DefaultDataList->mList)
			{
				check(DataPair.mProperty);

				if (Set->ShouldInitProperty(bInitialInit, DataPair.mProperty))
				{
					FTacticalAttribute AttributeToModify(DataPair.mProperty);
					AttributeSetComponentModel->SetAttributeBaseValue(AttributeToModify, DataPair.mValue);
				}
			}
		}		
	}
	
	// AttributeSetComponentModel->ForceReplication();
}

void FTacticalAttributeSetInitterDiscreteLevels::ApplyAttributeDefault(UAttributeSetComponentModel* AttributeSetComponentModel, FTacticalAttribute& InAttribute, FName GroupName, int32 Level) const
{
	// SCOPE_CYCLE_COUNTER(STAT_InitAttributeSetDefaults);

	const FAttributeSetDefaultsCollection* Collection = mDefaults.Find(GroupName);
	if (!Collection)
	{
		UE_LOG(LogTacticalFramework, Warning, TEXT("Unable to find DefaultAttributeSet Group %s. Falling back to Defaults"), *GroupName.ToString());
		Collection = mDefaults.Find(FName(TEXT("Default")));
		if (!Collection)
		{
			UE_LOG(LogTacticalFramework, Error, TEXT("FTacticalAttributeSetInitterDiscreteLevels::InitAttributeSetDefaults Default DefaultAttributeSet not found! Skipping Initialization"));
			return;
		}
	}

	if (!Collection->mLevelData.IsValidIndex(Level - 1))
	{
		UE_LOG(LogTacticalFramework, Warning, TEXT("Attribute defaults for Level %d are not defined! Skipping"), Level);
		return;
	}

	const FAttributeSetDefaults& SetDefaults = Collection->mLevelData[Level - 1];
	for (const UTacticalAttributeSet* Set : AttributeSetComponentModel->GetSpawnedAttributes())
	{
		if (!Set)
		{
			continue;
		}

		const FAttributeDefaultValueList* DefaultDataList = SetDefaults.mDataMap.Find(Set->GetClass());
		if (DefaultDataList)
		{
			UE_LOG(LogTacticalFramework, Log, TEXT("Initializing Set %s"), *Set->GetName());

			for (auto& DataPair : DefaultDataList->mList)
			{
				check(DataPair.mProperty);

				if (DataPair.mProperty == InAttribute.GetUProperty())
				{
					FTacticalAttribute AttributeToModify(DataPair.mProperty);
					AttributeSetComponentModel->SetAttributeBaseValue(AttributeToModify, DataPair.mValue);
				}
			}
		}
	}

	//AbilitySystemComponent->ForceReplication();
}

TArray<float> FTacticalAttributeSetInitterDiscreteLevels::GetAttributeSetValues(UClass* AttributeSetClass, FProperty* AttributeProperty, FName GroupName) const
{
	TArray<float> AttributeSetValues;
	const FAttributeSetDefaultsCollection* Collection = mDefaults.Find(GroupName);
	if (!Collection)
	{
		UE_LOG(LogTacticalFramework, Error, TEXT("FTacticalAttributeSetInitterDiscreteLevels::InitAttributeSetDefaults Default DefaultAttributeSet not found! Skipping Initialization"));
		return TArray<float>();
	}

	for (const FAttributeSetDefaults& SetDefaults : Collection->mLevelData)
	{
		const FAttributeDefaultValueList* DefaultDataList = SetDefaults.mDataMap.Find(AttributeSetClass);
		if (DefaultDataList)
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
