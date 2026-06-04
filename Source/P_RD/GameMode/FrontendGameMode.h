/*****************************************************************//**
 * @file   FrontendGameMode.h
 * @brief  프론트엔드 GameMode 정의 헤더
 * @author Codex
 * @date   2026-06-04
 *********************************************************************/

#pragma once

#include "Frontend/CharacterSelectTypes.h"
#include "Frontend/FrontendViewTypes.h"
#include "GameMode/RoomGameModeBase.h"
#include "Kismet/GameplayStatics.h"
#include "Singleton/InstanceSubsystem/PersistentDataWriter.h"

#include "FrontendGameMode.generated.h"

struct FStage;
struct FStreamableHandle;

/**
 * @brief 타이틀/캐릭터 선택/지도 화면을 한 프론트엔드 월드 안에서 처리하는 GameMode
 *
 * @details
 * HUD 클래스 선택은 BP_FrontendGameMode의 mHUDClass 기본값이 담당한다.
 * C++은 캐릭터 후보, 런 생성, 지도 View 데이터, 방 선택/입장 같은 게임 흐름 API만 제공한다.
 */
UCLASS()
class P_RD_API AFrontendGameMode : public ARoomGameModeBase, public IRunDataWriter
{
	GENERATED_BODY()

public:
	AFrontendGameMode();

	/** @brief 타이틀 START 입력을 처리해 캐릭터 선택 화면을 열도록 요청 */
	UFUNCTION(Category = Title, BlueprintCallable)
	bool StartNewRunFromTitle();

	/** @brief 선택한 플레이어 유닛으로 새 런 준비를 요청 */
	UFUNCTION(Category = Title, BlueprintCallable)
	bool StartRunWithPlayerUnit(FPrimaryAssetId PlayerUnitId);

	/** @brief AssetManager에 등록된 플레이어 유닛 PrimaryAssetId 목록을 가져옴 */
	bool GetPlayerUnitIds(TArray<FPrimaryAssetId>& OutPlayerUnitIds) const;

	/** @brief 캐릭터 선택 UI에 보여줄 카드 View 데이터를 가져옴 */
	UFUNCTION(Category = Title, BlueprintCallable)
	bool GetCharacterOptions(TArray<FFrontendCharacterOption>& OutOptions) const;

	/** @brief 캐릭터 선택용 DataAsset preload가 진행 중인지 확인 */
	UFUNCTION(Category = Title, BlueprintPure)
	bool IsCharacterOptionsLoading() const;

	/** @brief 선택한 플레이어 유닛으로 런을 만들고 지도 preview 생성 시작 */
	UFUNCTION(Category = Title, BlueprintCallable)
	bool PrepareRunMapWithPlayerUnit(FPrimaryAssetId PlayerUnitId);

	/** @brief 지도 UI가 그릴 방 노드 View 데이터를 가져옴 */
	UFUNCTION(Category = Title, BlueprintCallable)
	bool GetMapRoomViews(TArray<FFrontendMapRoomView>& OutRooms) const;

	/** @brief 현재 런이 살아 있는지 확인 */
	UFUNCTION(Category = Title, BlueprintPure)
	bool HasActiveRun() const;

	/** @brief 타이틀/세팅 UI에서 지금 런 저장 버튼을 사용할 수 있는지 조회 */
	UFUNCTION(Category = Title, BlueprintPure)
	bool CanSaveRun() const;

	/** @brief 타이틀/세팅 UI에서 지금 런 포기 버튼을 사용할 수 있는지 조회 */
	UFUNCTION(Category = Title, BlueprintPure)
	bool CanAbandonRun() const;

	/** @brief 지도 첫 표시 위치 계산용 런 상태 View를 가져옴 */
	UFUNCTION(Category = Title, BlueprintCallable)
	bool GetRunControlView(FFrontendRunControlView& OutView) const;

	/** @brief 세팅 화면용 현재 런 상태 조회 */
	bool GetRunControlState(OUT int32& RowIndex, OUT int32& ColumnIndex, OUT int32& PlayerLevel, OUT int32& Difficulty) const;

	/** @brief 타이틀/세팅 UI에서 런 저장을 요청 */
	bool SaveRunFromTitleAsync(FAsyncSaveGameToSlotDelegate Callback) const;

	/** @brief 타이틀/세팅 UI에서 현재 런 포기를 요청 */
	UFUNCTION(Category = Title, BlueprintCallable)
	bool AbandonRunFromTitle();

	/** @brief 지도에서 방 노드 선택 요청 */
	UFUNCTION(Category = Title, BlueprintCallable)
	bool SelectMapRoom(int32 RowIndex, int32 ColumnIndex);

	/** @brief 선택된 방으로 실제 입장 요청 */
	UFUNCTION(Category = Title, BlueprintCallable)
	bool EnterSelectedMapRoom();

protected:
	void InitializeCommonRoom() override;
	void BeginRoom() override;

private:
	void RequestPlayerUnitSpawnDataPreload();
	void HandlePlayerUnitSpawnDataPreloaded(TSharedPtr<FStreamableHandle> AssetHandle);
	bool OpenTitleCharacterSelect();
	bool StartRunPreviewWithPlayerUnit(FPrimaryAssetId PlayerUnitId);
	void HandleStageCreated(const FStage& NewStage);
	void ShowTitleMessage(const FText& Message) const;

private:
	bool bStartRunRequested = false;
	bool bPlayerUnitSpawnDataPreloadRequested = false;
	TSharedPtr<FStreamableHandle> mPlayerUnitSpawnDataPreloadHandle = nullptr;
};
