/**
 * @file TopMenuBarWidget.h
 * @brief 인게임 상단 메뉴에서 월드맵/설정 패널을 여닫는 위젯.
 */

#pragma once

#include "RDMinimal.h"
#include "SRPGFramework/SRPGFrameworkType.h"
#include "Singleton/WorldSubsystem/WorldWidgetType.h"
#include "UI/RDUserWidget.h"

#include "TopMenuBarWidget.generated.h"

struct FPresentationBarrier;

class UButton;
class UFrontendMapWidget;
class USettingsPanelWidget;
class UTextBlock;
class URDUserWidget;

/**
 * @brief WBP_TopMenuBar의 버튼 입력을 받아 필요한 월드 위젯을 OpenUI/CloseUI로 전환한다.
 *
 * 전투 승리 여부와 현재 방 정보는 GameMode/Subsystem에서 조회하고, 실제 지도/설정 화면의 표시 방식은
 * 각 위젯의 OpenUI/CloseUI 구현에 맡긴다.
 *
 * 탑바는 전투 결과를 계산하지 않고 SRPGCombatSubsystem의 종료 이벤트를 구독한다. 플레이어 승리 후에는
 * 지도 위젯을 열어 다음 방 선택을 안내하고, 사용자가 SET을 열었다 닫아도 승리 후 지도 표시 상태가
 * 유지되도록 복원만 담당한다.
 */
UCLASS(BlueprintType, Blueprintable)
class P_RD_API UTopMenuBarWidget : public URDUserWidget
{
	GENERATED_BODY()

public:
	/**
	 * @brief 탑바의 ZOrder와 기본 표시 정책을 초기화한다.
	 *
	 * @details
	 * 탑바는 일반 팝업보다 위에 있어야 하지만, 배경 전체가 입력을 막으면 게임 조작과 월드맵 입력을 방해한다.
	 * 생성자에서는 표시 순서만 정하고 실제 입력 통과 처리는 Construct/Open 시점에 다시 적용한다.
	 *
	 * @param ObjectInitializer Unreal 객체 생성에 사용하는 기본 초기화 값
	 */
	UTopMenuBarWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/** @brief 탑바 배경은 터치를 통과시키고 버튼만 입력을 받도록 설정한다. */
	void ApplyInputPassThrough();

	/** @brief 타이틀/연출용 표시 상태처럼 입력 없이 정보만 보이는 상태로 바꾼다. */
	void ApplyDisplayOnly();

protected:
	/**
	 * @brief WBP 바인딩 검증, 버튼 연결, 전투 종료 이벤트 구독을 수행한다.
	 */
	void NativeConstruct() override;

	/**
	 * @brief 탑바가 구독한 버튼/월드 위젯/전투 이벤트를 해제한다.
	 */
	void NativeDestruct() override;

	/**
	 * @brief OpenUI()로 표시될 때 방 정보와 입력 통과 설정을 다시 맞춘다.
	 */
	void ApplyOpenUI() override;

private:
	/**
	 * @brief WBP_TopMenuBar의 버튼 클릭 이벤트를 C++ 핸들러에 연결한다.
	 */
	void BindButtonEvents();

	/**
	 * @brief NativeConstruct()에서 연결한 버튼 클릭 이벤트를 해제한다.
	 */
	void UnbindButtonEvents();

	/**
	 * @brief 탑바 버튼이 모바일 터치에서 즉시 눌리도록 입력 방식을 보정한다.
	 *
	 * @param Button 입력 방식을 보정할 버튼
	 */
	void ConfigureDesignerButton(UButton* Button) const;

	/**
	 * @brief WBP_TopMenuBar에 필요한 선택 바인딩이 연결되어 있는지 로그로 확인한다.
	 */
	void ValidateDesignerBindings() const;

	/**
	 * @brief 탑바 기본 문구를 WBP TextBlock에 동기화한다.
	 */
	void SyncDefaultText() const;

	/**
	 * @brief 현재 방/런 상태를 상단 요약 텍스트에 반영한다.
	 */
	void RefreshRoomInfo() const;

	/**
	 * @brief 전투 종료 UI 이벤트를 구독해 승리 후 월드맵 흐름을 시작한다.
	 *
	 * @details
	 * 탑바가 전투 결과를 계산하지 않고, 전투 서브시스템이 보낸 결과만 받아 다음 UI 흐름을 연다.
	 */
	void BindCombatEvents();

