/*****************************************************************//**
 * @file   TacticalFrameworkModel.h
 * @brief  TAS의 글로벌 서브시스템 모델 정의 헤더
 * @author 모호재
 * @date   2026-06-23
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "ObjectModel.h"
#include "TAS/Effect/ActiveTacticalEffect.h"
#include "TacticalFrameworkModel.generated.h"

class UAttributeSetComponentModel;

// Tactical Framework 신규 로그 카테고리 등록
DECLARE_LOG_CATEGORY_EXTERN(LogTacticalFramework, Log, All)

/**
 * @brief  SRPG 유저 명령을 전달하는 서브시스템 모델
 */
UCLASS()
class P_RD_API UTacticalFrameworkModel : public UObjectModel
{
	GENERATED_BODY()

	friend struct FActiveTacticalEffectHandle;

protected:
	UPROPERTY(Category = "Effect", VisibleAnywhere, meta = (DisplayName = "EffectOwningModelMap"))
	TMap<FActiveTacticalEffectHandle, TWeakObjectPtr<UAttributeSetComponentModel>> mEffectOwningModelMap;
};
