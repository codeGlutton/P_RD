#include "Singleton/WorldSubsystem/WorldCameraSubsystem.h"
#include "Singleton/WorldSubsystem/WorldCameraModel.h"

void UWorldCameraSubsystem::BindModel(UObjectModel* Model)
{
	mWorldCameraModel = Cast<UWorldCameraModel>(Model);

	// 스킬 실행 때마다 발생하던 자동 중앙 이동/확대 요청은 의도적으로 구독하지 않는다.
	// 전투 카메라는 입장 시점의 위치와 OrthoWidth를 끝까지 유지한다.
}

void UWorldCameraSubsystem::UnbindModel(UObjectModel* Model)
{
	mWorldCameraModel.Reset();
}

UObjectModel* UWorldCameraSubsystem::GetModel_Internal() const
{
	return mWorldCameraModel.Get();
}

