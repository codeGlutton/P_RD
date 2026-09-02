/*****************************************************************//**
 * @file   DynamicPassiveData_Generic.h
 * @brief  데이터 기반 패시브가 전투 중 기억하는 값(카운터, 캡처값)
 * @author 이문환
 * @date   2026-08-29
 *********************************************************************/

#pragma once

#include "TAS/Passive/DynamicPassiveData.h"
#include "SRPGFramework/SRPGFrameworkType.h"
#include "DynamicPassiveData_Generic.generated.h"

/**
 * @brief 캡처 슬롯 (캡처 키 하나에 대한 저장값)
 *
 * @details
 * 캡처 타이밍에 피연산자를 평가해 소유자 값과 타겟별 값을 저장.
 * 발동 타이밍에 Captured 피연산자가 읽어 현재 값과 비교(변화량 판정).
 */
USTRUCT()
struct FPassiveCaptureSlot
{
	GENERATED_BODY()

	// 소유자(Self) 기준 저장값
	UPROPERTY()
	float mSelf = 0.f;

	// 타겟별 저장값 (Ctx.mTargets 인덱스와 짝)
	UPROPERTY()
	TArray<float> mTargets;
};

/**
 * @brief 데이터 기반 패시브 런타임 상태
 *
 * @details
 * UTacticalPassive_Generic이 쓰는 단일 상태 구조체.
 * - 카운터: 발동 타이밍 도달 횟수. 리셋 없이 증가만 하고, 리셋 타이밍 태그에서만 0으로 초기화
 * - 캡처: 캡처 타이밍에 저장한 값(키별) + 타일 위치(이동 여부 판정용)
 */
USTRUCT()
struct FDynamicPassiveData_Generic : public FDynamicPassiveData
{
	GENERATED_BODY()

	// 발동 타이밍 도달 횟수 (조건 통과 여부와 무관하게 증가)
	UPROPERTY()
	int32 mCounter = 0;

	// 캡처 키별 저장값
	UPROPERTY()
	TMap<FName, FPassiveCaptureSlot> mCaptures;

	// 캡처 시점의 소유자 타일 위치 (미캡처 = Invalid)
	UPROPERTY()
	FTileIndex mCapturedSelfTile = FTileIndex::Invalid;

	// 캡처 시점의 타겟별 타일 위치 (Ctx.mTargets 인덱스와 짝)
	UPROPERTY()
	TArray<FTileIndex> mCapturedTargetTiles;
};
