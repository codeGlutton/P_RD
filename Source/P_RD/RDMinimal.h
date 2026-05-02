/*****************************************************************//**
 * @file   RDMinimal.h
 * @brief  RD 프로젝트 전 영역에서 사용되는 최소한의 포함 헤더
 * @details
 * 모든 영역에서 사용되는 헤더를 미리 정의하여, 중복 작성을 방지합니다. \n
 * 단, 해당 영역에 불필요한 헤더를 삽입 시에 컴파일 시간이 길어질 수 있기 때문에 반드시 필요한 항목만 추가합니다.
 * @author 모호재
 * @date   2026-04-23
 *********************************************************************/

#pragma once

 /* RD 프로젝트 연관 기본 선언 포함 헤더 */

#include "P_RD.h"

/* 기본 엔진 헤더 */

#include "Engine.h"
#include "Engine/EngineTypes.h"
#include "EngineMinimal.h"

/* 기본 수학 라이브러리 */

#include "Kismet/KismetMathLibrary.h"

