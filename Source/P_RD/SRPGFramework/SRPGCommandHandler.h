/*****************************************************************//**
 * @file   SRPGCommandHandler.h
 * @brief  SRPG 명령을 처리할 수 있는 객체 인터페이스 정의 헤더
 * @author 모호재
 * @date   2026-06-15
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "UObject/Interface.h"
#include "SRPGFramework/SRPGFrameworkType.h"
#include "SRPGCommandHandler.generated.h"

struct FSRPGCommand;
struct FSRPGWorldTraceCommand;

UINTERFACE(MinimalAPI)
class USRPGCommandHandler : public UInterface
{
	GENERATED_BODY()
};

/**
 * @brief  SRPG 명령을 처리할 수 있는 객체 인터페이스
 */
class P_RD_API ISRPGCommandHandler
{
	GENERATED_BODY()

	friend class USRPGCommandRouterModel;

protected:
	/**
	 * 명령 처리 우선순위를 반환하는 함수
	 * @return 명령 처리 우선순위 값
	 */
	virtual int8 GetCommandPriority() const = 0;
	/**
	 * 명령을 처리하는 함수
	 * @param Command 명령 객체
	 * @return 명령 처리 결과
	 */
	virtual ESRPGCommandResult HandleCommand(const TInstancedStruct<FSRPGCommand>& Command) = 0;

	/* 헬퍼 함수 */
protected:
	/**
	 * 커서가 어떤 타일 액터를 가리키는지 알아오는 함수. 대상이 없는 경우 FTileIndex::Invalid를 반환
	 * @param World 월드 객체
	 * @param Channel 검사할 트래이스 채널명
	 * @param ScreenPosition 트레이스할 화면 좌표(픽셀). (-1,-1)이면 PC 마우스 커서를 사용. 모바일 터치는 커서가 없으므로 이 좌표로 트레이스한다.
	 * @param Actor 측정된 액터
	 * @param TileIndex 부딧친 대상의 타일의 인덱스 값
	 */
	static void GetTileActorUnderCursor(UWorld* World, ECollisionChannel Channel, const FVector2D& ScreenPosition, OUT AActor*& Actor, OUT FTileIndex& TileIndex);
	static void GetTileActorUnderCursor(UWorld* World, ECollisionChannel Channel, OUT AActor*& Actor, OUT FTileIndex& TileIndex);

	/** 명령에 확정 타일이 있으면 그대로 사용하고, 없을 때만 화면 좌표를 트레이스한다. */
	static void GetTileActorForCommand(UWorld* World, ECollisionChannel Channel,
		const FSRPGWorldTraceCommand& Command, OUT AActor*& Actor,
		OUT FTileIndex& TileIndex);

public:
	static constexpr int8 HIGHEST_PRIORITY = INT8_MAX;
	static constexpr int8 LOWEST_PRIORITY = INT8_MIN;
};
