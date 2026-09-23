/*****************************************************************//**
 * @file   TestObjectScope.h
 * @brief  테스트용 UObject 생성/정리 스코프
 * @details
 * RunTest 안에서 만든 UObject를 등록해 두었다가 스코프 종료 시 일괄 파괴.
 * 등록된 객체는 스코프가 살아 있는 동안 GC에서 보호됨 (FGCObject).
 * 파괴는 등록 역순. UObjectModel 계열은 인게임 파괴 경로와 같이
 * EndPlay(액터만) -> Uninitialize 후 MarkAsGarbage.
 * @author 이문환
 * @date   2026-09-23
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "UObject/GCObject.h"
#include "Actor/ActorModel.h"

/**
 * @brief 테스트용 UObject 생성/정리 스코프
 * @details RunTest 첫 줄에 로컬로 선언. New로 만든 객체는 이른 return을 포함한
 *          모든 종료 경로에서 파괴됨. 테스트 본문은 EndPlay/Uninitialize를 직접 부르지 않음
 */
class FTestObjectScope : public FGCObject
{
public:
	FTestObjectScope() = default;
	~FTestObjectScope();

	// 복사 금지 (파괴가 두 번 일어나지 않도록)
	FTestObjectScope(const FTestObjectScope&) = delete;
	FTestObjectScope& operator=(const FTestObjectScope&) = delete;

public:
	// @brief 객체 생성 + 등록 (NewObject 대체)
	template<typename T>
	T* New(UObject* Outer = GetTransientPackage(), UClass* Class = nullptr);

	// @brief 외부에서 만든 객체 등록 (DuplicateObject 등)
	template<typename T>
	T* Add(T* Object);

	// @brief 파괴 직전에 실행할 정리 작업 등록 (등록 역순 실행)
	void Defer(TFunction<void()> Task);

public:
	// FGCObject: 등록 객체를 참조로 보고해 테스트 도중 GC 방지
	void AddReferencedObjects(FReferenceCollector& Collector) override;
	FString GetReferencerName() const override;

private:
	// @brief 등록된 객체 (등록 순서 유지)
	TArray<TObjectPtr<UObject>> mObjects;
	// @brief 파괴 전 실행할 정리 작업
	TArray<TFunction<void()>> mDeferred;
};

// @brief 객체 생성 + 등록 (NewObject 대체)
template<typename T>
T* FTestObjectScope::New(UObject* Outer, UClass* Class)
{
	T* Object = Class != nullptr ? NewObject<T>(Outer, Class) : NewObject<T>(Outer);
	return Add(Object);
}

// @brief 외부에서 만든 객체 등록 (DuplicateObject 등)
template<typename T>
T* FTestObjectScope::Add(T* Object)
{
	if (Object != nullptr)
	{
		mObjects.Add(Object);
	}
	return Object;
}

// @brief 파괴 직전에 실행할 정리 작업 등록 (등록 역순 실행)
inline void FTestObjectScope::Defer(TFunction<void()> Task)
{
	mDeferred.Add(MoveTemp(Task));
}

// @brief 등록 역순으로 정리 작업 실행 후 객체 파괴
inline FTestObjectScope::~FTestObjectScope()
{
	// 1. 파괴 전 정리 작업 (등록 해제 등) 역순 실행
	for (int32 Index = mDeferred.Num() - 1; Index >= 0; --Index)
	{
		mDeferred[Index]();
	}
	mDeferred.Reset();

	// 2. 객체 파괴 (등록 역순: 컴포넌트 -> 유닛 -> 타일맵 순으로 내려감)
	for (int32 Index = mObjects.Num() - 1; Index >= 0; --Index)
	{
		UObject* Object = mObjects[Index];
		if (IsValid(Object) == false)
		{
			continue;
		}

		// 인게임 파괴 경로와 동일
		if (UObjectModel* Model = Cast<UObjectModel>(Object))
		{
			if (UActorModel* ActorModel = Cast<UActorModel>(Model))
			{
				ActorModel->EndPlay();
			}
			Model->Uninitialize();
		}

		// 외부에서 루트에 매단 객체는 먼저 풀어야 GC 대상이 됨
		if (Object->IsRooted())
		{
			Object->RemoveFromRoot();
		}
		Object->MarkAsGarbage();
	}
	mObjects.Reset();
}

// @brief 테스트 도중 GC가 돌아도 살아남게 GC 참조로 등록
inline void FTestObjectScope::AddReferencedObjects(FReferenceCollector& Collector)
{
	Collector.AddReferencedObjects(mObjects);
}

// @brief GC 디버그 출력용 참조자 이름
inline FString FTestObjectScope::GetReferencerName() const
{
	return TEXT("FTestObjectScope");
}
