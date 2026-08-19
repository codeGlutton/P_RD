/*****************************************************************//**
 * @file   P_RDEditorModule.h
 * @brief  에디터 모듈 정의 헤더
 * @author 모호재
 * @date   2026-05-13
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"

#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"

class FP_RDEditorModule : public IModuleInterface
{
public:
    void StartupModule() override;
    void ShutdownModule() override;

private:
    FDelegateHandle mBoardEventTrackEditorHandle;
};

