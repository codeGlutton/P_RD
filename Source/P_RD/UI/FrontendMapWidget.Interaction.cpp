#include "UI/FrontendMapWidget.h"

#include "Components/Button.h"
#include "GameMode/RoomGameModeBase.h"
#include "UI/FrontendMapViewPolicy.h"

using namespace RDFrontendMap;

/**
 * @brief 지도 노드 클릭을 방 선택 GameMode API로 전달한다.
 *
 * @details
 * 지도는 선택 가능 여부를 UI에서 한 번 막고, 최종 유효성은 RoomGameMode의 SelectNextRoom()에 맡긴다.
 *
 * 왜 최종 검증을 GameMode에 맡기는가:
 * UI는 표시된 노드를 보고 입력을 막을 수 있지만, 런 진행 중 전환 요청이나 저장 상태 같은 게임 규칙은 알 수 없다.
 * GameMode가 마지막으로 검사해야 잘못된 노드 클릭이 실제 방 선택으로 이어지지 않는다.
 */
void UFrontendMapWidget::HandleMapRoomClicked(int32 RowIndex, int32 ColumnIndex)
{
	if (mEnterRequested)
	{
		return;
	}

	if (ARoomGameModeBase* RoomGameMode = GetWorld() != nullptr ? GetWorld()->GetAuthGameMode<ARoomGameModeBase>() : nullptr)
	{
		if (RoomGameMode->SelectNextRoom(RowIndex, ColumnIndex))
		{
			RefreshMap();
			SetMapStatusText(mStatusOverrideText.IsEmpty() ? mMapReadyStatusText : mStatusOverrideText);
		}
	}
}

/**
 * @brief 노드 위젯의 클릭 이벤트를 지도 선택 처리로 연결한다.
 */
void UFrontendMapWidget::HandleMapNodeClicked(int32 RowIndex, int32 ColumnIndex)
{
	HandleMapRoomClicked(RowIndex, ColumnIndex);
}

/**
 * @brief 닫기 버튼 입력을 외부 닫기 요청 이벤트로 전달한다.
 */
void UFrontendMapWidget::HandleCloseButtonClicked()
{
	OnCloseRequested.Broadcast();
}

/**
 * @brief 선택된 방 입장을 요청하고 중복 입력을 막는다.
 *
 * @details
 * 실제 프리로드와 전환은 RoomGameMode가 수행한다.
 * UI는 요청 중 상태 문구와 버튼 라벨만 바꾸고, 실패하면 다시 입력 가능한 상태로 되돌린다.
 *
 * 왜 UI에서 직접 전환하지 않는가:
 * 방 전환은 프리로드, 페이드, 로딩 알림, 저장 상태와 함께 움직이는 게임 흐름이다.
 * 지도는 "입장하고 싶다"는 요청만 보내야 전환 정책이 한 곳에 남는다.
 */
void UFrontendMapWidget::HandleEnterRoomButtonClicked()
{
	if (mEnterRequested)
	{
		return;
	}

	mEnterRequested = true;
	SetEnterButtonText(mLoadingStatusText);
	SetMapStatusText(mLoadingStatusText);

	if (ARoomGameModeBase* RoomGameMode = GetWorld() != nullptr ? GetWorld()->GetAuthGameMode<ARoomGameModeBase>() : nullptr)
	{
		if (RoomGameMode->EnterSelectedRoom())
		{
			return;
		}
	}

	mEnterRequested = false;
	SetEnterButtonText(mEnterText);
	SetMapStatusText(mMapUnavailableStatusText);
}

/**
 * @brief 현재 지도 화면에서 실제 방 선택/입장 API를 호출해도 되는지 확인한다.
 *
 * @details
 * 선택 모드가 켜져 있고, 현재 월드의 GameMode가 방 선택 API를 제공할 때만 true를 반환한다.
 *
 * 왜 GameMode 존재까지 확인하는가:
 * 같은 위젯은 타이틀/프론트 화면에서도 배치될 수 있다. 방 GameMode가 없는 곳에서 노드 입력을 열면
 * 사용자는 선택 가능한 것처럼 보지만 실제로는 처리할 대상이 없다.
 */
bool UFrontendMapWidget::IsFrontendMapNavigationEnabled() const
{
	return mRoomSelectionEnabled && GetWorld() != nullptr && GetWorld()->GetAuthGameMode<ARoomGameModeBase>() != nullptr;
}
