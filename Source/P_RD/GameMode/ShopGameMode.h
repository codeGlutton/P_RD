/*****************************************************************//**
 * @file   ShopGameMode.h
 * @brief  상점 방에 대한 GameMode 정의 헤더
 * @author 모호재
 * @date   2026-05-19
 *********************************************************************/

#pragma once

#include "GameMode/RoomGameModeBase.h"
#include "UI/Shop/ShopUITypes.h"
#include "ShopGameMode.generated.h"

class UTileMapModel;
class UShopUIModel;
struct FShopItemList;

/**
 * @brief  상점 방에 대한 GameMode
 */
UCLASS(abstract)
class P_RD_API AShopGameMode : public ARoomGameModeBase
{
	GENERATED_BODY()

public:
	void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;

	/**
	 * @brief 상점 화면이 읽을 뷰모델.
	 *
	 * @details
	 * 보상과 같은 규칙이다 -- 게임모드가 SetShop() 으로 밀고, 위젯은 GetShop()
	 * 으로 읽고, 사고 나가는 것은 의도만 보낸다. 골드를 깎고 품절을 매기는 것은
	 * 전부 이쪽이다.
	 *
	 * 방마다 여는 화면이 아니라 이 방의 화면이라, 월드 위젯이 아니라 여기에
	 * 둔다. @return 뷰모델
	 */
	UFUNCTION(BlueprintPure, Category = "Shop")
	UShopUIModel* GetShopUIModel() const { return mShopUIModel; }

#if !UE_BUILD_SHIPPING
	/** @brief 개발 치트: 파티 골드 지급 후 구매 가능 여부가 다시 계산되게 화면 갱신 */
	void AddPartyGoldDev(int32 Amount);
#endif

protected:
	void InitializeRoom() override;
	void BeginRoom() override;

private:
	/**
	 * @brief 상점 스폰포인트 위치에 타일맵을 깐다.
	 */
	void SpawnTileMap();

	/** @brief 지금 파는 것과 가진 돈을 화면에 내린다. */
	void PushShopUIData();

	/**
	 * @brief 소지품(버리기 대상)을 표시값으로 채움
	 * @details 파티 공용 아티펙트 + 유닛별 소지 스킬. 거래마다 판매 목록과 함께 갱신됨
	 */
	void FillOwnedItems(FShopUI& ShopUIData) const;

	/** @brief 판매 스킬을 이미 가진 파티 유닛을 슬롯마다 표시한다. */
	// 판매 목록과 파티가 모두 채워진 다음에 불러야 한다.
	void MarkOwnedSaleItems(FShopUI& ShopUIData) const;

	/** @brief 한 칸을 샀다. 돈을 깎고 품절로 매긴 뒤 다시 내린다. */
	UFUNCTION() void HandleBuyRequested(int32 SlotIndex);

	/**
	 * @brief 스킬 한 칸을 샀다 (지급 대상 유닛+스킬 슬롯 지정)
	 * @details 지정 슬롯에 장착, 찬 슬롯이면 교체. SetSkill의 직업 검사가 거부하면 과금하지 않음
	 */
	UFUNCTION() void HandleBuySkillRequested(int32 SlotIndex, int32 UnitIndex, int32 SkillSlotIndex);

	/** @brief 판매 후보를 만들어 지정 파티 슬롯과 교체하고 과금한다. */
	UFUNCTION() void HandleHireMercenaryRequested(int32 SlotIndex, int32 PartySlotIndex);

	/**
	 * @brief 소지 아티펙트 하나를 버림 (0골드 거래)
	 * @details 파티 원본에서 제거 — 전 구성원 해제 배포까지 컴포넌트가 처리
	 */
	UFUNCTION() void HandleDiscardArtifactRequested(int32 ArtifactIndex);

	/**
	 * @brief 유닛의 스킬 슬롯 하나를 버림 (0골드 거래)
	 * @details 슬롯을 비움 — 저장 추적(OnChangeSkillUI)이 즉시 반영
	 */
	UFUNCTION() void HandleDiscardSkillRequested(int32 UnitIndex, int32 SlotIndex);

	/** @brief 파티 전원의 HP/AP를 완전히 회복하고 이 상점의 휴식을 소진한다. */
	UFUNCTION() void HandleRestRequested();

