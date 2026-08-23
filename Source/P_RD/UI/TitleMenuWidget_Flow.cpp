#include "UI/TitleMenuWidget.h"
#include "UI/TitleMenuWidgetPrivate.h"

#include "Components/Button.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "GameMode/FrontendGameMode.h"
#include "Singleton/WorldSubsystem/WorldWidgetSubsystem.h"
#include "UI/SettingsPanelWidget.h"

using namespace RDTitleMenu;

namespace
{
	/**
	 * @brief 프로필 위젯의 실제 배치 캔버스 슬롯을 찾는다.
	 *
	 * 틀·글자·버튼은 WBP에서 XxxMount(Overlay) 하나로 묶여 있고 자리는 그 Mount가 쥔다.
	 * 그래서 이름으로 찾은 위젯이 캔버스에 없으면 위로 올라가며 캔버스 슬롯을 가진 조상을 쓴다.
	 * 부모 이름을 문자열로 넘겨짚던 예전 방식과 달리, 무엇으로 감싸든 통한다.
	 */
	UCanvasPanelSlot* FindProfileCanvasSlot(UUserWidget* Owner, const TCHAR* BaseName, const FName ProfileName)
	{
		if (Owner == nullptr)
		{
			return nullptr;
		}

		UWidget* Widget = Owner->GetWidgetFromName(MakeProfileWidgetName(BaseName, ProfileName));
		for (UWidget* Node = Widget; Node != nullptr; Node = Node->GetParent())
		{
			if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Node->Slot))
			{
				return CanvasSlot;
			}
		}

		return nullptr;
	}

	/** @brief ToBase 프로필 위젯의 세로 위치(Y)만 FromBase 프로필 위젯의 Y로 맞춘다(X/크기는 그대로). */
	void CopyProfileWidgetPositionY(UUserWidget* Owner, const TCHAR* FromBase, const TCHAR* ToBase, const FName ProfileName)
	{
		UCanvasPanelSlot* FromSlot = FindProfileCanvasSlot(Owner, FromBase, ProfileName);
		UCanvasPanelSlot* ToSlot = FindProfileCanvasSlot(Owner, ToBase, ProfileName);
		if (FromSlot == nullptr || ToSlot == nullptr)
		{
			return;
		}

		FVector2D Position = ToSlot->GetPosition();
		Position.Y = FromSlot->GetPosition().Y;
		ToSlot->SetPosition(Position);
	}

	void SetWidgetAndGeneratedParentVisibility(UWidget* Widget, ESlateVisibility Visibility)
	{
		if (Widget == nullptr)
		{
			return;
		}

		Widget->SetVisibility(Visibility);

		// 캔버스에 놓인 조상(XxxMount 또는 런타임 _CenterOverlay)도 같이 숨긴다.
		// 빈 판이 남으면 입력과 레이아웃에 그대로 자리를 차지한다.
		for (UWidget* Node = Widget->GetParent(); Node != nullptr; Node = Node->GetParent())
		{
			if (Cast<UCanvasPanelSlot>(Node->Slot) != nullptr)
			{
				Node->SetVisibility(Visibility);
				break;
			}
		}
	}

	void SetNamedWidgetAndGeneratedParentVisibility(const UUserWidget* Owner, const FName WidgetName, ESlateVisibility Visibility)
	{
		if (Owner == nullptr)
		{
			return;
		}

		SetWidgetAndGeneratedParentVisibility(const_cast<UUserWidget*>(Owner)->GetWidgetFromName(WidgetName), Visibility);
	}

	/** @brief 장식 위젯만 표시 상태를 바꾸고, 같은 Mount를 공유하는 버튼의 히트 테스트는 건드리지 않는다. */
	void SetNamedWidgetVisibilityOnly(const UUserWidget* Owner, const FName WidgetName, ESlateVisibility Visibility)
	{
		if (Owner == nullptr)
		{
			return;
		}

		if (UWidget* Widget = const_cast<UUserWidget*>(Owner)->GetWidgetFromName(WidgetName))
		{
			Widget->SetVisibility(Visibility);
		}
	}

	void SetNamedText(const UUserWidget* Owner, const FName WidgetName, const FText& Text)
	{
		if (Owner == nullptr)
		{
			return;
		}

		if (UTextBlock* TextBlock = Cast<UTextBlock>(const_cast<UUserWidget*>(Owner)->GetWidgetFromName(WidgetName)))
		{
			TextBlock->SetText(Text);
		}
	}

	void SetNamedButtonEnabled(const UUserWidget* Owner, const FName WidgetName, const bool bIsEnabled)
	{
		if (Owner == nullptr)
		{
			return;
		}

		if (UButton* Button = Cast<UButton>(const_cast<UUserWidget*>(Owner)->GetWidgetFromName(WidgetName)))
		{
			Button->SetIsEnabled(bIsEnabled);
		}
	}
}

