/*****************************************************************//**
 * @file   CharacterCardWidget.h
 * @brief  캐릭터 선택 화면의 하단 아이콘 카드 위젯 정의 헤더
 * @date   2026-06-04
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "Frontend/CharacterSelectTypes.h"
#include "UI/RDUserWidget.h"

#include "CharacterCardWidget.generated.h"

class UButton;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCharacterCardClickedDelegate, int32, CharacterIndex);

/**
 * @brief 캐릭터 선택 후보 하나를 아이콘 버튼으로 표시하는 WBP 기반 위젯
 *
 * @details
 * 이 클래스는 캐릭터 카드 "한 장"만 담당한다.
 * 캐릭터 목록을 어디서 가져오는지, Confirm을 눌렀을 때 다음 화면으로 가는지는 모른다.
 * 부모 위젯이 SetCharacterOption()으로 표시할 View 값을 넣어 주면,
 * 이 위젯은 클릭 이벤트와 선택 상태 이벤트만 전달한다.
 * 아이콘/색/배치 같은 비주얼은 WBP_CharacterCard 계열 블루프린트가 가진다.
 *
 * 캐릭터 수가 늘어나도 이 클래스는 바뀌지 않는다.
 * 부모는 WBP에 배치된 카드 순서대로 데이터를 넣고,
 * 클릭 이벤트에 담긴 CharacterIndex로 어떤 캐릭터가 눌렸는지만 판단한다.
 *
 * @note 잠긴 캐릭터도 클릭은 가능하게 둔다.
 * 잠김 카드를 눌렀을 때 상세 영역에 잠김 상태를 보여줄 수 있어야 하기 때문이다.
 * 실제 시작 가능 여부는 부모 캐릭터 선택 위젯의 Confirm 버튼에서 막는다.
 */
UCLASS(BlueprintType, Blueprintable)
class P_RD_API UCharacterCardWidget : public URDUserWidget
{
	GENERATED_BODY()

public:
	UCharacterCardWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/**
	 * @brief 이 카드에 표시할 캐릭터 View 값을 넣음
	 *
	 * @param InOption GameMode가 만든 캐릭터 선택 표시용 데이터
	 * @param bSelected 이 카드가 현재 선택된 카드면 true
	 */
	void SetCharacterOption(const FFrontendCharacterOption& InOption, bool bSelected);

	/**
	 * @brief 선택 표시만 다시 갱신함
	 *
	 * @param bSelected 이 카드가 현재 선택된 카드면 true
	 */
	void SetSelected(bool bSelected);

public:
	/** @brief 카드가 눌리면 부모 위젯으로 캐릭터 index를 알려주는 이벤트 */
	UPROPERTY(Category = "Character Card", BlueprintAssignable)
	FCharacterCardClickedDelegate OnCharacterCardClicked;

protected:
	void NativeConstruct() override;
	void NativeDestruct() override;

	/** @brief WBP에서 선택/잠금 연출을 처리할 수 있게 상태만 알려준다. */
	UFUNCTION(Category = "Character Card", BlueprintImplementableEvent)
	void BP_OnCharacterOptionChanged(const FFrontendCharacterOption& CharacterOption, bool bSelected);

private:
	/** @brief 저장된 캐릭터 값/선택 상태를 WBP_CharacterCard의 위젯들에 반영함 */
	void SyncCard();

	/** @brief WBP_CharacterCard에 필요한 BindWidget 이름이 빠졌는지 로그로 알려줌 */
	void ValidateDesignerBindings() const;

	UFUNCTION()
	void HandleCardButtonClicked();

private:
	/** @brief 카드 전체를 누를 수 있게 하는 버튼 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> CardButton;

private:
	FFrontendCharacterOption mCharacterOption;
	bool mIsSelected = false;
};