	/** @brief 나간다. 지도를 열어 다음 방을 고르게 한다. */
	UFUNCTION() void HandleLeaveRequested();

	/**
	 * @brief 판매 슬롯 상세 요청. 종류(mSlotSources)에 맞는 상세 DTO를 조립해 밀어준다.
	 * @details 조립 실패 시에도 빈 DTO를 밀어 위젯이 플레인 상세로 폴백하게 한다.
	 */
	UFUNCTION() void HandleItemDetailRequested(int32 SlotIndex);

	/** @brief 소지 용병 스킬 상세 요청. BuildPartyUnitSkillDetailUI 결과를 밀어준다. */
	UFUNCTION() void HandleOwnedSkillDetailRequested(int32 UnitIndex, int32 SkillSlotIndex);

	/**
	 * @brief 판매 슬롯의 스킬 상품으로 스킬 상세 DTO를 조립한다.
	 *
	 * @details
	 * 상점 상세도 전투와 같은 리치 상세창을 쓰기 위한 생산 API다. 이제 요청
	 * 핸들러(HandleItemDetailRequested) 뒤에서만 쓰이므로 외부에 열지 않는다.
	 * 아티팩트 슬롯이면 거짓을 돌려주고 화면은 기존 플레인 상세로 폴백한다.
	 * @param SlotIndex 판매 슬롯 번호 (FShopItemUI::mSlotIndex)
	 * @param[out] OutDetail 채워 줄 상세 DTO
	 * @return 스킬 상품이라 채웠으면 참
	 */
	bool BuildShopItemSkillDetailUI(int32 SlotIndex, FSkillDetailUI& OutDetail) const;

	/**
	 * @brief 판매 슬롯의 아티팩트 상품으로 아티팩트 상세 DTO를 조립한다.
	 *
	 * @details
	 * 스킬 상세(BuildShopItemSkillDetailUI)와 짝을 이루는 생산 API다.
	 * 스킬 슬롯이거나 DA 로드에 실패하면 거짓 -- 화면은 플레인 상세로 폴백한다.
	 * @param SlotIndex 판매 슬롯 번호 (FShopItemUI::mSlotIndex)
	 * @param[out] OutDetail 채워 줄 상세 DTO
	 * @return 아티팩트 상품이라 채웠으면 참
	 */
	bool BuildShopItemArtifactDetailUI(int32 SlotIndex,
		FCombatArtifactUI& OutDetail) const;

	/** @brief 지금 가진 돈. @return 없으면 0 */
	int32 GetPartyGold() const;

	/** @brief 파티 골드 소비. 0 이하 가격은 무시 */
	void SpendPartyGold(int32 Price);

private:
	// @brief 상점방 타일맵 모델 (뷰 액터는 모델 팩토리가 함께 스폰)
	UPROPERTY(Transient)
	TObjectPtr<UTileMapModel> mTileMap;

	UPROPERTY(Transient)
	TObjectPtr<UShopUIModel> mShopUIModel;

	/**
	 * @brief 이미 팔린 칸.
	 *
	 * @details
	 * 원본(FShopRoom)은 세이브되는 값이라 읽기만 하고 수정하지 않는다.
	 * 팔린 슬롯 번호만 여기에 기록하고, 화면에 내릴 때마다
	 * "원본 + 팔림 오버레이"로 스냅샷(FShopUI)을 새로 만든다.
	 * 원본이 보존되므로 구매 무르기(환불) 규칙이 나중에 생겨도 대응할 수 있다.
	 */
	TSet<int32> mSoldSlots;

	/** @brief 현재 상점방 인스턴스에서 휴식 서비스를 이미 사용했는지 여부. */
	bool mRestUsed = false;

	/**
	 * @brief 판매 슬롯 번호가 가리키는 실제 상품
	 * @details
	 * 화면은 슬롯 번호만 돌려보내므로 구매 지급뿐 아니라 온디맨드 상세
	 * (HandleItemDetailRequested)의 읽기 경로도 여기서 상품을 찾는다.
	 * PushShopUIData가 판매 목록을 내릴 때마다 다시 기록한다.
	 */
	struct FShopSlotSource
	{
		EShopItemKind mKind = EShopItemKind::Skill;
		FPrimaryAssetId mItemId;
		int32 mCandidateIndex = INDEX_NONE;
	};
	TMap<int32, FShopSlotSource> mSlotSources;
};
