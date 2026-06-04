/*****************************************************************//**
 * @file   CharacterCardWidget.h
 * @brief  캐릭터 카드 하나를 표시하는 위젯 정의 헤더
 * @author Codex
 * @date   2026-06-03
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Frontend/CharacterSelectTypes.h"

#include "CharacterCardWidget.generated.h"

class UButton;
class UBorder;
class UImage;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCharacterCardClickedDelegate, int32, CharacterIndex);

/**
 * @brief 캐릭터 선택 화면 아래에 보이는 캐릭터 아이콘 버튼 하나를 담당하는 위젯
 *
 * @details
 * 이 위젯은 "캐릭터 하나를 아이콘 버튼으로 어떻게 보여줄지"만 알고 있다.
 * 캐릭터 목록을 어디서 가져오는지, Confirm을 누르면 어느 맵으로 가는지는 알지 않는다.
 *
 * CharacterSelectWidget이 캐릭터 수만큼 이 위젯을 만들고,
 * SetCharacterOption()으로 표시할 값을 넣어준다.
 * 카드가 눌리면 OnCharacterCardClicked 델리게이트로 자신의 인덱스만 부모에게 알려준다.
 *
 * 실제 카드 배치는 WBP_CharacterCard에서 만든다.
 * C++은 CardButton, IconBackground, IconImage를 BindWidget으로 받아서
 * 클릭 이벤트, 선택 표시, 임시 아이콘 색만 갱신한다.
 *
 * 현재 기본 화면에서는 Warrior, Archer, Magician 이름을 카드 안에 쓰지 않는다.
 * 화면 아래쪽에는 아이콘 슬롯만 보이고, 실제 아이콘 에셋이 준비되면
 * FFrontendCharacterOption::mIcon 값을 통해 이 슬롯에 넣으면 된다.
 *
 * @note
 * C++ fallback 카드 화면은 만들지 않는다.
 * WBP_CharacterCard에 CardButton, IconBackground, IconImage 이름이 있어야 한다.
 *
 * @note
 * 잠긴 캐릭터도 아이콘은 클릭 가능하다.
 * 그래야 Archer, Magician을 눌렀을 때 상세 영역에서 잠김 상태를 보여줄 수 있다.
 * 실제 게임 시작 가능 여부는 CharacterSelectWidget의 Confirm 버튼 활성화와 Confirm 처리에서 막는다.
 */
UCLASS(BlueprintType, Blueprintable)
class P_RD_API UCharacterCardWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/**
	 * @brief 기본 카드 문구를 준비함
	 *
	 * @param ObjectInitializer Unreal 객체 생성에 사용하는 기본 초기화 값
	 */
	UCharacterCardWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/**
	 * @brief 카드에 표시할 캐릭터 값과 목록 인덱스를 넣음
	 *
	 * @details
	 * 카드 위젯은 FFrontendCharacterOption을 저장한 뒤 SyncText()로 화면 문구를 갱신한다.
	 * InCharacterIndex는 클릭 이벤트를 부모에게 보낼 때 사용한다.
	 *
	 * @param InOption 카드에 표시할 캐릭터 값
	 * @param InCharacterIndex CharacterSelectWidget의 캐릭터 배열 안에서 이 카드가 가리키는 번호
	 */
	void SetCharacterOption(const FFrontendCharacterOption& InOption, int32 InCharacterIndex);

	/**
	 * @brief 이 카드가 현재 선택된 카드인지 표시함
	 *
	 * @details
	 * 선택 상태가 바뀌면 아이콘 배경 색이 다시 계산된다.
	 * 텍스트 상태 문구는 더 이상 기본 UI에서 사용하지 않는다.
	 *
	 * @param bSelected 현재 선택된 카드면 true
	 */
	void SetSelected(bool bSelected);

public:
	/**
	 * @brief 카드가 눌렸을 때 부모 CharacterSelectWidget에 알려주는 이벤트
	 *
	 * @details
	 * 이 이벤트에는 캐릭터 인덱스만 담는다.
	 * 실제 선택 처리와 상세 정보 갱신은 부모 위젯이 담당한다.
	 */
	UPROPERTY(Category = "Character Card", BlueprintAssignable)
	FCharacterCardClickedDelegate OnCharacterCardClicked;

