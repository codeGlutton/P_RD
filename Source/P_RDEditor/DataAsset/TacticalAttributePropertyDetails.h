/*****************************************************************//**
 * @file   TacticalAttributePropertyDetails.h
 * @brief  FTacticalAttribute 속성 선택 드롭다운 커스터마이제이션
 * @author 이문환
 * @date   2026-09-01
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "IPropertyTypeCustomization.h"

/**
 * @brief FTacticalAttribute 디테일 패널 커스터마이제이션
 *
 * @details
 * TFieldPath 기본 에디터로는 속성을 고를 수 없어서,
 * 전체 어트리뷰트셋 속성을 모아 검색 가능한 드롭다운으로 제공.
 */
class FTacticalAttributePropertyDetails : public IPropertyTypeCustomization
{
public:
	static TSharedRef<IPropertyTypeCustomization> MakeInstance();

	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> StructPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;
	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;

private:
	// 선택지 목록 구성 ("None" + "속성셋.속성")
	void BuildOptions();

	// 현재 값 표시 텍스트
	FText GetCurrentText() const;

	// 선택 반영
	void OnSelectionChanged(TSharedPtr<FString> NewValue, ESelectInfo::Type SelectInfo);

	// 대상 구조체 핸들
	TSharedPtr<IPropertyHandle> mStructHandle;

	// 드롭다운 선택지
	TArray<TSharedPtr<FString>> mOptions;

	// 표시 문자열 -> 속성
	TMap<FString, FProperty*> mPropertyByOption;
};