/** @brief 타이틀에서 공용 설정 패널을 Title 모드로 열어 보여준다. */
// 설정 기능은 타이틀과 인게임에서 같은 WBP_SettingsPanel 월드 위젯을 공유한다.
// 타이틀에서는 저장 후 종료/포기하기 같은 런 액션이 보이면 안 되므로 PanelMode를 Title로 바꾸고 Run 액션 상태를 비활성화한다.
// 왜 타이틀 내부 ScreenSwitcher로 보여주지 않는가:
// 설정 패널은 HUD 하위 화면이 아니라 공용 팝업이다. InGameSettings 월드 위젯을 OpenUI()로 열어야
// 타이틀/인게임 모두 AddToViewport, ZOrder, Back/Close 처리 규칙을 같은 경로로 검증할 수 있다.
void UTitleMenuWidget::OpenSettingsPanel()
{
	USettingsPanelWidget* TitleSettingsPanel = GetTitleSettingsPanel();
	if (TitleSettingsPanel == nullptr)
	{
		SetStatusText(mMainOnlyStatusText);
		return;
	}

	TitleSettingsPanel->OnBackRequested.AddUniqueDynamic(this, &UTitleMenuWidget::HandleSettingsPanelBackRequested);
	TitleSettingsPanel->SetPanelMode(ESettingsPanelMode::Title);
	// 같은 SettingsPanel 인스턴스를 인게임에서도 쓰므로, 타이틀에서만 런 액션 영역을 숨긴 상태로 갱신한다.
	TitleSettingsPanel->RefreshPanelState(false, false);
	TitleSettingsPanel->HideAbandonConfirm();
	TitleSettingsPanel->SetStatusText(FText::GetEmpty());

	TitleSettingsPanel->OpenUI();
	SetStatusText(FText::GetEmpty());
}