	/**
	 * @brief WorldWidgetSubsystem에서 RDUserWidget 기반 월드 위젯을 가져온다.
	 *
	 * @param WorldWidgetType 가져올 월드 위젯 타입
	 * @return 요청한 위젯이 URDUserWidget이면 해당 포인터, 아니면 nullptr
	 */
	URDUserWidget* GetToggleableWorldWidget(EWorldWidgetType WorldWidgetType) const;

	/**
	 * @brief 월드 위젯이 준비되어 있으면 공통 CloseUI() 흐름으로 닫는다.
	 *
	 * @param WorldWidgetType 닫을 월드 위젯 타입
	 */
	void CloseWorldWidget(EWorldWidgetType WorldWidgetType) const;

	/**
	 * @brief MAP 버튼 입력에 따라 월드맵을 열거나 닫는다.
	 *
	 * @details
	 * 일반 클릭으로 연 월드맵은 조회용이므로 방 선택을 비활성화한다.
	 * 승리 후 강제 지도 상태에서는 사용자가 맵을 닫아도 다시 복원되어 다음 방 선택 흐름이 유지된다.
	 */
	void ToggleWorldMap();

	/**
	 * @brief SET 버튼 입력에 따라 인게임 설정 패널을 열거나 닫는다.
	 *
	 * @details
	 * 설정 패널을 열 때 월드맵은 잠시 닫고, 승리 후 지도 잠금 상태라면 설정을 닫은 뒤 월드맵을 복원한다.
	 */
	void ToggleSettingsPanel();

	/**
	 * @brief 전투 종료 결과를 받아 플레이어 승리일 때 다음 방 선택 지도를 연다.
	 *
	 * @param Barrier 전투 종료 연출이 끝나기 전까지 흐름을 붙잡는 presentation barrier
	 * @param Result 전투 결과
	 */
	void HandleEndCombatUI(TSharedPtr<FPresentationBarrier> Barrier, ESRPGCombatResult Result);

	/**
	 * @brief 승리 후 다음 방 선택이 가능한 월드맵을 연다.
	 *
	 * @param Barrier 월드맵이 표시된 뒤 해제할 presentation barrier
	 */
	void OpenWorldMapAfterPlayerWin(TSharedPtr<FPresentationBarrier> Barrier);

	/**
	 * @brief 승리 후 지도 잠금 상태라면 월드맵을 다시 연다.
	 */
	void RestoreVictoryWorldMap();

	/**
	 * @brief 설정 패널을 닫은 뒤 승리 후 월드맵 상태를 복원한다.
	 */
	void CloseSettingsPanelAndRestoreVictoryWorldMap();

	UFUNCTION()
	void HandleMapButtonClicked();

	UFUNCTION()
	void HandleSettingsButtonClicked();

	UFUNCTION()
	void HandleDiceButtonClicked();

	UFUNCTION()
	void HandleSkillButtonClicked();

	UFUNCTION()
	void HandleWorldMapCloseRequested();

	UFUNCTION()
	void HandleSettingsBackRequested();

private:
	/** @brief 월드맵을 여는 버튼 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> MapButton;

	/** @brief 설정 패널을 여는 버튼 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> SettingsButton;

	/** @brief 주사위 팝업용 자리. 이 브랜치에서는 아직 기능 연결 전이다. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> DiceButton;

	/** @brief 스킬 팝업 또는 디버그 승리 버튼용 자리 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> SkillButton;

	/** @brief MAP 버튼 라벨 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> MapButtonText;

	/** @brief SET 버튼 라벨 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SettingsButtonText;

	/** @brief DICE 버튼 라벨 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> DiceButtonText;

	/** @brief SKILL 버튼 또는 디버그 WIN 버튼 라벨 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SkillButtonText;

	/** @brief 현재 방 제목을 표시하는 텍스트 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TitleTextBlock;

	/** @brief 현재 런 요약 정보를 표시하는 텍스트 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SummaryTextBlock;

	/**
	 * @brief 승리 후 다음 방 선택을 완료할 때까지 월드맵 복원을 강제할지 여부
	 *
	 * @details
	 * 플레이어가 승리 후 SET을 열거나 MAP을 닫아도 다음 방 선택 흐름이 사라지면 안 되므로,
	 * 이 플래그가 켜진 동안에는 월드맵을 닫는 대신 다시 복원한다.
	 */
	bool mVictoryWorldMapLocked = false;
};
