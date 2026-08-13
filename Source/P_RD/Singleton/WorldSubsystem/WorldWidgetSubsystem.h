/*****************************************************************//**
 * @file   WorldWidgetSubsystem.h
 * @brief  특정한 소유자 없이 월드에 소속된 Widget 서브시스템 정의 헤더
 * @author 모호재
 * @date   2026-05-22
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "Subsystems/WorldSubsystem.h"

#include "Singleton/WorldSubsystem/WorldWidgetType.h"

#include "WorldWidgetSubsystem.generated.h"

// World Widget 신규 로그 카테고리 등록
DECLARE_LOG_CATEGORY_EXTERN(LogWorldWidget, Log, All)

/**
 * @brief  특정한 소유자 없이 월드에 소속된 Widget 서브시스템
 */
UCLASS()
class P_RD_API UWorldWidgetSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	void InitWidgets(UClass* HUDClass, const TArray<EWorldWidgetType>& WorldWidgetTypes);
	void InitHUD(UClass* HUDClass);
	void InitWorldWidget(EWorldWidgetType WorldWidgetType);

public:
	template<typename T = UUserWidget>
	T* GetHUD() const
	{
		static_assert(TIsDerivedFrom<T, UUserWidget>::IsDerived);

		return Cast<T>(mHUD);
	}

	template<typename T>
	T* GetWorldWidget(EWorldWidgetType Type) const
	{
		static_assert(TIsDerivedFrom<T, UUserWidget>::IsDerived);

		return Cast<T>(GetWorldWidget(Type));
	}
	UUserWidget* GetWorldWidget(EWorldWidgetType Type) const;

#if WITH_DEV_AUTOMATION_TESTS
	/**
	 * @brief 실제 월드 위젯 조회 경로를 유지한 채 UI 입력 계약을 검사할 때 쓸 테스트 주입점.
	 *
	 * @details
	 * 에디터 커맨드렛 월드에는 PlayerController가 없어 InitWorldWidget()이 UMG를
	 * 생성할 수 없다. 테스트가 준비한 위젯을 같은 레지스트리 칸에 넣으면 전투
	 * HUD의 톱니 버튼은 제품 코드와 똑같이 GetWorldWidget()으로 설정창을 찾는다.
	 */
	void SetWorldWidgetForTest(EWorldWidgetType Type, UUserWidget* Widget)
	{
		mWorldWidgets[StaticCast<uint8>(Type)] = Widget;
	}
#endif

protected:
	UPROPERTY(Category = UI, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "HUD"))
	TObjectPtr<UUserWidget> mHUD;

	UPROPERTY(Category = UI, VisibleAnywhere, meta = (DisplayName = "WorldWidgets", ArraySizeEnum = "EWorldWidgetType"))
	TObjectPtr<UUserWidget> mWorldWidgets[static_cast<uint8>(EWorldWidgetType::Count)];
};
