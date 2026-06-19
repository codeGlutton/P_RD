/*****************************************************************//**
 * @file   ObjectView.h
 * @brief  시뮬레이션 뷰 최상위 인터페이스 정의 헤더
 * @author 이문환
 * @date   2026-06-18
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "UObject/Interface.h"
#include "ObjectView.generated.h"

class UObjectModel;

UINTERFACE(MinimalAPI)
class UObjectView : public UInterface
{
	GENERATED_BODY()
};

/**
 * @brief  시뮬레이션 뷰의 최상위 인터페이스
 * @details 모든 하위 뷰(보드 액터, 서브시스템 등)가 구현하는 공통 인터페이스다.
 */
class P_RD_API IObjectView
{
	GENERATED_BODY()

public:
	template<typename T = UObjectModel>
	T* GetModel() const
	{
		static_assert(TIsDerivedFrom<T, UObjectModel>::IsDerived, "UObjectModel를 상속해야함");
		return Cast<T>(GetModel_Internal());
	}

public:
	/**
	 * 모델의 요소를 확인하며 바인딩하는 함수. 초기 생성 시 호출됨.
	 * @param Model 모델 객체
	 */
	virtual void BindModel(UObjectModel* Model) = 0;
	virtual void UnbindModel(UObjectModel* Model) = 0;

protected:
	/**
	 * 참고한 모델을 반환하는 함수
	 * @return 참고한 모델 객체
	 */
	virtual UObjectModel* GetModel_Internal() const = 0;
};