protected:
	/**
	 * @brief 위젯이 화면에 올라올 때 카드 버튼 클릭 이벤트를 연결함
	 */
	void NativeConstruct() override;

	/**
	 * @brief 위젯이 화면에서 내려갈 때 카드 버튼 클릭 이벤트를 해제함
	 */
	void NativeDestruct() override;

private:
	/**
	 * @brief 저장된 캐릭터 값과 선택 상태를 아이콘 버튼에 반영함
	 *
	 * @details
	 * 캐릭터 값이 바뀌거나 선택 상태가 바뀔 때 호출한다.
	 * 기본 UI에서는 이름, 역할, 스탯, 설명, 상태 문구를 모두 숨기고 아이콘만 보여준다.
	 * 실제 아이콘 텍스처가 이미 로드되어 있으면 IconImage에 적용한다.
	 * 아직 아이콘이 없으면 캐릭터 순서에 따른 단색 placeholder를 사용한다.
	 */
	void SyncText() const;

	/**
	 * @brief 카드에 꼭 필요한 위젯이 연결됐는지 로그로 확인함
	 *
	 * @details
	 * WBP_CharacterCard에는 CardButton, IconBackground, IconImage가 필요하다.
	 * 텍스트 위젯들은 예전 카드형 WBP가 남아 있을 때 숨기기 위한 Optional 필드라서 없어도 경고하지 않는다.
	 */
	void ValidateDesignerBindings() const;

	/**
	 * @brief CardButton 클릭 이벤트를 OnCharacterCardClicked로 바꿔서 내보냄
	 */
	UFUNCTION()
	void HandleCardButtonClicked();

private:
	/**
	 * @brief 카드 전체를 감싸는 클릭 버튼
	 *
	 * @note
	 * 잠긴 캐릭터도 눌러서 상세 잠김 정보를 확인할 수 있어야 하므로 항상 활성화한다.
	 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> CardButton;

	/**
	 * @brief 아이콘 뒤에 깔리는 단색 배경
	 *
	 * @details
	 * 실제 아이콘 에셋이 아직 없을 때도 하단 버튼이 보이도록 색 placeholder 역할을 한다.
	 * 선택 상태와 잠김 상태에 따라 색 밝기를 다르게 준다.
	 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBorder> IconBackground;

	/**
	 * @brief 캐릭터 아이콘 이미지를 표시하는 자리
	 *
	 * @details
	 * FFrontendCharacterOption::mIcon이 있고 이미 로드되어 있으면 이 Image에 표시한다.
	 * 아이콘이 없으면 숨기고 IconBackground의 단색 placeholder만 보이게 한다.
	 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> IconImage;

	/** @brief 예전 카드형 UI에 있던 캐릭터 이름. 현재 기본 UI에서는 숨긴다. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> NameText;

	/** @brief 예전 카드형 UI에 있던 역할 문구. 현재 기본 UI에서는 숨긴다. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> RoleText;

	/** @brief 예전 카드형 UI에 있던 스탯 요약. 현재 기본 UI에서는 숨긴다. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> StatText;

	/** @brief 예전 카드형 UI에 있던 캐릭터 설명. 현재 기본 UI에서는 숨긴다. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> DescriptionText;

	/** @brief 예전 카드형 UI에 있던 Selected/Ready/Locked 문구. 현재 기본 UI에서는 숨긴다. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> StateText;

private:
	/**
	 * @brief 이 카드가 가리키는 캐릭터 배열 번호
	 *
	 * @details
	 * 카드 클릭 시 CharacterSelectWidget으로 이 번호를 돌려보낸다.
	 */
	int32 mCharacterIndex = INDEX_NONE;

	/**
	 * @brief 이 카드에 표시할 캐릭터 값
	 *
	 * @details
	 * 이름, 역할, 설명, 스탯, 잠김 여부가 모두 들어 있다.
	 */
	FFrontendCharacterOption mCharacterOption;

	/**
	 * @brief 현재 선택된 카드면 true
	 *
	 * @details
	 * 선택 가능 카드에서는 이 값으로 아이콘 배경 밝기를 고른다.
	 * 잠긴 카드도 선택될 수 있으므로, 선택된 잠김 카드는 어둡지만 조금 더 밝은 색으로 표시한다.
	 */
	bool bIsSelected = false;
};
