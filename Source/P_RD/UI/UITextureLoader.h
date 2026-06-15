/*****************************************************************//**
 * @file   UITextureLoader.h
 * @brief  런타임 UI PNG 텍스처 로더
 * @date   2026-06-14
 *********************************************************************/

#pragma once

#include "RDMinimal.h"

class UTexture2D;

/**
 * @brief Content 폴더의 PNG 파일을 런타임에 읽어 텍스처로 만드는 헬퍼입니다.
 *
 * 타이틀/캐릭터 선택처럼 자주 바뀌는 일러스트는 매번 임포트(.uasset)하지 않고
 * 원본 PNG를 그대로 두고 실행 중에 읽어 쓰기로 했습니다. 그 로딩 절차를 한곳에 모아,
 * 여러 위젯이 ImageWrapper 디코딩과 예외 처리를 각자 반복하지 않게 합니다.
 */
namespace RDUITexture
{
	/**
	 * @brief Content 기준 상대 경로를 디스크의 절대 경로로 바꿉니다.
	 *
	 * 이미 절대 경로면 그대로 두고, 상대 경로면 ProjectContentDir 기준으로 붙입니다.
	 * 호출부는 "SourceArt/UI/Title/..." 같은 Content 상대 경로만 알면 되도록 하기 위한 분리입니다.
	 */
	FString ResolveContentFilePath(const FString& RelativeContentPath);

	/**
	 * @brief Content 상대 경로의 PNG를 읽어 트랜션트 텍스처로 반환합니다.
	 *
	 * 파일이 없거나 디코딩에 실패하면 nullptr를 돌려주고 LogOwner를 붙여 경고만 남깁니다.
	 * 에셋 임포트 파이프라인을 거치지 않으므로, 원본 PNG는 패키징 시 함께 스테이징돼야 합니다.
	 * @param RelativeContentPath Content 폴더 기준 PNG 경로
	 * @param LogOwner            실패 로그에 표시할 호출자 식별 문자열
	 */
	UTexture2D* LoadTextureFromContentPng(const FString& RelativeContentPath, const TCHAR* LogOwner);
}
