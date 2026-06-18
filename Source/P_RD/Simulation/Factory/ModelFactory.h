/*****************************************************************//**
 * @file   ModelFactory.h
 * @brief  모델 생성 용 팩토리 헤더
 * @author 모호재
 * @date   2026-06-17
 *********************************************************************/
#pragma once

#include "RDMinimal.h"
#include "Model/ObjectModel.h"
#include "ModelFactory.generated.h"

// Model Factory 신규 로그 카테고리 등록
DECLARE_LOG_CATEGORY_EXTERN(LogModelFactory, Log, All)

UINTERFACE(MinimalAPI)
class UModelFactory : public UInterface
{
	GENERATED_BODY()
};

class P_RD_API IModelFactory
{
	GENERATED_BODY()

public:
	void SetModelDefaultOuter(UObject* DefaultOuter);

public:
	template<typename T>
	T* NewModel(const UClass* Class, FName Name = NAME_None, EObjectFlags Flags = RF_NoFlags, UObject* Template = nullptr, bool CopyTransientsFromClassDefaults = false, FObjectInstancingGraph* InInstanceGraph = nullptr, UPackage* InExternalPackage = nullptr)
	{
		T* Result = static_cast<T*>(NewModel_Internal(Class, Name, Flags, Template, CopyTransientsFromClassDefaults, InInstanceGraph, InExternalPackage));
		return Result;
	}

	template<typename T>
	T* NewModel()
	{
		T* Result = static_cast<T*>(NewModel_Internal());
		return Result;
	}

	template<typename T>
	T* NewModel(FName Name, EObjectFlags Flags = RF_NoFlags, UObject* Template = nullptr, bool CopyTransientsFromClassDefaults = false, FObjectInstancingGraph* InInstanceGraph = nullptr, UPackage* InExternalPackage = nullptr)
	{
		T* Result = static_cast<T*>(NewModel_Internal(Name, Flags, Template, CopyTransientsFromClassDefaults, InInstanceGraph, InExternalPackage));
		return Result;
	}

protected:
	virtual IObjectModel* NewModel_Internal(const UClass* Class, FName Name = NAME_None, EObjectFlags Flags = RF_NoFlags, UObject* Template = nullptr, bool CopyTransientsFromClassDefaults = false, FObjectInstancingGraph* InInstanceGraph = nullptr, UPackage* InExternalPackage = nullptr) = 0;
	virtual IObjectModel* NewModel_Internal() = 0;
	virtual IObjectModel* NewModel_Internal(FName Name, EObjectFlags Flags = RF_NoFlags, UObject* Template = nullptr, bool CopyTransientsFromClassDefaults = false, FObjectInstancingGraph* InInstanceGraph = nullptr, UPackage* InExternalPackage = nullptr) = 0;

protected:
	TWeakObjectPtr<UObject> mModelDefaultOuter = nullptr;
};

/**
 * @brief 실제 게임에서의 모델 팩토리
 */
UCLASS()
class UGameModelFactory : public UObject, public IModelFactory
{
	GENERATED_BODY()

protected:
	IObjectModel* NewModel_Internal(const UClass* Class, FName Name, EObjectFlags Flags, UObject* Template, bool CopyTransientsFromClassDefaults, FObjectInstancingGraph* InInstanceGraph, UPackage* InExternalPackage) override;
	IObjectModel* NewModel_Internal() override;
	IObjectModel* NewModel_Internal(FName Name, EObjectFlags Flags, UObject* Template, bool CopyTransientsFromClassDefaults, FObjectInstancingGraph* InInstanceGraph, UPackage* InExternalPackage) override;
};

/**
 * @brief 시뮬레이션에서의 모델 팩토리
 */
UCLASS()
class USimulationModelFactory : public UObject, public IModelFactory
{
	GENERATED_BODY()

protected:
	IObjectModel* NewModel_Internal(const UClass* Class, FName Name, EObjectFlags Flags, UObject* Template, bool CopyTransientsFromClassDefaults, FObjectInstancingGraph* InInstanceGraph, UPackage* InExternalPackage) override;
	IObjectModel* NewModel_Internal() override;
	IObjectModel* NewModel_Internal(FName Name, EObjectFlags Flags, UObject* Template, bool CopyTransientsFromClassDefaults, FObjectInstancingGraph* InInstanceGraph, UPackage* InExternalPackage) override;
};
