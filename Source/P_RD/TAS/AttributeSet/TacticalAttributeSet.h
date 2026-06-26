/*****************************************************************//**
 * @file   TacticalAttributeSet.h
 * @brief  AttributeSetComponentModel에 적합하게 변경한 AttributeSet 클래스 정의 헤더
 * @author 모호재
 * @date   2026-06-26
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "Engine/DataTable.h"
#include "TacticalAttributeSet.generated.h"

class UActorModel;

class UAttributeSetComponentModel;
class UTacticalAttributeSet;
class UCurveTable;
struct FTacticalAggregator;

USTRUCT(BlueprintType)
struct P_RD_API FTacticalAttributeData
{
	GENERATED_BODY()

public:
	FTacticalAttributeData() :
		mBaseValue(0.f),
		mCurrentValue(0.f)
	{}

	FTacticalAttributeData(float DefaultValue) :
		mBaseValue(DefaultValue),
		mCurrentValue(DefaultValue)
	{}

	virtual ~FTacticalAttributeData()
	{}

	float GetCurrentValue() const;
	virtual void SetCurrentValue(float NewValue);

	float GetBaseValue() const;
	virtual void SetBaseValue(float NewValue);

protected:
	UPROPERTY(Category = "Attribute", BlueprintReadOnly, meta = (DisplayName = "BaseValue"))
	float mBaseValue;

	UPROPERTY(Category = "Attribute", BlueprintReadOnly, meta = (DisplayName = "CurrentValue"))
	float mCurrentValue;
};

USTRUCT(BlueprintType)
struct P_RD_API FTacticalAttribute
{
	GENERATED_USTRUCT_BODY()

	friend class FTacticalAttributePropertyDetails;

public:
	FTacticalAttribute() :
		mAttribute(nullptr),
		mAttributeOwner(nullptr)
	{
	}

	FTacticalAttribute(FProperty *NewProperty);

	FTacticalAttribute(const FTacticalAttribute& Other)
	{
		SetUProperty(Other.GetUProperty());
	}

	operator FTacticalAttribute() const
	{
		return FTacticalAttribute(GetUProperty());
	}

	bool IsValid() const
	{
		return mAttribute.Get() != nullptr;
	}

	void SetUProperty(FProperty *NewProperty)
	{
		mAttribute = NewProperty;
		if (NewProperty != nullptr)
		{
			mAttributeOwner = mAttribute->GetOwnerStruct();
			mAttribute->GetName(mAttributeName);
		}
		else
		{
			mAttributeOwner = nullptr;
			mAttributeName.Empty();
		}
	}

	FProperty* GetUProperty() const
	{
		return mAttribute.Get();
	}

	UClass* GetAttributeSetClass() const
	{
		check(mAttribute.Get());
		return CastChecked<UClass>(mAttribute->GetOwner<UObject>());
	}

	bool IsSystemAttribute() const;

	static bool IsSupportedProperty(const FProperty* Prop);

	static bool IsTacticalAttributeDataProperty(const FProperty* Property);

	void SetNumericValueChecked(float& NewValue, class UTacticalAttributeSet* Dest) const;

	float GetNumericValue(const UTacticalAttributeSet* Src) const;
	float GetNumericValueChecked(const UTacticalAttributeSet* Src) const;

	const FTacticalAttributeData* GetTacticalAttributeData(const UTacticalAttributeSet* Src) const;
	const FTacticalAttributeData* GetTacticalAttributeDataChecked(const UTacticalAttributeSet* Src) const;
	FTacticalAttributeData* GetTacticalAttributeData(UTacticalAttributeSet* Src) const;
	FTacticalAttributeData* GetTacticalAttributeDataChecked(UTacticalAttributeSet* Src) const;
	
	bool operator==(const FTacticalAttribute& Other) const;
	bool operator!=(const FTacticalAttribute& Other) const;

	friend uint32 GetTypeHash(const FTacticalAttribute& InAttribute)
	{
		return PointerHash(InAttribute.mAttribute.Get());
	}

	/** Returns name of attribute, usually the same as the property */
	FString GetName() const
	{
		return mAttributeName.IsEmpty() ? *GetNameSafe(mAttribute.Get()) : mAttributeName;
	}

#if WITH_EDITORONLY_DATA
	/** Custom serialization */
	void PostSerialize(const FArchive& Ar);
#endif

	static void GetAllAttributeProperties(TArray<FProperty*>& OutProperties, FString FilterMetaStr = FString(), bool UseEditorOnlyData = true);

	UPROPERTY(Category = "GameplayAttribute", VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "AttributeName"))
	FString mAttributeName;
	
private:

	UPROPERTY(Category = "GameplayAttribute", EditAnywhere, meta = (DisplayName = "Attribute"))
	TFieldPath<FProperty> mAttribute;

	UPROPERTY(Category = "GameplayAttribute", VisibleAnywhere, meta = (DisplayName = "AttributeOwner"))
	TObjectPtr<UStruct> mAttributeOwner;
};

#if WITH_EDITORONLY_DATA
template<>
struct TStructOpsTypeTraits< FTacticalAttribute > : public TStructOpsTypeTraitsBase2< FTacticalAttribute >
{
	enum
	{
		WithPostSerialize = true,
	};
};
#endif

UCLASS(DefaultToInstanced, Blueprintable)
class P_RD_API UTacticalAttributeSet : public UObject
{
	GENERATED_UCLASS_BODY()

public:
	static void GetAttributesFromSetClass(const TSubclassOf<UTacticalAttributeSet>& AttributeSetClass, TArray<FTacticalAttribute>& Attributes);

