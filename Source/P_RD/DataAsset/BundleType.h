/*****************************************************************//**
 * @file   BundleType.h
 * @brief  사용되는 Bundle Type 매크로를 모아둔 파일
 * @author 모호재
 * @date   2026-05-20
 *********************************************************************/

#pragma once

#include "RDMinimal.h"

/**
 * 주의사항
 * 
 * UPROPERTY 내부 AssetBundles 대입 문자열에는 매크로 사용하지 말고 직접 입력해줄 것.
 */

#define BUNDLE_PAD "PAD"
#define BUNDLE_WORLD "World"
#define BUNDLE_ACTOR "Actor"
#define BUNDLE_UI "UI"

#define BULDLE_ALL BUNDLE_PAD, BUNDLE_WORLD, BUNDLE_ACTOR, BUNDLE_UI
