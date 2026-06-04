/*****************************************************************//**
 * @file   PresentationBarrier.h
 * @brief  연출 완료 시기를 연장해주는 배리어 객체 정의 헤더
 * @author 모호재
 * @date   2026-05-28
 *********************************************************************/

#pragma once

#include "RDMinimal.h"

DECLARE_DELEGATE(FOnFinishPresentation);

/**
 * @brief  연출 완료 시기를 연장해주는 배리어 객체 정의 헤더. 해당 객체가 소멸 시 연출 완료로 인식
 */
struct FPresentationBarrier
{
	friend class UPresentationSyncSubsystem;

protected:
	FPresentationBarrier(FOnFinishPresentation Delegate) :
		OnFinishPresentation(Delegate)
	{
	}
	~FPresentationBarrier()
	{
		OnFinishPresentation.ExecuteIfBound();
	}

protected:
	FOnFinishPresentation OnFinishPresentation;
};

