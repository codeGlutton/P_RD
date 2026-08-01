#include "P_RDEditorModule.h"
#include "PropertyEditorModule.h"
#include "DataAsset/SkillData/StaticSkillData.h"
#include "DataAsset/SkillData/StaticSkillDataPropertyCustomization.h"

IMPLEMENT_GAME_MODULE(FP_RDEditorModule, P_RDEditor);

void FP_RDEditorModule::StartupModule()
{
	/* 커스텀 디테일 레이아웃 등록 */

	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	if (FModuleManager::Get().IsModuleLoaded("PropertyEditor") == true)
	{
		PropertyModule.RegisterCustomClassLayout(
			UStaticSkillData::StaticClass()->GetFName(),
			FOnGetDetailCustomizationInstance::CreateStatic(&FStaticSkillDataPropertyCustomization::MakeInstance)
		);
	}
}

void FP_RDEditorModule::ShutdownModule()
{
	/* 커스텀 디테일 레이아웃 등록 해제 */

	if (FModuleManager::Get().IsModuleLoaded("PropertyEditor") == true)
	{
		FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
		PropertyModule.UnregisterCustomClassLayout(UStaticSkillData::StaticClass()->GetFName());
	}
}