// @file ShopUIModel.h
// @brief 상점(런 중 상점방) 화면 UI와 게임플레이를 잇는 경계(뷰모델)입니다.
// @date 2026-06-18

#pragma once

#include "RDMinimal.h"
#include "UI/Combat/CombatUITypes.h"   // FSkillDetailUI / FCombatArtifactUI (온디맨드 상세)
#include "UI/Shop/ShopUITypes.h"
#include "ShopUIModel.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnShopUIChanged, EShopUIDomain, Domain);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnShopBuyRequested, int32, SlotIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnShopBuySkillRequested, int32, SlotIndex, int32, UnitIndex, int32, SkillSlotIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnShopHireMercenaryRequested, int32, SlotIndex, int32, PartySlotIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnShopDiscardArtifactRequested, int32, ArtifactIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnShopDiscardSkillRequested, int32, UnitIndex, int32, SlotIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnShopRestRequested);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnShopLeaveRequested);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnShopItemDetailRequested, int32, SlotIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnShopOwnedSkillDetailRequested, int32, UnitIndex, int32, SkillSlotIndex);

/** @brief 마지막으로 밀린 온디맨드 상세의 종류. 위젯이 Detail 도메인 알림에서 분기한다. */
UENUM(BlueprintType)
enum class EShopDetailKind : uint8
{
	None,
	Skill,
	Artifact
};

/** @brief 상점 화면 뷰모델. 상점방 진입 시 게임플레이가 하나 만들어 위젯에 물린다. */
// RewardUIModel과 같은 계약: 게임플레이가 SetShop()으로 밀어넣고, 위젯은 GetShop()으로 읽는다.
// 구매/나가기는 의도만 보낸다 — 골드 차감/지급/품절 처리는 게임플레이(ShopGameMode)가 하고 SetShop으로 갱신.
UCLASS(BlueprintType)
class P_RD_API UShopUIModel : public UObject
{
	GENERATED_BODY()

	/* ───────── 위젯이 구독하는 알림 ───────── */
public:
	/**
	 * @brief 상점 표시값이 설정/갱신됐음을 알림 (어느 도메인이 갱신됐는지 함께 전달)
	 * @details 위젯은 자기 도메인 알림만 받아 해당 부분을 다시 그림
	 */
	UPROPERTY(BlueprintAssignable, Category = "Shop|UI")
	FOnShopUIChanged OnUIChanged;

	/* ───────── 게임플레이가 구독하는 입력(의도) ───────── */
public:
	/** @brief 위젯이 구매를 확정했음(SlotIndex). 게임플레이가 골드 차감/지급한다. */
	UPROPERTY(BlueprintAssignable, Category = "Shop|Input")
	FOnShopBuyRequested OnBuyRequested;

	/**
	 * @brief 위젯이 스킬 구매를 확정했음 (슬롯 index + 지급 대상 유닛 index)
	 * @details 스킬은 유닛별 소유라 버리기와 마찬가지로 대상 유닛 지정이 필요
	 */
	UPROPERTY(BlueprintAssignable, Category = "Shop|Input")
	FOnShopBuySkillRequested OnBuySkillRequested;

	/** @brief 용병 후보를 고용해 지정 파티 슬롯과 교체한다. */
	UPROPERTY(BlueprintAssignable, Category = "Shop|Input")
	FOnShopHireMercenaryRequested OnHireMercenaryRequested;

	/**
	 * @brief 위젯이 소지 아티펙트 하나를 버리려 함 (파티 공용 배열 index)
	 * @details 실제 제거는 게임플레이가 처리 — 버리기는 0골드 거래로 구매와 같은 파이프를 탐
	 */
	UPROPERTY(BlueprintAssignable, Category = "Shop|Input")
	FOnShopDiscardArtifactRequested OnDiscardArtifactRequested;

	/**
	 * @brief 위젯이 유닛의 스킬 슬롯 하나를 버리려 함 (유닛 index + 슬롯 index)
	 * @details 스킬은 유닛별 소유라 대상 지정에 두 index가 필요
	 */
	UPROPERTY(BlueprintAssignable, Category = "Shop|Input")
	FOnShopDiscardSkillRequested OnDiscardSkillRequested;

	/** @brief 위젯이 파티 전체 휴식을 요청함. 가격/1회 제한/회복은 게임플레이가 검증한다. */
	UPROPERTY(BlueprintAssignable, Category = "Shop|Input")
	FOnShopRestRequested OnRestRequested;

	/** @brief 위젯이 상점을 나가려 함(다음 화면으로). */
	UPROPERTY(BlueprintAssignable, Category = "Shop|Input")
	FOnShopLeaveRequested OnLeaveRequested;

	/**
	 * @brief 위젯이 판매 슬롯(스킬/아티팩트)의 리치 상세를 청함
	 * @details 게임플레이가 DTO를 조립해 SetSkillDetail/SetArtifactDetail로 되민다.
	 */
	UPROPERTY(BlueprintAssignable, Category = "Shop|Input")
	FOnShopItemDetailRequested OnItemDetailRequested;

	/** @brief 위젯이 소지 용병 스킬(유닛 index + 슬롯 index)의 리치 상세를 청함 */
	UPROPERTY(BlueprintAssignable, Category = "Shop|Input")
	FOnShopOwnedSkillDetailRequested OnOwnedSkillDetailRequested;

