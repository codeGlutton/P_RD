/*****************************************************************//**
 * @file   TileSelectable.h
 * @brief  SRPG 타일에 선택 가능하여 디테일 정보를 볼 수 있는 객체 인터페이스 정의 헤더
 * @author 모호재
 * @date   2026-05-19
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "UObject/Interface.h"
#include "TileSelectable.generated.h"

UINTERFACE(MinimalAPI)
class UTileSelectable : public UInterface
{
	GENERATED_BODY()
};

/**
 * @brief  SRPG 타일에 선택 가능하여 디테일 정보를 볼 수 있는 객체
 */
class P_RD_API ITileSelectable
{
	GENERATED_BODY()

public:
	virtual bool IsSelectable() const;

protected:
	virtual UUserWidget* GetInfoPanel() const PURE_VIRTUAL(ITileSelectable::GetInfoPanel, return nullptr;)
};
