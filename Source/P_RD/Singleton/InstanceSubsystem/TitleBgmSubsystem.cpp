#include "Singleton/InstanceSubsystem/TitleBgmSubsystem.h"

#include "Components/AudioComponent.h"
#include "Sound/SoundBase.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	// 타이틀 BGM(SVN 관리 uasset). 문자열 경로로 LoadObject -> DefaultGame.ini의 DirectoriesToAlwaysCook로 쿡 보장.
	// (다른 SVN 텍스처들과 동일한 방식. 쿠커가 문자열 로드 의존성을 자동감지 못하므로 폴더를 AlwaysCook에 등록해 둔다.)
	const TCHAR* const TitleBgmAssetPath = TEXT("/Game/SVN/OutSideAsset/AICreation/Audio/TitleMusic/BGM_Title_CampfireMoon_ACE_01.BGM_Title_CampfireMoon_ACE_01");
}

void UTitleBgmSubsystem::StartTitleBgm()
{
	// 이미 재생 중이면 그대로 둔다(타이틀->캐릭터 선택 재진입 등에서 처음부터 다시재생 방지).
	if (IsPlaying() == true)
	{
		return;
	}

	UWorld* World = GetGameInstance() != nullptr ? GetGameInstance()->GetWorld() : nullptr;
	if (World == nullptr)
	{
		return;
	}

	USoundBase* Bgm = LoadObject<USoundBase>(nullptr, TitleBgmAssetPath);
	if (Bgm == nullptr)
	{
		UE_LOG(LogRD, Warning, TEXT("[TitleBgm] BGM 사운드 로드 실패: %s"), TitleBgmAssetPath);
		return;
	}

	// 레벨 전환에도 유지(bPersistAcrossLevelTransition=true), 자동소멸 X(bAutoDestroy=false).
	// 루프는 SoundWave의 bLooping=true(임포트 시 설정)로 처리된다.
	mBgmComponent = UGameplayStatics::SpawnSound2D(
		World, Bgm,
		/*VolumeMultiplier*/1.0f, /*PitchMultiplier*/1.0f, /*StartTime*/0.0f,
		/*ConcurrencySettings*/nullptr,
		/*bPersistAcrossLevelTransition*/true,
		/*bAutoDestroy*/false);
}

void UTitleBgmSubsystem::StopTitleBgm()
{
	if (mBgmComponent != nullptr)
	{
		// 뚝 끊지 않고 짧게 페이드아웃 후 종료.
		mBgmComponent->FadeOut(0.6f, 0.0f);
		mBgmComponent = nullptr;
	}
}

bool UTitleBgmSubsystem::IsPlaying() const
{
	return mBgmComponent != nullptr && mBgmComponent->IsPlaying();
}

void UTitleBgmSubsystem::Deinitialize()
{
	if (mBgmComponent != nullptr)
	{
		mBgmComponent->Stop();
		mBgmComponent = nullptr;
	}

	Super::Deinitialize();
}
