/*****************************************************************//**
 * @file   ComponentModel.h
 * @brief  보드 액터에 부착되는 컴포넌트 데이터 모델 클래스 정의 헤더
 * @author 이문환
 * @date   2026-06-18
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "ObjectModel.h"
#include "ComponentModel.generated.h"

class UActorModel;

/**
 * @brief  액터에 부착되는 컴포넌트 데이터 모델 클래스
 * @details 패시브, 장비 등 액터 모델이 소유하는 컴포넌트 모델이다.
 */
UCLASS()
class P_RD_API UComponentModel : public UObject
{
	GENERATED_BODY()

public:
	virtual void Initialize() PURE_VIRTUAL(UComponentModel::Initialize, return;);
	virtual void Uninitialize() PURE_VIRTUAL(UComponentModel::Uninitialize, return;);

public:
	virtual void BeginPlay() PURE_VIRTUAL(UComponentModel::BeginPlay, return;);
	virtual void EndPlay() PURE_VIRTUAL(UComponentModel::EndPlay, return;);

public:
	template<typename T>
	T* GetOwnerModel() const
	{
		static_assert(TIsDerivedFrom<T, UActorModel>::IsDerived, "UActorModel를 상속해야함");
		return Cast<T>(GetOwnerModel());
	}

public:
	/**
	 * 부모 액터 모델을 반환
	 * @return 부모 액터 모델
	 */
	UActorModel* GetOwnerModel() const;
};
