/*****************************************************************//**
 * @file   UITextureLoader.h
 * @brief  UI 텍스처/미디어 경로 헬퍼
 *
 * @details
 * 무거운 에셋(텍스처)은 팀 규칙대로 SVN 콘텐츠 폴더에 두되, 런타임 PNG 디코드 대신
 * 임포트된 uasset을 쓴다. (PNG 직접 디코드는 매 로드마다 CPU 부하 + 무압축/밉맵 없음 → 폐기)
 * 영상(mp4)만은 uasset이 아니라 파일이라, 디스크 절대경로 변환만 따로 둔다.
 *********************************************************************/

#pragma once

#include "RDMinimal.h"

class UTexture2D;

namespace RDUITexture
{
	/** @brief mp4 등 raw 미디어의 Content 상대경로를 디스크 절대경로로 변환(영상 재생용). */
	FString ResolveContentFilePath(const FString& RelativeContentPath);

	/** @brief 임포트된 텍스처 uasset을 오브젝트 경로(/Game/...)로 로드. 없으면 경고 후 nullptr. */
	UTexture2D* LoadUITexture(const FString& AssetPath, const TCHAR* LogOwner);
}
