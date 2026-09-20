/*****************************************************************//**
 * @file   FrontendMapLandscapeWidget.h
 * @brief  시안 기반 가로형 월드맵 전용 런타임 위젯
 *********************************************************************/

#pragma once

#include "UI/FrontendMapGraphWidgets.h"
#include "UI/FrontendMapWidget.h"

#include "FrontendMapLandscapeWidget.generated.h"

/**
 * 기존 세로형 WBP_FrontendMap을 대체하는 가로형 월드맵 전용 클래스다.
 * 진행 데이터와 방 전환 API는 부모 구현을 재사용하되 배치 규칙만 가로형으로 선택한다.
 */
UCLASS(BlueprintType, Blueprintable)
class P_RD_API UFrontendMapLandscapeWidget : public UFrontendMapWidget
{
	GENERATED_BODY()

public:
	/** @brief 메인 WBP CDO에 직렬화된 연결선 클래스를 반환한다. */
	TSubclassOf<UFrontendMapLineWidget> GetLandscapeLineWidgetClass() const
	{
		return GetMapLineWidgetClass();
	}

	/** @brief 메인 WBP CDO에 직렬화된 노드 클래스를 반환한다. */
	TSubclassOf<UFrontendMapNodeWidget> GetLandscapeNodeWidgetClass() const
	{
		return GetMapNodeWidgetClass();
	}

#if WITH_EDITOR
	/** @brief 에디터 빌더가 메인 WBP의 하드 클래스 참조를 CDO 기본값으로 기록한다. */
	void SetLandscapeGraphWidgetClassesForEditor(
		TSubclassOf<UFrontendMapLineWidget> InLineWidgetClass,
		TSubclassOf<UFrontendMapNodeWidget> InNodeWidgetClass);
#endif

protected:
	bool IsLandscapeLayout() const override { return true; }
};

/** @brief 새 가로형 지도 전용 연결선 WBP 베이스. */
UCLASS(BlueprintType, Blueprintable)
class P_RD_API UFrontendMapLandscapeLineWidget : public UFrontendMapLineWidget
{
	GENERATED_BODY()

public:
	UFrontendMapLandscapeLineWidget(
		const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
};

/** @brief 새 가로형 지도 전용 노드 WBP 베이스. */
UCLASS(BlueprintType, Blueprintable)
class P_RD_API UFrontendMapLandscapeNodeWidget : public UFrontendMapNodeWidget
{
	GENERATED_BODY()

public:
	UFrontendMapLandscapeNodeWidget(
		const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	UTexture2D* GetTypeIconTexture(ERoomType RoomType) const override;
	UTexture2D* GetStateRingTexture(EMapRoomState RoomState, bool bIsCurrentRoom) const override;
	float GetVisualScale(ERoomType RoomType, EMapRoomState RoomState, bool bIsCurrentRoom) const override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> mLandscapeMonster;
	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> mLandscapeElite;
	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> mLandscapeBoss;
	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> mLandscapeShop;
	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> mLandscapeTreasure;
};
