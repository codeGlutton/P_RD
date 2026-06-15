/*****************************************************************//**
 * @file   UITextureLoader.h
 * @brief  런타임 UI PNG 텍스처 로더
 * @date   2026-06-14
 *********************************************************************/

#pragma once

#include "RDMinimal.h"

class UTexture2D;

namespace RDUITexture
{
	FString ResolveContentFilePath(const FString& RelativeContentPath);
	UTexture2D* LoadTextureFromContentPng(const FString& RelativeContentPath, const TCHAR* LogOwner);
}
