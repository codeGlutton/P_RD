/*****************************************************************//**
 * @file   BoardSelectionTarget.h
 * @brief  SRPG 타일에 선택 가능하여 디테일 정보를 볼 수 있는 모델 인터페이스 정의 헤더
 * @author 모호재
 * @date   2026-05-19
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "UObject/Interface.h"
#include "BoardSelectionTarget.generated.h"

UINTERFACE(MinimalAPI)
class UBoardSelectionTarget : public UInterface
{
	GENERATED_BODY()
};

/**
 * @brief  SRPG 타일에 선택 가능하여 디테일 정보를 볼 수 있는 객체
 */
class P_RD_API IBoardSelectionTarget
{
	GENERATED_BODY()

public:
	virtual bool IsSelectable() const;

public:
	virtual UUserWidget* GetInfoPanel() const PURE_VIRTUAL(IBoardSelectionTarget::GetInfoPanel, return nullptr;)

	/** @brief 선택 대상의 모델 ID(유닛 상세 매칭용). 유닛만 유효, 나머지는 INDEX_NONE. */
	virtual int32 GetSelectionModelId() const { return INDEX_NONE; }
};
