/*****************************************************************//**
 * @file   BoardEventChannelDetailsCustomization.h
 * @brief  보드 이벤트 채널/데이터의 디테일 패널 커스텀 커스터마이즈 정의 헤더
 * @author 모호재
 * @date   2026-08-19
 *********************************************************************/

#pragma once

#include "RDEditorMinimal.h"
#include "IPropertyTypeCustomization.h"

class IPropertyHandle;

/**
 * @brief 보드 이벤트 채널 데이터의 프로퍼티 에디터 커스터마이즈 클래스
 */
class FBoardEventChannelDetailsCustomization : public IPropertyTypeCustomization
{
public:
	static TSharedRef<IPropertyTypeCustomization> MakeInstance();

	/* IPropertyTypeCustomization 상속 */
public:
	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils) override;
	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils) override;
};
