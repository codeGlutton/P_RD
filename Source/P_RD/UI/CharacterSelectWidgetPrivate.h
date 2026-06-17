#pragma once

#include "RDMinimal.h"

namespace RDCharacterSelect
{
	/** @brief 선택 화면 전용 Text 키를 한 곳에서 해석해 생성자/상태 갱신 코드의 문자열 중복을 줄인다. */
	P_RD_API FText Text(const TCHAR* Key);
}
