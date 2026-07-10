/*****************************************************************//**
 * @file   UITextureLoader.h
 * @brief  UI 미디어 경로 헬퍼
 *
 * @details
 * 텍스처는 임포트된 uasset을 위젯에서 TSoftObjectPtr로 직접 참조한다(별도 로더 불필요).
 * 영상(mp4)만은 uasset이 아니라 파일이라, MediaPlayer가 열 수 있게 디스크 절대경로 변환만 여기 둔다.
 *********************************************************************/

#pragma once

#include "RDMinimal.h"

namespace RDUITexture
{
	/** @brief mp4 등 raw 미디어의 Content 상대경로를 MediaPlayer가 열 수 있는 디스크 절대경로로 변환한다. */
	FString ResolveContentFilePath(const FString& RelativeContentPath);
}
