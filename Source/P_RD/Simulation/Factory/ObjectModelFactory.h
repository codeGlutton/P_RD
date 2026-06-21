/*****************************************************************//**
 * @file   ObjectModelFactory.h
 * @brief  모델 생성 용 팩토리 헤더
 * @author 모호재
 * @date   2026-06-17
 *********************************************************************/
#pragma once

#include "RDMinimal.h"
#include "ObjectModelFactory.generated.h"

class UObjectModel;
struct FRoomContext;

// Model Factory 신규 로그 카테고리 등록
DECLARE_LOG_CATEGORY_EXTERN(LogObjectModelFactory, Log, All)

UCLASS(abstract)
class P_RD_API UObjectModelFactory : public UObject
{
	GENERATED_BODY()

public:
	void SetContext(FRoomContext& RoomContext);

public:
	template<typename T>
	T* NewModel(const UClass* Class, const FTransform& ViewTransform = FTransform::Identity, FName Name = NAME_None, EObjectFlags Flags = RF_NoFlags, UObject* Template = nullptr, bool CopyTransientsFromClassDefaults = false, FObjectInstancingGraph* InInstanceGraph = nullptr, UPackage* InExternalPackage = nullptr)
	{
		T* Result = Cast<T>(NewModel_Internal(Class, ViewTransform, Name, Flags, Template, CopyTransientsFromClassDefaults, InInstanceGraph, InExternalPackage));
		return Result;
	}

	template<typename T>
	T* NewModel(const FTransform& ViewTransform, FName Name, EObjectFlags Flags = RF_NoFlags, UObject* Template = nullptr, bool CopyTransientsFromClassDefaults = false, FObjectInstancingGraph* InInstanceGraph = nullptr, UPackage* InExternalPackage = nullptr)
	{
		static_assert(TIsDerivedFrom<T, UObjectModel>::IsDerived, "UObjectModel를 상속해야 함");
		T* Result = Cast<T>(NewModel_Internal(T::StaticClass(), ViewTransform, Name, Flags, Template, CopyTransientsFromClassDefaults, InInstanceGraph, InExternalPackage));
		return Result;
	}

	template<typename T>
	T* NewModel(const FTransform& ViewTransform = FTransform::Identity)
	{
		static_assert(TIsDerivedFrom<T, UObjectModel>::IsDerived, "UObjectModel를 상속해야 함");
		T* Result = Cast<T>(NewModel_Internal(T::StaticClass(), ViewTransform, NAME_None));
		return Result;
	}

	template<typename T>
	T* NewModelDeferred(const UClass* Class = T::StaticClass(), FName Name = NAME_None, EObjectFlags Flags = RF_NoFlags, UObject* Template = nullptr, bool CopyTransientsFromClassDefaults = false, FObjectInstancingGraph* InInstanceGraph = nullptr, UPackage* InExternalPackage = nullptr)
	{
		T* Result = Cast<T>(NewModelDeferred_Internal(T::StaticClass(), Name, Flags, Template, CopyTransientsFromClassDefaults, InInstanceGraph, InExternalPackage));
		return Result;
	}

	template<typename T>
	T* NewModelDeferred(FName Name = NAME_None, EObjectFlags Flags = RF_NoFlags, UObject* Template = nullptr, bool CopyTransientsFromClassDefaults = false, FObjectInstancingGraph* InInstanceGraph = nullptr, UPackage* InExternalPackage = nullptr)
	{
		static_assert(TIsDerivedFrom<T, UObjectModel>::IsDerived, "UObjectModel를 상속해야 함");
		T* Result = Cast<T>(NewModelDeferred_Internal(T::StaticClass(), Name, Flags, Template, CopyTransientsFromClassDefaults, InInstanceGraph, InExternalPackage));
		return Result;
	}

protected:
	UObjectModel* NewModel_Internal(const UClass* Class, const FTransform& ViewTransform = FTransform::Identity, FName Name = NAME_None, EObjectFlags Flags = RF_NoFlags, UObject* Template = nullptr, bool CopyTransientsFromClassDefaults = false, FObjectInstancingGraph* InInstanceGraph = nullptr, UPackage* InExternalPackage = nullptr);
	UObjectModel* NewModelDeferred_Internal(const UClass* Class, FName Name = NAME_None, EObjectFlags Flags = RF_NoFlags, UObject* Template = nullptr, bool CopyTransientsFromClassDefaults = false, FObjectInstancingGraph* InInstanceGraph = nullptr, UPackage* InExternalPackage = nullptr);

public:
	void FinishCreatingModel(UObjectModel* Model, const FTransform& ViewTransform = FTransform::Identity);
	void DestroyModel(UObjectModel* Model);

protected:
	virtual void OnPostCreateNewModel(UObjectModel* Model, const FTransform& ViewTransform) PURE_VIRTUAL(UObjectModelFactory::OnPostCreateNewModel, return;);
	virtual void OnPreRemoveModel(UObjectModel* Model) PURE_VIRTUAL(UObjectModelFactory::OnPreRemoveModel, return;);

protected:
	FRoomContext* mRoomContext = nullptr;
};

/**
 * @brief 실제 게임에서의 모델 팩토리
 */
UCLASS()
class UGameObjectModelFactory : public UObjectModelFactory
{
	GENERATED_BODY()

public:
	void RegisterSubsystemModels();
	void UnregisterSubsystemModels();

protected:
	void OnPostCreateNewModel(UObjectModel* Model, const FTransform& ViewTransform) override;
	void OnPreRemoveModel(UObjectModel* Model) override;
};

/**
 * @brief 시뮬레이션에서의 모델 팩토리
 */
UCLASS()
class USimulationObjectModelFactory : public UObjectModelFactory
{
	GENERATED_BODY()

protected:
	void OnPostCreateNewModel(UObjectModel* Model, const FTransform& ViewTransform) override;
	void OnPreRemoveModel(UObjectModel* Model) override;
};