/** @brief 현재 활성 Run 여부에 따라 START/NEW START/CONTINUE 버튼 상태를 갱신한다. */
// 세이브 데이터 로드는 Intro 단계에서 끝난다는 전제를 사용한다.
// 타이틀은 디스크를 다시 읽지 않고, FrontendGameMode가 현재 RunPersistData 요약을 만들 수 있는지만 본다.
// 활성 Run이 없어도 새 런 시작 버튼은 항상 NEW START로 보여준다.
// 활성 Run이 있으면 CONTINUE 버튼 묶음까지 같이 보여주고, 없으면 버튼/프레임/텍스트를 모두 숨긴다.
void UTitleMenuWidget::RefreshMainMenuState() const
{
	const bool bCanContinueRun = CanContinueRun();
	const ESlateVisibility ContinueVisibility = bCanContinueRun ? ESlateVisibility::Visible : ESlateVisibility::Collapsed;
	const ESlateVisibility ContinueDecorationVisibility = bCanContinueRun
		? ESlateVisibility::HitTestInvisible
		: ESlateVisibility::Collapsed;

	// 프로필 위젯을 쓰는 판인지 본다. 전에는 스위처가 있느냐로 물었는데,
	// 스위처는 벌이 여럿일 때만 있던 것이고 이제 한 벌뿐이라 없앴다.
	// 물어야 할 것은 처음부터 "접미사 붙은 단추가 있느냐" 였다.
	const bool bUsesProfileLayouts = const_cast<UTitleMenuWidget*>(this)
		->GetWidgetFromName(MakeProfileWidgetName(
			TEXT("StartButton"), TitleLayoutProfileBase16x9)) != nullptr;
	if (bUsesProfileLayouts)
	{
		SetWidgetAndGeneratedParentVisibility(StartButton, ESlateVisibility::Collapsed);
		SetWidgetAndGeneratedParentVisibility(ContinueButton, ESlateVisibility::Collapsed);
		SetWidgetAndGeneratedParentVisibility(SettingsButton, ESlateVisibility::Collapsed);
		SetWidgetAndGeneratedParentVisibility(StartButtonText, ESlateVisibility::Collapsed);
		SetWidgetAndGeneratedParentVisibility(ContinueButtonText, ESlateVisibility::Collapsed);
		SetWidgetAndGeneratedParentVisibility(SettingsButtonText, ESlateVisibility::Collapsed);
		SetNamedWidgetAndGeneratedParentVisibility(this, TEXT("TitleLogoImage"), ESlateVisibility::Collapsed);
		SetNamedWidgetAndGeneratedParentVisibility(this, TEXT("StartButtonFrameImage"), ESlateVisibility::Collapsed);
		SetNamedWidgetAndGeneratedParentVisibility(this, TEXT("ContinueButtonFrameImage"), ESlateVisibility::Collapsed);
		SetNamedWidgetAndGeneratedParentVisibility(this, TEXT("SettingsButtonFrameImage"), ESlateVisibility::Collapsed);
		SetNamedWidgetAndGeneratedParentVisibility(this, TEXT("ExitButtonFrameImage"), ESlateVisibility::Collapsed);
		SetNamedWidgetAndGeneratedParentVisibility(this, TEXT("ExitButtonText"), ESlateVisibility::Collapsed);
		SetNamedWidgetAndGeneratedParentVisibility(this, TEXT("VersionPlateImage"), ESlateVisibility::Collapsed);
		SetNamedWidgetAndGeneratedParentVisibility(this, TEXT("VersionText"), ESlateVisibility::Collapsed);

		for (const FName ProfileName : TitleLayoutProfiles)
		{
			SetNamedWidgetAndGeneratedParentVisibility(this, MakeProfileWidgetName(TEXT("TitleLogoImage"), ProfileName), ESlateVisibility::HitTestInvisible);
			SetNamedWidgetAndGeneratedParentVisibility(this, MakeProfileWidgetName(TEXT("StartButton"), ProfileName), ESlateVisibility::Visible);
			SetNamedWidgetVisibilityOnly(this, MakeProfileWidgetName(TEXT("StartButtonFrameImage"), ProfileName), ESlateVisibility::HitTestInvisible);
			SetNamedWidgetVisibilityOnly(this, MakeProfileWidgetName(TEXT("StartButtonText"), ProfileName), ESlateVisibility::HitTestInvisible);
			SetNamedText(this, MakeProfileWidgetName(TEXT("StartButtonText"), ProfileName), mNewStartButtonText);
			SetNamedButtonEnabled(this, MakeProfileWidgetName(TEXT("StartButton"), ProfileName), true);

			SetNamedWidgetAndGeneratedParentVisibility(this, MakeProfileWidgetName(TEXT("ContinueButton"), ProfileName), ContinueVisibility);
			SetNamedWidgetVisibilityOnly(this, MakeProfileWidgetName(TEXT("ContinueButtonFrameImage"), ProfileName), ContinueDecorationVisibility);
			SetNamedWidgetVisibilityOnly(this, MakeProfileWidgetName(TEXT("ContinueButtonText"), ProfileName), ContinueDecorationVisibility);
			SetNamedText(this, MakeProfileWidgetName(TEXT("ContinueButtonText"), ProfileName), mContinueButtonText);
			SetNamedButtonEnabled(this, MakeProfileWidgetName(TEXT("ContinueButton"), ProfileName), bCanContinueRun);

			SetNamedWidgetAndGeneratedParentVisibility(this, MakeProfileWidgetName(TEXT("SettingsButton"), ProfileName), ESlateVisibility::Visible);
			SetNamedWidgetVisibilityOnly(this, MakeProfileWidgetName(TEXT("SettingsButtonFrameImage"), ProfileName), ESlateVisibility::HitTestInvisible);
			SetNamedWidgetVisibilityOnly(this, MakeProfileWidgetName(TEXT("SettingsButtonText"), ProfileName), ESlateVisibility::HitTestInvisible);
			SetNamedText(this, MakeProfileWidgetName(TEXT("SettingsButtonText"), ProfileName), mSettingsButtonText);
			SetNamedButtonEnabled(this, MakeProfileWidgetName(TEXT("SettingsButton"), ProfileName), true);

			// EXIT과 버전 표기도 장식이 버튼/화면 입력을 가로채지 않게 한다.
			SetNamedWidgetAndGeneratedParentVisibility(this, MakeProfileWidgetName(TEXT("ExitButton"), ProfileName), ESlateVisibility::Visible);
			SetNamedWidgetVisibilityOnly(this, MakeProfileWidgetName(TEXT("ExitButtonFrameImage"), ProfileName), ESlateVisibility::HitTestInvisible);
			SetNamedWidgetVisibilityOnly(this, MakeProfileWidgetName(TEXT("ExitButtonText"), ProfileName), ESlateVisibility::HitTestInvisible);
			SetNamedWidgetVisibilityOnly(this, MakeProfileWidgetName(TEXT("VersionPlateImage"), ProfileName), ESlateVisibility::HitTestInvisible);
			SetNamedWidgetVisibilityOnly(this, MakeProfileWidgetName(TEXT("VersionText"), ProfileName), ESlateVisibility::HitTestInvisible);
		}

		// 세이브가 없어 CONTINUE가 숨겨지면, 캔버스 절대배치라 리플로우가 안 돼 NEW START만 위 슬롯에 홀로 떠 보인다.
		// 이때 NEW START(프레임/버튼/텍스트)를 비어 있는 CONTINUE 슬롯 위치로 내려 SETTING/EXIT 묶음과 붙인다.
		if (bCanContinueRun == false)
		{
			// 틀·글자·버튼이 XxxMount 하나로 묶여 있으므로 자리도 하나만 옮기면 된다.
			// 예전에는 셋을 따로 옮겼고, 하나라도 빠뜨리면 글자만 제자리에 남았다.
			UTitleMenuWidget* MutableSelf = const_cast<UTitleMenuWidget*>(this);
			for (const FName ProfileName : TitleLayoutProfiles)
			{
				CopyProfileWidgetPositionY(MutableSelf, TEXT("ContinueButtonFrameImage"), TEXT("StartButtonFrameImage"), ProfileName);
			}
		}

		return;
	}

	if (StartButton != nullptr)
	{
		StartButton->SetVisibility(ESlateVisibility::Visible);
		StartButton->SetIsEnabled(true);
	}

	if (StartButtonText != nullptr)
	{
		StartButtonText->SetText(mNewStartButtonText);
	}

	if (ContinueButton != nullptr)
	{
		ContinueButton->SetVisibility(ContinueVisibility);
		ContinueButton->SetIsEnabled(bCanContinueRun);
	}

	if (ContinueButtonText != nullptr)
	{
		ContinueButtonText->SetText(mContinueButtonText);
		SetWidgetAndGeneratedParentVisibility(ContinueButtonText, ContinueVisibility);
	}

	SetWidgetAndGeneratedParentVisibility(GetWidgetFromName(TEXT("ContinueButtonFrameImage")), ContinueVisibility);

	if (SettingsButton != nullptr)
	{
		SettingsButton->SetVisibility(ESlateVisibility::Visible);
		SettingsButton->SetIsEnabled(true);
	}
}

