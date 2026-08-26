/*****************************************************************//**
 * @file   StaticPuddleGimmickSpawnData.cpp
 * @brief  장판 기믹 생성 시 사용되는 정적 Primary Data Asset 구현 파일
 * @author 이문환
 * @date   2026-08-21
 *********************************************************************/

#include "DataAsset/ObstacleSpawnData/StaticPuddleGimmickSpawnData.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#if WITH_EDITOR
EDataValidationResult UStaticPuddleGimmickSpawnData::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult SuperResult = Super::IsDataValid(Context);
	EDataValidationResult ThisResult = EDataValidationResult::Valid;

	// 라운드 수명 0은 의미가 없음 (양수 = 라운드 수, 음수 = 무제한)
	if (mRoundLifetime == 0)
	{
		Context.AddError(FText::FromString(TEXT("라운드 수명 0 금지 (양수 = 라운드 수, 음수 = 무제한)")));
		ThisResult = EDataValidationResult::Invalid;
	}

	return CombineDataValidationResults(SuperResult, ThisResult);
}
#endif
