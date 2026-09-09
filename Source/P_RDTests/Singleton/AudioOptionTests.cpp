#include "Misc/AutomationTest.h"
#include "Editor.h"
#include "Engine/World.h"
#include "AudioDevice.h"
#include "AudioThread.h"
#include "Sound/AudioSettings.h"
#include "Setting/GamePlaySettings.h"
#include "Singleton/InstanceSubsystem/PersistentData.h"
#include "Sound/SoundWave.h"
#include "Sound/SoundClass.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/Package.h"
#include "SaveGame/SaveCheckpoint.h"
#include "GameFramework/PlayerController.h"
#include "Misc/ScopeExit.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOptionVibrationPersistenceTest, "P_RD.Singleton.Option.VibrationPersistence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FOptionVibrationPersistenceTest::RunTest(const FString&)
{
	UWorld* World = GEditor->GetEditorWorldContext().World();
	auto* PC = World->SpawnActor<APlayerController>();
	if (!TestNotNull(TEXT("Feedback controller"), PC)) return false;
	// Editor worlds do not run the normal game controller registration lifecycle.
	World->AddController(PC);
	ON_SCOPE_EXIT { World->RemoveController(PC); PC->Destroy(); };
	TStrongObjectPtr<UOptionPersistData> Options(NewObject<UOptionPersistData>(World));
	Options->SetVibrationEnabled(false);
	TestFalse(TEXT("Disabling vibration reaches the controller"), bool(PC->bForceFeedbackEnabled));
	TArray<uint8> Bytes;
	TestTrue(TEXT("Serialize saved vibration preference"), RDCheckpoint::Serialize(Options.Get(), Bytes));
	TStrongObjectPtr<UOptionPersistData> Restored(NewObject<UOptionPersistData>(World));
	TestTrue(TEXT("Restore vibration preference"), RDCheckpoint::Deserialize(Bytes, Restored.Get()));
	TestFalse(TEXT("Vibration remains disabled after reload"), Restored->IsVibrationEnabled());
	Restored->SetVibrationEnabled(true);
	TestTrue(TEXT("Re-enabling vibration reaches the controller"), bool(PC->bForceFeedbackEnabled));
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOptionAudioRoutingTest, "P_RD.Singleton.Option.AudioRouting",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FOptionAudioRoutingTest::RunTest(const FString&)
{
	const auto* Settings = GetDefault<UGamePlaySettings>();
	TStrongObjectPtr<UOptionPersistData> Options(NewObject<UOptionPersistData>(GEditor->GetEditorWorldContext().World()));
	// An imported UI wave is often loaded by widget CDOs before the profile exists.
	auto* Click = LoadObject<USoundWave>(nullptr, TEXT("/Game/SVN/OutSideAsset/AICreation/Audio/UISFX/SFX_UI_Click_Scratch003.SFX_UI_Click_Scratch003"));
	if (!TestNotNull(TEXT("Production UI click"), Click)) return false;
	Options->MakeCaches();
	TestEqual(TEXT("Preloaded UI click uses UI volume"), Click->SoundClassObject.Get(), Settings->mSoundClasses[4].LoadSynchronous());
	auto* Title = LoadObject<USoundWave>(nullptr, TEXT("/Game/SVN/OutSideAsset/AICreation/Audio/TitleMusic/BGM_Title_CampfireMoon_ACE_01.BGM_Title_CampfireMoon_ACE_01"));
	if (TestNotNull(TEXT("Production title music"), Title))
		TestEqual(TEXT("Title BGM uses BGM volume"), Title->SoundClassObject.Get(), Settings->mSoundClasses[1].LoadSynchronous());
	// Exercise the late asset notification separately from the startup scan.
	auto* Late = NewObject<USoundWave>(CreatePackage(TEXT("/Game/SVN/Test/UISFX/LateAudioRoutingTest")));
	Late->SoundClassObject = nullptr;
	FCoreUObjectDelegates::OnAssetLoaded.Broadcast(Late);
	TestEqual(TEXT("Newly loaded UI sounds also use UI volume"), Late->SoundClassObject.Get(), Settings->mSoundClasses[4].LoadSynchronous());
	Late->SoundClassObject = Settings->mSoundClasses[3].LoadSynchronous();
	FCoreUObjectDelegates::OnAssetLoaded.Broadcast(Late);
	TestEqual(TEXT("Explicit voice assignment is preserved"), Late->SoundClassObject.Get(), Settings->mSoundClasses[3].LoadSynchronous());
	TestEqual(TEXT("Unclassified/synthesized sounds use the game's SFX bus"),
		GetDefault<UAudioSettings>()->DefaultSoundClassName.TryLoad(), static_cast<UObject*>(Settings->mSoundClasses[2].LoadSynchronous()));
	for (int32 Index : {1,2,3,4})
		TestEqual(TEXT("Every category inherits master volume"), Settings->mSoundClasses[Index].LoadSynchronous()->ParentClass.Get(), Settings->mSoundClasses[0].LoadSynchronous());
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOptionLiveAudioMixTest, "P_RD.Singleton.Option.LiveAudioMix",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FOptionLiveAudioMixTest::RunTest(const FString&)
{
	UWorld* World = GEditor->GetEditorWorldContext().World();
	FAudioDeviceHandle Device = World->GetAudioDevice();
	if (!TestTrue(TEXT("Real audio device required; run without -nosound"), Device.IsValid())) return false;
	struct FState
	{
		TStrongObjectPtr<UOptionPersistData> Options;
		TArray<USoundClass*> Classes;
		TArray<float> Volumes;
		TAtomic<bool> Ready{false};
		bool Queued = false;
		double Started = FPlatformTime::Seconds();
	};
	const auto State = MakeShared<FState>();
	State->Options.Reset(NewObject<UOptionPersistData>(World));
	State->Options->MakeCaches();
	State->Options->ApplyAudioOptions();
	for (int32 Index : {0,1,2,4}) State->Classes.Add(GetDefault<UGamePlaySettings>()->mSoundClasses[Index].LoadSynchronous());
	State->Options->SetVolume(EGameVolumeType::Master, .4f);
	State->Options->SetVolume(EGameVolumeType::BGM, .25f);
	State->Options->SetVolume(EGameVolumeType::SFX, .5f);
	State->Options->SetVolume(EGameVolumeType::UI, 0.f);
	AddCommand(new FFunctionLatentCommand([this, State, Device]() mutable
	{
		if (FPlatformTime::Seconds()-State->Started < 1.0) return false;
		if (!State->Queued)
		{
			State->Queued = true;
			FAudioThread::RunCommandOnAudioThread([State, Device]() mutable
			{
				for (USoundClass* Class : State->Classes)
				{
					const auto* Properties = Device->GetSoundClassCurrentProperties(Class);
					State->Volumes.Add(Properties ? Properties->Volume : -1.f);
				}
				State->Ready.Store(true);
			});
		}
		if (!State->Ready.Load()) return false;
		const float Expected[] = {.4f, .1f, .2f, 0.f};
		for (int32 Index = 0; Index < 4; ++Index)
			TestTrue(*FString::Printf(TEXT("Audio renderer bus %s volume: %.3f (expected %.3f)"), *State->Classes[Index]->GetName(), State->Volumes[Index], Expected[Index]),
				FMath::IsNearlyEqual(State->Volumes[Index], Expected[Index], .01f));
		for (EGameVolumeType Type : {EGameVolumeType::Master,EGameVolumeType::BGM,EGameVolumeType::SFX,EGameVolumeType::UI}) State->Options->SetVolume(Type,1.f);
		return true;
	}));
	return true;
}
#endif
