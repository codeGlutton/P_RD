#include "P_RD.h"
#include "Modules/ModuleManager.h"
#if WITH_EDITOR
#include "Audio/RDSoundRouting.h"
#include "Setting/GamePlaySettings.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundClass.h"
#include "UObject/ObjectSaveContext.h"
#include "UObject/UObjectGlobals.h"
#endif

class FRDGameModule : public FDefaultGameModuleImpl
{
public:
	virtual void StartupModule() override
	{
#if WITH_EDITOR
		// Run in cook commandlets as well as the editor. Shipping has no
		// OnAssetLoaded delegate, so fallback buses must be baked into cooked assets.
		FCoreUObjectDelegates::OnAssetLoaded.AddRaw(this, &FRDGameModule::RouteSound);
		FCoreUObjectDelegates::OnObjectPreSave.AddRaw(this, &FRDGameModule::BeforeSave);
#endif
	}
	virtual void ShutdownModule() override
	{
#if WITH_EDITOR
		FCoreUObjectDelegates::OnAssetLoaded.RemoveAll(this);
		FCoreUObjectDelegates::OnObjectPreSave.RemoveAll(this);
#endif
	}
#if WITH_EDITOR
private:
	void RouteSound(UObject* Asset)
	{
		USoundBase* Sound = Cast<USoundBase>(Asset);
		if (!Sound || !Sound->GetPathName().StartsWith(TEXT("/Game/"))) return;
		if (Sound->SoundClassObject && Sound->SoundClassObject->GetPathName().StartsWith(TEXT("/Game/"))) return;
		const int32 Index = static_cast<int32>(RDSoundRouting::Category(Sound->GetPathName()));
		const auto& Classes = GetDefault<UGamePlaySettings>()->mSoundClasses;
		Sound->SoundClassObject = Classes[Index].LoadSynchronous();
	}
	void BeforeSave(UObject* Asset, FObjectPreSaveContext Context) { RouteSound(Asset); }
#endif
};

IMPLEMENT_PRIMARY_GAME_MODULE( FRDGameModule, P_RD, "P_RD" );

DEFINE_LOG_CATEGORY(LogRD)