/** @brief 현재 프론트엔드 GameMode가 이어가기 가능한 활성 Run을 갖는지 확인한다. */
// 여기서 SaveGame 파일을 직접 읽지는 않는다.
// Intro에서 복구된 PersistentData에 활성 Run이 있는지만 확인해 CONTINUE 버튼 표시 여부를 정한다.
// 현재 방 위치, 난이도, 레벨 같은 방 안 UI 표시는 실제 방에 들어간 뒤 RoomGameMode가 처리한다.
bool UTitleMenuWidget::CanContinueRun() const
{
	if (AFrontendGameMode* FrontendGameMode = GetWorld() != nullptr ? GetWorld()->GetAuthGameMode<AFrontendGameMode>() : nullptr)
	{
		return FrontendGameMode->HasActiveRun();
	}

	return false;
}

/** @brief 타이틀 설정에 사용할 공용 SettingsPanelWidget 월드 위젯을 찾는다. */
// 타이틀 WBP 안에 설정 패널을 직접 소유하지 않는다.
// WorldWidgetSubsystem이 준비한 InGameSettings 월드 위젯만 받아와 OpenUI()/CloseUI() 생명주기를 통일한다.
USettingsPanelWidget* UTitleMenuWidget::GetTitleSettingsPanel() const
{
	UWorld* World = GetWorld();
	UWorldWidgetSubsystem* WorldWidgetSubsystem = World != nullptr ? World->GetSubsystem<UWorldWidgetSubsystem>() : nullptr;
	return WorldWidgetSubsystem != nullptr
		? WorldWidgetSubsystem->GetWorldWidget<USettingsPanelWidget>(EWorldWidgetType::InGameSettings)
		: nullptr;
}

