/*****************************************************************//**
 * @file   StaticSkillDataPropertyCustomization.h
 * @brief  StaticSkillData의 에디터 커스텀 디테일 패널 구현 헤더
 * @author 모호재
 * @date   2026-07-24
 *********************************************************************/

#pragma once

#include "RDEditorMinimal.h"
#include "IDetailCustomization.h"
#include "Input/Reply.h"

class IDetailLayoutBuilder;
class UStaticSkillData;

/**
 * @brief UStaticSkillData 전용 디테일 커스터마이제이션 클래스
 */
class FStaticSkillDataPropertyCustomization : public IDetailCustomization
{
public:
	static TSharedRef<IDetailCustomization> MakeInstance();

	/* IDetailCustomization 인터페이스 상속 */
public:
	void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

private:
	FReply OnGenerateDescription() const;

private:
	TArray<TWeakObjectPtr<UStaticSkillData>> mSelectedSkillDatas;
};
