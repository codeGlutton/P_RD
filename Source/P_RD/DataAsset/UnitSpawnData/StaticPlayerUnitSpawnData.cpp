#include "DataAsset/UnitSpawnData/StaticPlayerUnitSpawnData.h"
#include "Pawn/Player/PlayerUnitModel.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#if WITH_EDITOR
/**
 * @brief 플레이어 유닛 DataAsset의 에디터 유효성을 검사한다.
 * @param Context 유효성 검사 결과 메시지를 누적할 컨텍스트
 * @return 부모 검사 결과와 직업 타입 검사 결과를 합친 Data Validation 결과
 */
EDataValidationResult UStaticPlayerUnitSpawnData::IsDataValid(FDataValidationContext& Context) const
{
    EDataValidationResult SuperResult = Super::IsDataValid(Context);
    EDataValidationResult ThisResult = EDataValidationResult::Valid;

    if (mJobType >= EUnitJobType::PlayerJobCount)
    {
        Context.AddError(FText::FromString(TEXT("잘못된 직업 타입")));
        ThisResult = EDataValidationResult::Invalid;
    }

    return CombineDataValidationResults(SuperResult, ThisResult);
}
#endif

