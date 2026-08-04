/*****************************************************************//**
 * @file   ActorModel.h
 * @brief  액터 데이터 모델 클래스 정의 헤더
 * @author 모호재
 * @date   2026-06-19
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "ObjectModel.h"
#include "Component/ComponentModel.h"
#include "ActorModel.generated.h"

class IObjectView;

/**
 * @brief  액터 데이터 모델 클래스
 * @details 액터(플레이어, 몬스터 등)의 데이터 모델 베이스 클래스다.
 */
UCLASS(Abstract)
class P_RD_API UActorModel : public UObjectModel
{
	GENERATED_BODY()

	/* UObject 상속 */
public:
	void PostInitProperties() override;
	void PostLoadSubobjects(FObjectInstancingGraph* OuterInstanceGraph) override;

private:
	void ResetComponentModels();

	/* UObjectModel 상속 */
public:
	void Initialize() override;
	void Uninitialize() override;

public:
	virtual void PreInitializeComponentModels() {}
	void InitializeComponentModels();
	virtual void PostInitializeComponentModels() {}

	void UninitializeComponentModels();

public:
	virtual void BeginPlay();
	virtual void EndPlay();

public:
	template<typename T>
	T* AddComponentModelByClass()
	{
		static_assert(TIsDerivedFrom<T, UComponentModel>::IsDerived, "UComponentModel를 상속해야 함");
		return Cast<T>(AddComponentModelByClass(T::StaticClass()));
	}

public:
	template<typename T>
	T* FindComponentModelByClass() const
	{
		static_assert(TPointerIsConvertibleFromTo<T, const UComponentModel>::Value, "'UComponentModel를 상속해야 함");
		return Cast<T>(FindComponentModelByClass(T::StaticClass()));
	}
	template<typename T>
	TArray<T*> FindComponentModelsByClass() const
	{
		static_assert(TPointerIsConvertibleFromTo<T, const UComponentModel>::Value, "'UComponentModel를 상속해야 함");
		TArray<T*> ComponentModels;
		ForEachComponentModel_Internal(T::StaticClass(), [&ComponentModels](T* ComponentModel) {
			ComponentModels.Add(ComponentModel);
			});
		return ComponentModels;
	}
	template<typename T>
	T* FindComponentModelByInterface() const
	{
		static_assert(TIsIInterface<T>::Value, "Interface여야 함");
		return Cast<T>(FindComponentModelByInterface(T::UClassType::StaticClass()));
	}
	template<typename T>
	TArray<T*> FindComponentModelsByInterface() const
	{
		static_assert(TIsIInterface<T>::Value, "Interface여야 함");
		TArray<T*> ComponentModels;
		ForEachComponentModel_Internal(UComponentModel::StaticClass(), [&ComponentModels](UComponentModel* ComponentModel) {
			if (ComponentModel != nullptr && ComponentModel->GetClass()->ImplementsInterface(T::UClassType::StaticClass()))
			{
				ComponentModels.Add(Cast<T>(ComponentModel));
			}
			});
		return ComponentModels;
	}

public:
	UComponentModel* AddComponentModelByClass(UClass* ComponentModelClass);

	UComponentModel* FindComponentModelByInterface(UClass* InterfaceClass) const;
	TArray<UComponentModel*> FindComponentModelsByInterface(UClass* InterfaceClass) const;
	UComponentModel* FindComponentModelByClass(TSubclassOf<UComponentModel> ComponentModelClass) const;
	TArray<UComponentModel*> FindComponentModelsByClass(TSubclassOf<UComponentModel> ComponentModelClass) const;

private:
	template<class ComponentModelType, typename FuncType>
	void ForEachComponentModel_Internal(TSubclassOf<UComponentModel> ComponentModelClass, FuncType Func)
	{
		const UClass* TargetClass = ComponentModelClass.Get();
		if (TargetClass != nullptr)
		{
			check(TargetClass->IsChildOf(ComponentModelType::StaticClass()));

			for (UComponentModel* ComponentModel : mComponentModels)
			{
				if (ComponentModel != nullptr)
				{
					if (ComponentModel->IsA(TargetClass) == true)
					{
						Func(static_cast<ComponentModelType*>(ComponentModel));
					}
				}
			}
		}
	}

public:
	const TSet<TObjectPtr<UComponentModel>>& GetComponentModels() const;

protected:
	UPROPERTY()
	TSet<TObjectPtr<UComponentModel>> mComponentModels;
};
