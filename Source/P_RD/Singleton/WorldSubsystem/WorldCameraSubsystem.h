/*****************************************************************//**
 * @file   WorldCameraSubsystem.h
 * @brief  월드 공간 상의 카메라 제어 서브시스템 정의 헤더
 * @author 모호재
 * @date   2026-07-10
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "ObjectView.h"
#include "Subsystems/WorldSubsystem.h"
#include "WorldCameraSubsystem.generated.h"

class UWorldCameraModel;

/**
 * @brief  월드 공간 상의 카메라 제어 서브시스템 정의
 */
UCLASS()
class P_RD_API UWorldCameraSubsystem : public UWorldSubsystem, public IObjectView
{
	GENERATED_BODY()

	/* IObjectView 상속 */
public:
	void BindModel(UObjectModel* Model) override;
	void UnbindModel(UObjectModel* Model) override;
	void Deinitialize() override;

protected:
	UObjectModel* GetModel_Internal() const override;

protected:
	UPROPERTY(Category = Model, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "WorldCameraModel"))
	TWeakObjectPtr<UWorldCameraModel> mWorldCameraModel;
};
