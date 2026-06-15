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
class UCanvasPanel;
class UFrontendMapWidget;
class USettingsPanelWidget;
class UTextBlock;
class URDUserWidget;

/**
 * @brief WBP_TopMenuBar의 버튼 입력을 받아 필요한 월드 위젯을 OpenUI/CloseUI로 전환한다.
 *
 * 전투 승리 여부와 현재 방 정보는 게임 모드/서브시스템에서 조회하고, 실제 지도/설정 화면의 표시 방식은
 * 각 위젯의 OpenUI/CloseUI 구현에 맡긴다.
 *
 * 탑바는 전투 결과를 계산하지 않고 SRPGCombatSubsystem의 종료 이벤트를 구독한다. 플레이어 승리 후에는
 * 지도 위젯을 열어 다음 방 선택을 안내하고, 사용자가 SET을 열었다 닫아도 승리 후 지도 표시 상태가
 * 유지되도록 복원만 담당한다.
 *
 * 왜 탑바를 공통 진입점으로 두는가:
 * 방 HUD마다 MAP/SET 버튼을 따로 만들면 방 종류가 늘 때마다 열기 규칙과 닫기 규칙이 갈라진다.
 * 탑바가 WorldWidgetSubsystem의 공용 위젯만 열고 닫으면 전투/상점/보물 방 모두 같은 OpenUI 경로를 쓴다.
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
	 * @brief 모바일에서 최상단 버튼을 안정적으로 누를 수 있도록 투명 터치 영역을 덧댄다.
	 *
	 * @details
	 * WBP_TopMenuBar의 DICE/SKILL 버튼은 화면 최상단에 가까워, 기기 상태바/제스처 영역과 겹치기 쉽다.
	 * 시각 배치는 그대로 두고 살짝 아래쪽까지 입력 영역만 확장해 실제 터치가 같은 토글 핸들러로 들어오게 한다.
	 */
	void EnsureRuntimeTopBarHitAreas();

	/**
	 * @brief 런타임 투명 터치 버튼 하나의 배치와 입력 방식을 설정한다.
	 *
	 * @param Button 설정할 투명 터치 버튼
	 * @param Anchors 화면 비율 기준 터치 영역
	 */
	void ConfigureRuntimeHitArea(UButton* Button, const FAnchors& Anchors) const;

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
	 * @brief 전투 종료 UI 이벤트 구독을 해제한다.
	 */
	void UnbindCombatEvents();

	/**
	 * @brief WorldWidgetSubsystem에서 RDUserWidget 기반 월드 위젯을 가져온다.
	 *
	 * @param WorldWidgetType 가져올 월드 위젯 타입
	 * @return 요청한 위젯이 URDUserWidget이면 해당 포인터, 아니면 nullptr
	 */
	URDUserWidget* GetToggleableWorldWidget(EWorldWidgetType WorldWidgetType) const;

	/**
	 * @brief 월드맵/설정 패널에서 탑바로 들어오는 요청 이벤트를 해제한다.
	 */
	void UnbindPanelEvents();

	/**
	 * @brief 월드 위젯이 준비되어 있으면 공통 CloseUI() 흐름으로 닫는다.
	 *
	 * @param WorldWidgetType 닫을 월드 위젯 타입
	 */
	void CloseWorldWidget(EWorldWidgetType WorldWidgetType) const;

	/**
	 * @brief 탑바에서 여는 플로팅 패널 중 지정한 패널을 제외하고 모두 닫는다.
	 *
	 * @details
	 * MAP/SET/DICE/SKILL은 같은 화면 위에 떠 있는 팝업이다.
	 * 한 번에 여러 개가 열리면 입력 대상이 헷갈리므로 새 패널을 열기 전 기존 패널을 정리한다.
	 *
	 * @param ExceptWorldWidgetType 닫지 않고 유지할 패널 타입
	 */
	void CloseFloatingPanels(EWorldWidgetType ExceptWorldWidgetType) const;

	/**
	 * @brief MAP 버튼 입력에 따라 월드맵을 열거나 닫는다.
	 *
	 * @details
	 * 일반 클릭으로 연 월드맵은 조회용이므로 방 선택을 비활성화한다.
	 * 승리 후 강제 지도 상태에서는 사용자가 맵을 닫아도 다시 복원되어 다음 방 선택 흐름이 유지된다.
	 *
	 * 왜 조회용 지도에서는 선택을 끄는가:
	 * MAP 버튼은 진행 상황 확인용이다. 여기서 노드 선택이 가능하면 사용자가 정보를 보려다 실수로 다음 방 전환을 시작할 수 있다.
	 */
	void ToggleWorldMap();

	/**
	 * @brief SET 버튼 입력에 따라 인게임 설정 패널을 열거나 닫는다.
	 *
	 * @details
	 * 설정 패널을 열 때 월드맵은 잠시 닫고, 승리 후 지도 잠금 상태라면 설정을 닫은 뒤 월드맵을 복원한다.
	 *
	 * 왜 월드맵과 동시에 열지 않는가:
	 * 두 팝업이 동시에 떠 있으면 Back/Close 입력의 대상이 불명확해진다.
	 * 한 번에 하나만 열어야 사용자가 현재 조작 중인 화면을 명확히 알 수 있다.
	 */
	void ToggleSettingsPanel();

	/**
	 * @brief DICE/SKILL처럼 단순히 열고 닫는 플로팅 패널을 토글한다.
	 *
	 * @details
	 * 아직 실제 주사위/스킬 사용 로직은 각 패널에 붙지 않았지만, WBP가 있는 패널은 공통 OpenUI 경로로 동작해야 한다.
	 * 이 함수는 탑바 버튼과 월드 위젯 연결만 책임진다.
	 *
	 * @param WorldWidgetType 열고 닫을 패널 타입
	 * @param DebugName 설정 누락 로그에 표시할 이름
	 */
	void ToggleFloatingPanel(EWorldWidgetType WorldWidgetType, const TCHAR* DebugName);

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
	 *
	 * 왜 barrier를 월드맵 OpenUI 뒤에 푸는가:
	 * 전투 종료 연출이 끝났는데 다음 방 선택 UI가 아직 안 뜨면 흐름이 끊겨 보인다.
	 * 월드맵이 실제로 열린 뒤 barrier를 해제해야 승리 연출에서 선택 화면까지 자연스럽게 이어진다.
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

	/**
	 * @brief SET 버튼 클릭을 설정 패널 토글로 연결한다.
	 */
	UFUNCTION()
	void HandleSettingsButtonClicked();

	/**
	 * @brief DICE 버튼 클릭을 주사위 패널 토글로 연결한다.
	 *
	 * @details
	 * 현재 단계에서는 실제 주사위 사용이 아니라 WBP_DicePanel 표시와 터치 반응 확인만 담당한다.
	 */
	UFUNCTION()
	void HandleDiceButtonClicked();

	/**
	 * @brief SKILL 버튼 클릭을 스킬 패널 토글로 연결한다.
	 *
	 * @details
	 * 현재 단계에서는 실제 스킬 발동이 아니라 WBP_SkillPanel 표시와 카드 선택 흐름 확인만 담당한다.
	 */
	UFUNCTION()
	void HandleSkillButtonClicked();

	/**
	 * @brief 월드맵 위젯의 닫기 요청을 현재 흐름에 맞게 처리한다.
	 */
	UFUNCTION()
	void HandleWorldMapCloseRequested();

	/**
	 * @brief 설정 패널 Back 요청을 현재 흐름에 맞게 처리한다.
	 */
	UFUNCTION()
	void HandleSettingsBackRequested();

