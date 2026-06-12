#include "DataAsset/UnitSpawnData/StaticUnitSpawnData.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#if WITH_EDITOR
EDataValidationResult UStaticUnitSpawnData::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult SuperResult = Super::IsDataValid(Context);
	EDataValidationResult ThisResult = EDataValidationResult::Valid;

	if (mClass == nullptr || mDisplayName.IsEmpty() == true)
	{
		Context.AddError(FText::FromString(TEXT("액터 클래스 혹은 이름 미지정")));
		ThisResult = EDataValidationResult::Invalid;
	}

	return CombineDataValidationResults(SuperResult, ThisResult);
}
#endif

/**
 * @brief AttributeSet 기본값 테이블에서 사용할 row key를 만든다.
 *
 * @details
 * mDisplayName은 캐릭터 선택 화면에 보여줄 UI 텍스트라서 "Knight"처럼 짧게 바뀔 수 있다.
 * 하지만 GAS AttributeSet 기본값 테이블의 row는 실제 유닛 클래스 이름인 "KnightPlayerUnit" 기준으로 저장되어 있다.
 * 따라서 기본 스탯을 찾을 때는 표시명이 아니라 mClass의 에셋 이름을 우선 사용해야 한다.
 *
 * 블루프린트 클래스는 에셋 이름 뒤에 "_C"가 붙을 수 있으므로 제거해서 row 이름과 맞춘다.
 * 클래스가 비어 있는 편집 중 상태에서는 기존 표시명 기반 데이터를 완전히 끊지 않기 위해 mDisplayName을 대체값으로 사용한다.
 *
 * @return GAS AttributeSet 기본값 조회에 사용할 row key
 */
FName UStaticUnitSpawnData::GetKeyName() const
{
	FString Key;
	if (mClass.IsNull() == false)
	{
		Key = mClass.ToSoftObjectPath().GetAssetName();
		Key.RemoveFromEnd(TEXT("_C"));
	}

	if (Key.IsEmpty() == true)
	{
		Key = mDisplayName.ToString();
	}
	Key.RemoveSpacesInline();
	return *Key;
}

