/*****************************************************************//**
 * @file   ObjectModel.h
 * @brief  시뮬레이션 데이터 모델 최상위 인터페이스 정의 헤더
 * @author 이문환
 * @date   2026-06-17
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "ObjectModel.generated.h"

class IObjectView;

/**
 * @brief  시뮬레이션 데이터 모델의 최상위 인터페이스
 * @details 하위 데이터 모델(보드 액터 모델, 서브시스템 모델 등)이 구현하는 공통 인터페이스다.
 */
UCLASS(abstract)
class P_RD_API UObjectModel : public UObject
{
	GENERATED_BODY()

public:
	virtual void Initialize();
	virtual void Uninitialize();

public:
	virtual void PostBindView(TScriptInterface<IObjectView> View);

public:
	virtual void BeginPlay();
	virtual void EndPlay();

public:
	template<typename T>
	T* GetView() const
	{
		static_assert(TIsDerivedFrom<T, IObjectView>::IsDerived, "IObjectView를 상속해야함");
		return Cast<T>(GetView());
	}

public:
	/**
	 * 참고한 모델을 반환하는 함수
	 * @return 참고한 모델 객체
	 */
	IObjectView* GetView() const;

protected:
	TWeakObjectPtr<UObject> mView;
};