	/* ───────── UI → gameplay : 의도만 보낸다 ───────── */
public:
	/** @brief SlotIndex 구매 의도를 게임플레이 구독자에게 전달한다(구매 확인은 화면/팝업이 먼저 처리). */
	UFUNCTION(BlueprintCallable, Category = "Shop|Input") void RequestBuy(int32 SlotIndex);

	/** @brief 스킬 구매 의도를 전달 (지급 대상 유닛+스킬 슬롯 지정) */
	UFUNCTION(BlueprintCallable, Category = "Shop|Input") void RequestBuySkill(int32 SlotIndex, int32 UnitIndex, int32 SkillSlotIndex);

	/** @brief 용병 후보 판매 슬롯과 교체할 파티 슬롯을 게임플레이에 전달한다. */
	UFUNCTION(BlueprintCallable, Category = "Shop|Input") void RequestHireMercenary(int32 SlotIndex, int32 PartySlotIndex);

	/** @brief 소지 아티펙트 버리기 의도를 전달 (버리기 확인은 화면/팝업이 먼저 처리) */
	UFUNCTION(BlueprintCallable, Category = "Shop|Input") void RequestDiscardArtifact(int32 ArtifactIndex);

	/** @brief 유닛 스킬 슬롯 버리기 의도를 전달 (버리기 확인은 화면/팝업이 먼저 처리) */
	UFUNCTION(BlueprintCallable, Category = "Shop|Input") void RequestDiscardSkill(int32 UnitIndex, int32 SlotIndex);

	/** @brief 파티 전체 휴식 의도만 전달한다. */
	UFUNCTION(BlueprintCallable, Category = "Shop|Input") void RequestRest();

	/** @brief 상점 나가기 의도를 전달한다. */
	UFUNCTION(BlueprintCallable, Category = "Shop|Input") void RequestLeave();

	/**
	 * @brief 판매 슬롯(스킬/아티팩트 공용)의 상세 의도만 전달한다.
	 * @details RequestBuySkill과 같은 계약 — 응답은 Set*Detail 푸시로 돌아온다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Shop|Input") void RequestItemDetail(int32 SlotIndex);

	/** @brief 소지 용병 스킬 슬롯의 상세 의도만 전달한다. */
	UFUNCTION(BlueprintCallable, Category = "Shop|Input") void RequestOwnedSkillDetail(int32 UnitIndex, int32 SkillSlotIndex);

	/* ───────── gameplay → UI : 표시값을 밀어넣는다 ───────── */
public:
	/** @brief 게임플레이/어댑터가 확정한 상점 표시 스냅샷을 저장하고 변경 알림을 보낸다. */
	UFUNCTION(BlueprintCallable, Category = "Shop|Push") void SetShop(const FShopUI& Shop);

	/**
	 * @brief 게임플레이가 조립한 스킬 상세를 저장하고 Detail 도메인 알림을 보낸다.
	 * @details 조립 실패 시에도 빈 DTO를 밀어 위젯이 플레인 폴백을 열 수 있게 한다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Shop|Push") void SetSkillDetail(const FSkillDetailUI& Detail);

	/** @brief 게임플레이가 조립한 아티팩트 상세를 저장하고 Detail 도메인 알림을 보낸다. */
	UFUNCTION(BlueprintCallable, Category = "Shop|Push") void SetArtifactDetail(const FCombatArtifactUI& Detail);

	/* ───────── 위젯이 읽는다 ───────── */
public:
	/** @brief 위젯이 현재 상점 스냅샷을 읽는다; 반환 참조는 다음 SetShop()까지 유효하다. */
	UFUNCTION(BlueprintPure, Category = "Shop|Read") const FShopUI& GetShop() const { return mShop; }

	/** @brief 마지막으로 밀린 스킬 상세. 다음 SetSkillDetail()까지 유효하다. */
	UFUNCTION(BlueprintPure, Category = "Shop|Read") const FSkillDetailUI& GetSkillDetail() const { return mSkillDetail; }

	/** @brief 마지막으로 밀린 아티팩트 상세. 다음 SetArtifactDetail()까지 유효하다. */
	UFUNCTION(BlueprintPure, Category = "Shop|Read") const FCombatArtifactUI& GetArtifactDetail() const { return mArtifactDetail; }

	/** @brief 마지막 Detail 알림이 스킬/아티팩트 중 무엇이었는지. */
	UFUNCTION(BlueprintPure, Category = "Shop|Read") EShopDetailKind GetLastDetailKind() const { return mLastDetailKind; }

private:
	/** @brief 마지막으로 Push된 상점 표시값; Transient라 세이브/에셋 상태로 남기지 않는다. */
	UPROPERTY(Transient) FShopUI mShop;

	/** @brief 마지막으로 밀린 온디맨드 상세. 목록(Trade)과 갱신 시점이 달라 따로 둔다. */
	UPROPERTY(Transient) FSkillDetailUI mSkillDetail;
	UPROPERTY(Transient) FCombatArtifactUI mArtifactDetail;
	UPROPERTY(Transient) EShopDetailKind mLastDetailKind = EShopDetailKind::None;
};
