#include "UI/Shop/ShopUIModel.h"

/** @details 구매 의도만 구독자(게임플레이)에게 중계한다. 골드 차감/지급/품절은 게임플레이가 처리한다. */
void UShopUIModel::RequestBuy(int32 SlotIndex)
{
	OnBuyRequested.Broadcast(SlotIndex);
}

/** @details 스킬 구매 의도만 중계한다. 직업 검증과 지급은 게임플레이(ShopGameMode)가 처리한다. */
void UShopUIModel::RequestBuySkill(int32 SlotIndex, int32 UnitIndex, int32 SkillSlotIndex)
{
	OnBuySkillRequested.Broadcast(SlotIndex, UnitIndex, SkillSlotIndex);
}

void UShopUIModel::RequestHireMercenary(const int32 SlotIndex, const int32 PartySlotIndex)
{
	OnHireMercenaryRequested.Broadcast(SlotIndex, PartySlotIndex);
}

/** @details 아티펙트 버리기 의도만 중계한다. 실제 제거는 게임플레이(ShopGameMode)가 처리한다. */
void UShopUIModel::RequestDiscardArtifact(int32 ArtifactIndex)
{
	OnDiscardArtifactRequested.Broadcast(ArtifactIndex);
}

/** @details 스킬 슬롯 버리기 의도만 중계한다. 실제 슬롯 비우기는 게임플레이(ShopGameMode)가 처리한다. */
void UShopUIModel::RequestDiscardSkill(int32 UnitIndex, int32 SlotIndex)
{
	OnDiscardSkillRequested.Broadcast(UnitIndex, SlotIndex);
}

/** @details 휴식 의도만 중계한다. 가격 차감과 회복은 ShopGameMode가 처리한다. */
void UShopUIModel::RequestRest()
{
	OnRestRequested.Broadcast();
}

/** @details 상점 나가기 의도만 전달한다. 실제 화면 전환은 게임플레이가 결정한다. */
void UShopUIModel::RequestLeave()
{
	OnLeaveRequested.Broadcast();
}

/** @details 상세 의도만 중계한다. DTO 조립은 게임플레이(ShopGameMode)가 하고 Set*Detail로 되민다. */
void UShopUIModel::RequestItemDetail(int32 SlotIndex)
{
	OnItemDetailRequested.Broadcast(SlotIndex);
}

/** @details 소지 스킬 상세 의도만 중계한다. 조립은 게임플레이가 한다. */
void UShopUIModel::RequestOwnedSkillDetail(int32 UnitIndex, int32 SkillSlotIndex)
{
	OnOwnedSkillDetailRequested.Broadcast(UnitIndex, SkillSlotIndex);
}

/** @details 상점 스냅샷을 저장하고 거래 도메인 변경 알림을 쏜다. 거래 후 게임플레이가 다시 호출해 골드/품절/소지품을 갱신한다. */
void UShopUIModel::SetShop(const FShopUI& Shop)
{
	mShop = Shop;
	OnUIChanged.Broadcast(EShopUIDomain::Trade);
}

/** @details 스킬 상세를 저장하고 Detail 도메인 알림을 쏜다. 빈 DTO(mName 없음)면 위젯이 플레인 폴백을 연다. */
void UShopUIModel::SetSkillDetail(const FSkillDetailUI& Detail)
{
	mSkillDetail = Detail;
	mLastDetailKind = EShopDetailKind::Skill;
	OnUIChanged.Broadcast(EShopUIDomain::Detail);
}

/** @details 아티팩트 상세를 저장하고 Detail 도메인 알림을 쏜다. */
void UShopUIModel::SetArtifactDetail(const FCombatArtifactUI& Detail)
{
	mArtifactDetail = Detail;
	mLastDetailKind = EShopDetailKind::Artifact;
	OnUIChanged.Broadcast(EShopUIDomain::Detail);
}
