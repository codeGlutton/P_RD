/*****************************************************************//**
 * @file   StaticGimmickSpawnData.cpp
 * @brief  기믹 생성 시 사용되는 정적 Primary Data Asset 구현 파일
 * @author 이문환
 * @date   2026-08-20
 *********************************************************************/

#include "DataAsset/ObstacleSpawnData/StaticGimmickSpawnData.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#if WITH_EDITOR
EDataValidationResult UStaticGimmickSpawnData::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult SuperResult = Super::IsDataValid(Context);
	EDataValidationResult ThisResult = EDataValidationResult::Valid;

	// 횟수 0은 의미가 없음 (양수 = 횟수, 음수 = 무제한)
	if (mTriggerCount == 0)
	{
		Context.AddError(FText::FromString(TEXT("발동 횟수 0 금지 (양수 = 횟수, 음수 = 무제한)")));
		ThisResult = EDataValidationResult::Invalid;
	}

	// 발동할 스킬 슬롯이 스킬 목록 안에 있어야 함
	if (mSkillDatas.IsValidIndex(mTriggerSkillIndex) == false)
	{
		Context.AddError(FText::FromString(TEXT("발동 스킬 슬롯 인덱스가 스킬 목록 범위 밖")));
		ThisResult = EDataValidationResult::Invalid;
	}

	return CombineDataValidationResults(SuperResult, ThisResult);
}
#endif
