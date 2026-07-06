/**
 * @file   TitleBgmSubsystem.h
 * @brief  타이틀~캐릭터 선택 구간 배경음악(BGM)을 레벨 전환에도 끊기지 않게 재생하는 GameInstance 서브시스템.
 * @author 박용수
 * @date   2026-07-07
 **/

#pragma once

#include "RDMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include "TitleBgmSubsystem.generated.h"

class UAudioComponent;
class USoundBase;

/**
 * @brief 타이틀 화면 진입 시 BGM을 시작하고, 스테이지로 넘어갈 때(캐릭터 선택 확정/이어하기) 정지한다.
 * @details
 *  프론트엔드는 "타이틀 -> 캐릭터 선택"이 한 흐름이라 그 구간 내내 같은 곡이 끊김 없이 이어져야 한다.
 *  GameInstance 단위(레벨 전환에도 살아 있음) + SpawnSound2D(bPersistAcrossLevelTransition=true)로
 *  프론트엔드 내부의 어떤 화면 전환/레벨 로드에도 재생이 유지된다.
 *  이미 재생 중이면 StartTitleBgm()은 무시(중복 재생/처음부터 다시재생 방지)한다.
 */
UCLASS()
class P_RD_API UTitleBgmSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	/** @brief 타이틀 BGM 재생 시작. 이미 재생 중이면 무시(구간 연속성 유지). */
	void StartTitleBgm();

	/** @brief 타이틀 BGM 정지(스테이지 전환 시). 짧게 페이드아웃 후 종료. */
	void StopTitleBgm();

	/** @brief 현재 BGM이 재생 중인지. */
	bool IsPlaying() const;

protected:
	/** @brief GameInstance 종료 시 정리: 재생 중이면 정지. */
	void Deinitialize() override;

private:
	/** @brief 재생 중인 BGM 오디오 컴포넌트(레벨 전환에도 유지, 자동소멸 X). 루프는 SoundWave.bLooping로 처리. */
	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> mBgmComponent;
};
