/*****************************************************************//**
 * @file   ComponentView.h
 * @brief  컴포넌트의 시뮬레이션 뷰 최상위 인터페이스 정의 헤더
 * @author 모호재
 * @date   2026-06-18
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "UObject/Interface.h"
#include "ObjectModel.h"
#include "ComponentView.generated.h"

class IActorView;

UINTERFACE(MinimalAPI)
class UComponentView : public UInterface
{
	GENERATED_BODY()
};

/**
 * @brief  컴포넌트의 시뮬레이션 뷰 최상위 인터페이스 정의 헤더
 * @details 컴포넌트가 구현하는 공통 인터페이스다.
 */
class P_RD_API IComponentView
{
	GENERATED_BODY()

public:
	template<typename T>
	T* GetOwnerView() const
	{
		static_assert(TIsDerivedFrom<T, IActorView>::IsDerived, "IActorView를 상속해야함");
		return Cast<T>(GetOwnerView());
	}

	template<typename T>
	T* GetOwnerModel() const
	{
		static_assert(TIsDerivedFrom<T, UObjectModel>::IsDerived, "UObjectModel를 상속해야함");
		return Cast<T>(GetOwnerModel());
	}

public:
	/**
	 * 부모 액터 뷰를 반환
	 * @return 부모 액터 뷰
	 */
	IActorView* GetOwnerView() const;
	/**
	 * 부모 액터 뷰의 모델을 반환
	 * @return 부모 액터 뷰의 모델
	 */
	UObjectModel* GetOwnerModel() const;
	/**
	 * 오너의 모델의 요소를 확인하며 바인딩하는 함수. 초기 생성 시 호출됨.
	 * @param Model 오너의 모델 객체
	 */
	virtual void BindOwnerModel(UObjectModel* Model) = 0;
	virtual void UnbindOwnerModel(UObjectModel* Model) = 0;
};
