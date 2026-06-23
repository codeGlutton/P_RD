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
class UObjectModelFactory;

/**
 * @brief  시뮬레이션 데이터 모델의 최상위 인터페이스
 * @details 하위 데이터 모델(보드 액터 모델, 서브시스템 모델 등)이 구현하는 공통 인터페이스다.
 */
UCLASS(abstract)
class P_RD_API UObjectModel : public UObject
{
	GENERATED_BODY()

	friend UObjectModelFactory;

public:
	virtual void Initialize();
	virtual void Uninitialize();

public:
	virtual void PostBindView(TScriptInterface<IObjectView> View);

public:
	void FinishCreating(const FTransform& ViewTransform = FTransform::Identity);
	void Destroy();

public:
	template<typename T>
	T* GetView() const
	{
		return Cast<T>(GetView());
	}

public:
	/**
	 * 참고한 모델을 반환하는 함수
	 * @return 참고한 모델 객체
	 */
	IObjectView* GetView() const;
	int32 GetModelId() const;

protected:
	TWeakObjectPtr<UObject> mView;

protected:
	// @brief 구분용 ID
	UPROPERTY(Category = "Spawn", VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "ModelId"))
	int32 mModelId;

private:
	UPROPERTY()
	bool mIsInitialized = false;
	UPROPERTY()
	bool mIsPendingDestroy = false;
};