/** @brief START 버튼 입력을 GameMode의 새 런 시작 흐름으로 전달한다. */
// GameMode가 준비되어 있으면 RequestCharacterSelectFromTitle()을 통해 독립 캐릭터 선택 월드 위젯을 연다.
void UTitleMenuWidget::HandleStartButtonClicked()
{
	if (AFrontendGameMode* FrontendGameMode = GetWorld() != nullptr ? GetWorld()->GetAuthGameMode<AFrontendGameMode>() : nullptr)
	{
		if (FrontendGameMode->RequestCharacterSelectFromTitle())
		{
			return;
		}
	}
}

/** @brief CONTINUE 버튼 입력으로 현재 활성 Run의 방에 바로 들어간다. */
// 버튼은 RefreshMainMenuState()에서 활성 Run이 있을 때만 보이지만,
// 클릭 순간에도 다시 검사해 Run 데이터가 비었거나 방 전환을 시작할 수 없으면 메인 화면으로 되돌린다.
// 지도 조회/다음 방 선택은 방에 들어간 뒤 RoomGameMode가 준비한 WorldMap 위젯에서만 처리한다.
void UTitleMenuWidget::HandleContinueButtonClicked()
{
	if (AFrontendGameMode* FrontendGameMode = GetWorld() != nullptr ? GetWorld()->GetAuthGameMode<AFrontendGameMode>() : nullptr)
	{
		if (FrontendGameMode->ContinueRunFromTitle())
		{
			return;
		}
	}

	SetStatusText(mMainOnlyStatusText);
}

/** @brief SETTING 버튼 입력으로 공용 설정 패널을 연다. */
void UTitleMenuWidget::HandleSettingsButtonClicked()
{
	OpenSettingsPanel();
}

/** @brief 설정 화면 Back 요청을 타이틀 메인 복귀로 처리한다. */
// 타이틀 설정도 공용 InGameSettings 월드 위젯으로 열리므로 CloseUI()까지 호출해 팝업 상태를 정리한다.
void UTitleMenuWidget::HandleSettingsPanelBackRequested()
{
	if (USettingsPanelWidget* TitleSettingsPanel = GetTitleSettingsPanel())
	{
		TitleSettingsPanel->CloseUI();
	}

	SetStatusText(FText::GetEmpty());
}