private:
	/** @brief 월드맵을 여는 버튼 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> MapButton;

	/** @brief 설정 패널을 여는 버튼 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> SettingsButton;

	/** @brief 주사위 패널을 여는 버튼 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> DiceButton;

	/** @brief 스킬 패널을 여는 버튼 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> SkillButton;

	/** @brief MAP 버튼 라벨 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> MapButtonText;

	/** @brief SET 버튼 라벨 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SettingsButtonText;

	/**
	 * @brief DICE 버튼 라벨
	 *
	 * @details
	 * 아직 실제 주사위 보유 수와 연결하지 않았으므로 기본 문구는 DICE 0으로 둔다.
	 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> DiceButtonText;

	/**
	 * @brief SKILL 버튼 라벨
	 *
	 * @details
	 * 아직 실제 사용 가능 스킬 수와 연결하지 않았으므로 기본 문구는 SKILL 0으로 둔다.
	 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SkillButtonText;

	/** @brief 현재 방 제목을 표시하는 텍스트 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TitleTextBlock;

	/** @brief 현재 런 요약 정보를 표시하는 텍스트 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SummaryTextBlock;

	/** @brief DICE 버튼 아래쪽까지 입력을 넓히는 런타임 투명 터치 영역 */
	UPROPERTY(Transient)
	TObjectPtr<UButton> mRuntimeDiceHitButton;

	/** @brief SKILL 버튼 아래쪽까지 입력을 넓히는 런타임 투명 터치 영역 */
	UPROPERTY(Transient)
	TObjectPtr<UButton> mRuntimeSkillHitButton;

	/**
	 * @brief 승리 후 다음 방 선택을 완료할 때까지 월드맵 복원을 강제할지 여부
	 *
	 * @details
	 * 플레이어가 승리 후 SET을 열거나 MAP을 닫아도 다음 방 선택 흐름이 사라지면 안 되므로,
	 * 이 플래그가 켜진 동안에는 월드맵을 닫는 대신 다시 복원한다.
	 *
	 * 왜 잠금이 필요한가:
	 * 승리 후 다음 방을 고르지 않으면 런 진행이 멈춘다. 사용자가 설정을 잠깐 열 수는 있어도,
	 * 선택해야 하는 월드맵 자체가 사라지면 다음 행동을 잃어버리기 쉽다.
	 *
	 * 왜 여기서 별도 해제하지 않는가:
	 * 다음 방 입장이 확정되면 방 전환 시스템이 새 레벨을 열고, 월드에 속한 TopMenuBar 위젯도 새로 준비된다.
	 * 즉 이 플래그는 "현재 방에서 다음 방 선택을 마칠 때까지"만 유지되는 상태다.
	 * 나중에 같은 월드를 유지한 채 방만 교체하는 구조로 바뀌면, 전환 확정 시점에 이 플래그를 명시적으로 내려야 한다.
	 */
	bool mVictoryWorldMapLocked = false;
};
