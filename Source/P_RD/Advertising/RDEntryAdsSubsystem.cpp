#include "Advertising/RDEntryAdsSubsystem.h"
#include "Misc/ConfigCacheIni.h"

#if PLATFORM_ANDROID
#include "Android/AndroidApplication.h"
#include "Android/AndroidJNI.h"
#include "Async/Async.h"

namespace { TWeakObjectPtr<URDEntryAdsSubsystem> AndroidEntryAds; }

JNI_METHOD void Java_com_epicgames_unreal_GameActivity_nativeRDEntryAdFinished(
	JNIEnv*, jobject, jint Ticket)
{
	AsyncTask(ENamedThreads::GameThread, [Ticket]()
	{
		if (URDEntryAdsSubsystem* Ads = AndroidEntryAds.Get()) Ads->FinishEntry(Ticket);
	});
}
#endif

void URDEntryAdsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
#if PLATFORM_ANDROID
	AndroidEntryAds = this;
#endif
}

void URDEntryAdsSubsystem::Deinitialize()
{
	CancelEntry();
#if PLATFORM_ANDROID
	if (AndroidEntryAds.Get() == this) AndroidEntryAds.Reset();
#endif
	Super::Deinitialize();
}

bool URDEntryAdsSubsystem::IsEnabled()
{
#if PLATFORM_ANDROID
	bool Enabled = false;
	GConfig->GetBool(TEXT("RD.BetaAds"), TEXT("bEnabled"), Enabled, GEngineIni);
	return Enabled;
#else
	return false;
#endif
}

void URDEntryAdsSubsystem::Prepare()
{
#if PLATFORM_ANDROID
	if (!IsEnabled()) return;
	if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
	{
		jmethodID Method = FJavaWrapper::FindMethod(Env, FJavaWrapper::GameActivityClassID,
			"AndroidThunkJava_RDPrepareEntryAd", "()V", true);
		if (Method && FJavaWrapper::GameActivityThis)
			FJavaWrapper::CallVoidMethod(Env, FJavaWrapper::GameActivityThis, Method);
	}
#endif
}

void URDEntryAdsSubsystem::BeforeEntry(FSimpleDelegate Continuation)
{
	const int32 Ticket = EntryGate.Begin(MoveTemp(Continuation));
	if (Ticket == 0) return;
#if PLATFORM_ANDROID
	if (IsEnabled())
	{
		if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
		{
			jmethodID Method = FJavaWrapper::FindMethod(Env, FJavaWrapper::GameActivityClassID,
				"AndroidThunkJava_RDShowEntryAd", "(I)V", true);
			if (Method && FJavaWrapper::GameActivityThis)
			{
				FJavaWrapper::CallVoidMethod(Env, FJavaWrapper::GameActivityThis, Method, Ticket);
				return;
			}
		}
	}
#endif
	FinishEntry(Ticket);
}

void URDEntryAdsSubsystem::CancelEntry() { EntryGate.Cancel(); }
void URDEntryAdsSubsystem::FinishEntry(int32 Ticket) { EntryGate.Complete(Ticket); }