	virtual bool ShouldInitProperty(bool FirstInit, FProperty* PropertyToInit) const { return true; }
	virtual bool PreGameplayEffectExecute(struct FGameplayEffectModCallbackData &Data) { return true; }
	virtual void PostGameplayEffectExecute(const struct FGameplayEffectModCallbackData &Data) { }

	virtual void PreAttributeChange(const FTacticalAttribute& Attribute, float& NewValue) { }
	virtual void PostAttributeChange(const FTacticalAttribute& Attribute, float OldValue, float NewValue) { }

	virtual void PreAttributeBaseChange(const FTacticalAttribute& Attribute, float& NewValue) const { }
	virtual void PostAttributeBaseChange(const FTacticalAttribute& Attribute, float OldValue, float NewValue) const { }

	virtual void OnAttributeAggregatorCreated(const FTacticalAttribute& Attribute, FTacticalAggregator* NewAggregator) const { }

	void SetNetAddressable();

	virtual void InitFromMetaDataTable(const UDataTable* DataTable);

	UActorModel* GetOwningActor() const;
	UAttributeSetComponentModel* GetOwningAttributeSetComponentModel() const;
	UAttributeSetComponentModel* GetOwningAttributeSetComponentModelChecked() const;

	virtual void PrintDebug();

protected:
	uint32 mNetAddressable : 1;
};

struct FTacticalAttributeSetInitter
{
	virtual ~FTacticalAttributeSetInitter() {}

	virtual void PreloadAttributeSetData(const TArray<UCurveTable*>& CurveData) = 0;
	virtual void InitAttributeSetDefaults(UAttributeSetComponentModel* AttributeSetComponentModel, FName GroupName, int32 Level, bool bInitialInit) const = 0;
	virtual void ApplyAttributeDefault(UAttributeSetComponentModel* AttributeSetComponentModel, FTacticalAttribute& InAttribute, FName GroupName, int32 Level) const = 0;
	virtual TArray<float> GetAttributeSetValues(UClass* AttributeSetClass, FProperty* AttributeProperty, FName GroupName) const { return TArray<float>(); }
};

struct FTacticalAttributeSetInitterDiscreteLevels : public FTacticalAttributeSetInitter
{
	virtual void PreloadAttributeSetData(const TArray<UCurveTable*>& CurveData) override;
	virtual void InitAttributeSetDefaults(UAttributeSetComponentModel* AttributeSetComponentModel, FName GroupName, int32 Level, bool bInitialInit) const override;
	virtual void ApplyAttributeDefault(UAttributeSetComponentModel* AttributeSetComponentModel, FTacticalAttribute& InAttribute, FName GroupName, int32 Level) const override;
	virtual TArray<float> GetAttributeSetValues(UClass* AttributeSetClass, FProperty* AttributeProperty, FName GroupName) const override;

private:
	struct FAttributeDefaultValueList
	{
		void AddPair(FProperty* InProperty, float InValue)
		{
			mList.Add(FOffsetValuePair(InProperty, InValue));
		}

		struct FOffsetValuePair
		{
			FOffsetValuePair(FProperty* InProperty, float InValue) :
				mProperty(InProperty), 
				mValue(InValue) 
			{ 
			}

			FProperty*	mProperty;
			float		mValue;
		};

		TArray<FOffsetValuePair>	mList;
	};

	struct FAttributeSetDefaults
	{
		TMap<TSubclassOf<UTacticalAttributeSet>, FAttributeDefaultValueList> mDataMap;
	};

	struct FAttributeSetDefaultsCollection
	{
		TArray<FAttributeSetDefaults>		mLevelData;
	};

	TMap<FName, FAttributeSetDefaultsCollection>	mDefaults;
};

#define TACTICAL_ATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	static FTacticalAttribute Get##PropertyName##Attribute() \
	{ \
		static FProperty* Prop = FindFieldChecked<FProperty>(ClassName::StaticClass(), GET_MEMBER_NAME_CHECKED(ClassName, PropertyName)); \
		return Prop; \
	}

#define TACTICAL_ATTRIBUTE_VALUE_GETTER(PropertyName) \
	inline float Get##PropertyName() const \
	{ \
		return PropertyName.GetCurrentValue(); \
	}

#define TACTICAL_ATTRIBUTE_VALUE_SETTER(PropertyName) \
	inline void Set##PropertyName(float NewVal) \
	{ \
		UAttributeSetComponentModel* Model = GetOwningAttributeSetComponentModel(); \
		if (ensure(Model)) \
		{ \
			Model->SetAttributeBaseValue(Get##PropertyName##Attribute(), NewVal); \
		}; \
	}

#define TACTICAL_ATTRIBUTE_VALUE_INITTER(PropertyName) \
	inline void Init##PropertyName(float NewVal) \
	{ \
		PropertyName.SetBaseValue(NewVal); \
		PropertyName.SetCurrentValue(NewVal); \
	}

#define TACTICAL_ATTRIBUTE_ACCESSORS_BASIC(ClassName, PropertyName) \
	TACTICAL_ATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	TACTICAL_ATTRIBUTE_VALUE_GETTER(PropertyName) \
	TACTICAL_ATTRIBUTE_VALUE_SETTER(PropertyName) \
	TACTICAL_ATTRIBUTE_VALUE_INITTER(PropertyName)

#undef UE_API
