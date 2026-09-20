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
	virtual void Initialize() {}
	virtual void Uninitialize() {}

public:
	virtual void BeginPlay() {}
	virtual void EndPlay() {}

public:
	template<typename T>
	T* GetOwnerModel() const
	{
		return Cast<T>(GetOwnerModel());
	}

public:
	/**
	 * 부모 액터 모델을 반환
	 * @return 부모 액터 모델
	 */
	UActorModel* GetOwnerModel() const;
};
