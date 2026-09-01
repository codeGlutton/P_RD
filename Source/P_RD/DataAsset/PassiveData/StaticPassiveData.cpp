/*****************************************************************//**
 * @file   StaticPassiveData.cpp
 * @brief  패시브 정적 데이터 구현
 * @author 김준형, 이문환
 * @date   2026-06-18
 *********************************************************************/

#include "DataAsset/PassiveData/StaticPassiveData.h"

#include "TAS/Passive/TacticalPassive_Generic.h"

UStaticPassiveData::UStaticPassiveData()
{
	// 새 DA는 별도 지정 없이 제네릭 패시브로 동작
	mPassiveClass = UTacticalPassive_Generic::StaticClass();
}
