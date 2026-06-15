#pragma once

/**
 * @file UITextureLoader.h
 * @brief PNG 파일을 읽어서 UI 텍스처로 만드는 함수입니다.
 *
 * @details
 * UI 그림이 자주 바뀌는 동안에는 PNG를 매번 uasset으로 만들기 번거롭습니다.
 * 그래서 Content 폴더 안의 PNG를 실행 중에 읽어 텍스처로 바꿉니다.
 * 읽기에 실패하면 nullptr를 돌려주므로, 호출하는 쪽은 그림이 없을 때도 버틸 수 있어야 합니다.
 */

#include "RDMinimal.h"

class UTexture2D;

/**
 * @brief Content 폴더의 PNG를 읽는 함수 모음입니다.
 *
 * @details
 * 여기서 만든 텍스처는 저장되는 에셋이 아니라 게임이 켜져 있는 동안만 쓰는 텍스처입니다.
 * 위젯에서 계속 써야 하면 UPROPERTY로 들고 있어야 합니다.
 */
namespace RDUITexture
{
	/**
	 * @brief Content 기준 경로를 실제 파일 경로로 바꿉니다.
	 *
	 * @details
	 * "SourceArt/UI/Title/A.png"처럼 Content 안의 경로를 넘기면 실제 디스크 경로로 바꿉니다.
	 *
	 * @param RelativeContentPath Content 폴더 기준 경로 또는 이미 완성된 파일 경로.
	 * @return 실제 파일 경로.
	 */
	FString ResolveContentFilePath(const FString& RelativeContentPath);

	/**
	 * @brief PNG 파일을 읽어 UTexture2D로 만듭니다.
	 *
	 * @details
	 * 파일이 없거나 읽기에 실패하면 경고 로그만 남기고 nullptr를 돌려줍니다.
	 * 원본 PNG는 패키징할 때 같이 들어가 있어야 합니다.
	 *
	 * @param RelativeContentPath Content 폴더 기준 PNG 경로.
	 * @param LogOwner 실패 로그에 찍을 이름.
	 * @return 성공하면 텍스처, 실패하면 nullptr.
	 */
	UTexture2D* LoadTextureFromContentPng(const FString& RelativeContentPath, const TCHAR* LogOwner);
}
