/*****************************************************************//**
 * @file   SaveGameArchive.h
 * @brief  게임 세이브 로드를 돕기 위한 아카이브 객체 구현 헤더
 * @author 모호재
 * @date   2026-05-10
 *********************************************************************/

#include "RDMinimal.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h" 

/**
 * @brief  게임 세이브 로드를 돕기 위한 아카이브
 */
struct FSaveGameArchive : public FObjectAndNameAsStringProxyArchive
{
    FSaveGameArchive(FArchive& InInnerArchive);
};