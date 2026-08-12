#include "UI/Shop/ShopUIWidgetBase.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/WrapBox.h"
#include "Components/WrapBoxSlot.h"
#include "Engine/Texture2D.h"
#include "UI/Shop/ShopUIModel.h"
#include "UI/ViewportZOrderType.h"

#define LOCTEXT_NAMESPACE "ShopUIWidgetBase"

namespace
{
	/** @brief 상점 항목 종류별 기본 아이콘 텍스처 경로(SVN 임포트). */
	// 종류 구분은 거래 파이프라인(#490)의 것을 따르고, 경로는 실재하는
	// SVN 에셋으로 건다 -- 옛 InSideAsset 경로 3종은 프로젝트에서 지워져
	// 아이콘이 빈칸으로 나왔다(0807 감사).
	const TCHAR* ShopKindIconPath(EShopItemKind Kind)
	{
		switch (Kind)
		{
		case EShopItemKind::Skill:
			return TEXT("/Game/SVN/OutSideAsset/AICreation/UI/P_RD/Combat/Skills/T_SkillIcon_Whirlwind.T_SkillIcon_Whirlwind");
		case EShopItemKind::Artifact:
			return TEXT("/Game/SVN/OutSideAsset/AICreation/UI/P_RD/Items/Artifacts/T_Artifact_BloodChalice.T_Artifact_BloodChalice");
		case EShopItemKind::Heal:
			return TEXT("/Game/SVN/OutSideAsset/AICreation/UI/P_RD/Combat/Skills/T_skill_meditation_heal_icon.T_skill_meditation_heal_icon");
		case EShopItemKind::Mercenary:   // 용병 전용 그림이 아직 없다 -- 골드로
		default:
			return TEXT("/Game/SVN/OutSideAsset/AICreation/UI/P_RD/Common/Icons/T_gold_icon.T_gold_icon");
		}
	}

	UTexture2D* ResolveShopIcon(const FShopItemUI& Item)
	{
		if (Item.mIcon != nullptr)
		{
			return Item.mIcon;
		}
		return LoadObject<UTexture2D>(nullptr, ShopKindIconPath(Item.mKind));
	}

	// 카드 텍스트 폰트 크기 (UMG 기본 24pt는 카드에 과대)
	constexpr int32 CardHeaderFontSize = 18;
	constexpr int32 CardTextFontSize = 16;

	/** @brief 동적 생성 TextBlock의 폰트 크기만 바꾼다(서체는 기본 유지). */
	void SetCardFontSize(UTextBlock* Text, int32 Size)
	{
		FSlateFontInfo Font = Text->GetFont();
		Font.Size = Size;
		Text->SetFont(Font);
	}
}

UShopUIWidgetBase::UShopUIWidgetBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	mViewportZOrder = StaticCast<int32>(EViewportZOrderType::PopUp);
}

/** @brief 나가기 버튼 클릭을 연결하고, BindUIModel이 먼저 됐다면 들어온 값을 즉시 그린다. */
void UShopUIWidgetBase::NativeConstruct()
{
	Super::NativeConstruct();

	if (mCloseButton != nullptr)
	{
		mCloseButton->OnClicked.AddUniqueDynamic(this, &UShopUIWidgetBase::HandleCloseClicked);
	}
	if (mCloseButtonText != nullptr)
	{
		mCloseButtonText->SetText(LOCTEXT("Leave", "Leave"));
	}
	if (mTitleText != nullptr)
	{
		mTitleText->SetText(LOCTEXT("Shop", "Shop"));
	}

	RefreshView();
}

/** @brief 새 UIModel을 구독하고 이미 들어온 상점 스냅샷도 즉시 한 번 그린다. */
void UShopUIWidgetBase::BindUIModel(UShopUIModel* InUIModel)
{
	if (mUIModel == InUIModel)
	{
		return;
	}
	UnbindUIModel();
	mUIModel = InUIModel;
	if (mUIModel != nullptr)
	{
		mUIModel->OnUIChanged.AddDynamic(this, &UShopUIWidgetBase::HandleUIChanged);
		// 상점 데이터가 BindUIModel보다 먼저 들어온 경우도 있으므로 연결 직후 한 번 그린다.
		RefreshView();
		OnShopRefreshed();
	}
}

void UShopUIWidgetBase::HandleCloseClicked()
{
	if (mUIModel != nullptr)
	{
		mUIModel->RequestLeave();
	}
	RemoveFromParent();
}

/** @brief WBP의 슬롯 구매 입력을 UIModel의 구매 의도 이벤트로 전달한다. */
void UShopUIWidgetBase::BuyItem(int32 SlotIndex)
{
	if (mUIModel != nullptr)
	{
		mUIModel->RequestBuy(SlotIndex);
	}
}

/** @brief WBP의 나가기 입력을 UIModel의 나가기 의도 이벤트로 전달한다. */
void UShopUIWidgetBase::Leave()
{
	if (mUIModel != nullptr)
	{
		mUIModel->RequestLeave();
	}
}

/** @brief 현재 UIModel 구독을 해제해 화면 파괴 후 OnUIChanged가 들어오지 않게 한다. */
void UShopUIWidgetBase::UnbindUIModel()
{
	if (mUIModel != nullptr)
	{
		mUIModel->OnUIChanged.RemoveDynamic(this, &UShopUIWidgetBase::HandleUIChanged);
	}
	mUIModel = nullptr;
}

/** @brief UIModel 변경 알림 → 거래 도메인이면 뷰 갱신 후 WBP 구현 이벤트로도 전달. */
void UShopUIWidgetBase::HandleUIChanged(EShopUIDomain Domain)
{
	// 이 위젯은 거래 화면 — 프리뷰 등 다른 도메인 알림은 각자의 위젯이 처리
	if (Domain != EShopUIDomain::Trade)
	{
		return;
	}

	RefreshView();
	OnShopRefreshed();
}

/** @brief 현재 모델의 골드/판매 슬롯을 BindWidget 위젯(골드 라벨 + 항목 박스)에 반영한다. */
void UShopUIWidgetBase::RefreshView()
{
	if (mUIModel == nullptr)
	{
		return;
	}

	const FShopUI& Shop = mUIModel->GetShop();

	if (mGoldText != nullptr)
	{
		mGoldText->SetText(FText::Format(
			LOCTEXT("Gold {0}", "Gold {0}"),
			FText::AsNumber(Shop.mGold)));
	}

	// 소지품(파티 아티펙트 + 유닛 카드)은 판매 박스 유무와 무관하게 갱신
	RefreshOwnedView(Shop);

	// 종류별 박스가 WBP에 없으면 공용 박스(mItemBox)로 폴백 — WBP를 단계적으로 바꿔도 동작 유지
	if (mItemBox == nullptr && mSkillItemBox == nullptr && mArtifactItemBox == nullptr)
	{
		return;
	}
	if (mItemBox != nullptr)
	{
		mItemBox->ClearChildren();
	}
	if (mSkillItemBox != nullptr)
	{
		mSkillItemBox->ClearChildren();
	}
	if (mArtifactItemBox != nullptr)
	{
		mArtifactItemBox->ClearChildren();
	}

	for (int32 ItemIndex = 0; ItemIndex < Shop.mItems.Num(); ++ItemIndex)
	{
		const FShopItemUI& Item = Shop.mItems[ItemIndex];

		// 카드가 들어갈 박스 결정. 미분류 종류(용병 등 추후 추가분)도 공용 박스로
		UPanelWidget* TargetBox = nullptr;
		switch (Item.mKind)
		{
		case EShopItemKind::Skill:
			TargetBox = mSkillItemBox;
			break;
		case EShopItemKind::Artifact:
			TargetBox = mArtifactItemBox;
			break;
		default:
			break;
		}
		if (TargetBox == nullptr)
		{
			TargetBox = mItemBox;
		}
		if (TargetBox == nullptr)
		{
			continue;
		}

		// 슬롯 카드 = 세로 박스(아이콘 + 이름 + 가격). 종류별 아이콘 자동 매핑.
		UVerticalBox* Card = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		if (Card == nullptr)
		{
			continue;
		}

		// 슬롯 인덱스 라벨. BuyItem(SlotIndex)에 넣을 모델 인덱스 확인용(테스트 UI)
		if (UTextBlock* IndexLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass()))
		{
			IndexLabel->SetText(FText::Format(LOCTEXT("[{0}]", "[{0}]"), FText::AsNumber(ItemIndex)));
			IndexLabel->SetJustification(ETextJustify::Center);
			SetCardFontSize(IndexLabel, CardTextFontSize);
			if (UVerticalBoxSlot* IndexSlot = Card->AddChildToVerticalBox(IndexLabel))
			{
				IndexSlot->SetHorizontalAlignment(HAlign_Center);
			}
		}

		if (UImage* Icon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass()))
		{
			if (UTexture2D* Tex = ResolveShopIcon(Item))
			{
				Icon->SetBrushFromTexture(Tex, false);
			}
			Icon->SetDesiredSizeOverride(FVector2D(112.f, 112.f));
			// 품절/구매불가는 흐리게.
			Icon->SetColorAndOpacity((Item.mIsSoldOut || !Item.mIsAffordable)
				? FLinearColor(1.f, 1.f, 1.f, 0.35f) : FLinearColor::White);
			Card->AddChildToVerticalBox(Icon);
		}

		if (UTextBlock* Name = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass()))
		{
			Name->SetText(Item.mName);
			Name->SetJustification(ETextJustify::Center);
			SetCardFontSize(Name, CardHeaderFontSize);
			if (UVerticalBoxSlot* NameSlot = Card->AddChildToVerticalBox(Name))
			{
				NameSlot->SetPadding(FMargin(0.f, 6.f, 0.f, 0.f));
				NameSlot->SetHorizontalAlignment(HAlign_Center);
			}
		}

		if (UTextBlock* Price = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass()))
		{
			const FText PriceText = Item.mIsSoldOut
				? LOCTEXT("Sold Out", "Sold Out")
				: FText::Format(LOCTEXT("{0} G", "{0} G"), FText::AsNumber(Item.mPrice));
			Price->SetText(PriceText);
			Price->SetJustification(ETextJustify::Center);
			SetCardFontSize(Price, CardTextFontSize);
			Price->SetColorAndOpacity(FSlateColor(Item.mIsAffordable && !Item.mIsSoldOut
				? FLinearColor(0.95f, 0.85f, 0.45f, 1.f) : FLinearColor(0.8f, 0.4f, 0.4f, 1.f)));
			if (UVerticalBoxSlot* PriceSlot = Card->AddChildToVerticalBox(Price))
			{
				PriceSlot->SetPadding(FMargin(0.f, 2.f, 0.f, 0.f));
				PriceSlot->SetHorizontalAlignment(HAlign_Center);
			}
		}

		// 박스 종류(수평/줄바꿈)에 따라 슬롯 타입이 달라 캐스팅으로 여백 적용
		UPanelSlot* CardSlot = TargetBox->AddChild(Card);
		if (UHorizontalBoxSlot* HorizontalSlot = Cast<UHorizontalBoxSlot>(CardSlot))
		{
			HorizontalSlot->SetPadding(FMargin(14.f, 0.f));
		}
		else if (UWrapBoxSlot* WrapSlot = Cast<UWrapBoxSlot>(CardSlot))
		{
			WrapSlot->SetPadding(FMargin(14.f, 8.f));
		}
	}
}

/**
 * @brief 소지 아티펙트(파티 소유)와 파티 유닛 카드를 소지 박스에 반영한다.
 * @details
 * 아티펙트 카드의 [N]은 파티 배열 index — RD.ShopDiscardArtifact payload 그대로.
 * 유닛 카드는 [유닛 index] 직업 Lv 헤더 + 슬롯별 스킬 줄(S슬롯: 이름, 빈 슬롯은 '-') —
 * (유닛, 슬롯) 쌍이 스킬 구매/버리기 명령 payload 그대로.
 */
void UShopUIWidgetBase::RefreshOwnedView(const FShopUI& Shop)
{
	// 소지 아티펙트 — 아이콘 + [N] + 이름(희귀도 색)
	if (mOwnedArtifactBox != nullptr)
	{
		mOwnedArtifactBox->ClearChildren();
		for (const FShopOwnedArtifactUI& Artifact : Shop.mOwnedArtifacts)
		{
			UVerticalBox* Card = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
			if (Card == nullptr)
			{
				continue;
			}

			if (UTextBlock* IndexLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass()))
			{
				IndexLabel->SetText(FText::Format(LOCTEXT("[{0}]", "[{0}]"), FText::AsNumber(Artifact.mArtifactIndex)));
				IndexLabel->SetJustification(ETextJustify::Center);
				SetCardFontSize(IndexLabel, CardTextFontSize);
				if (UVerticalBoxSlot* IndexSlot = Card->AddChildToVerticalBox(IndexLabel))
				{
					IndexSlot->SetHorizontalAlignment(HAlign_Center);
				}
			}

			if (UImage* Icon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass()))
			{
				if (Artifact.mIcon != nullptr)
				{
					Icon->SetBrushFromTexture(Artifact.mIcon, false);
				}
				// 판매 카드(112px)와 구분되게 소지품은 작게
				Icon->SetDesiredSizeOverride(FVector2D(64.f, 64.f));
				Card->AddChildToVerticalBox(Icon);
			}

			if (UTextBlock* Name = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass()))
			{
				Name->SetText(Artifact.mName);
				Name->SetJustification(ETextJustify::Center);
				Name->SetColorAndOpacity(FSlateColor(Artifact.mRarityColor));
				SetCardFontSize(Name, CardTextFontSize);
				if (UVerticalBoxSlot* NameSlot = Card->AddChildToVerticalBox(Name))
				{
					NameSlot->SetPadding(FMargin(0.f, 4.f, 0.f, 0.f));
					NameSlot->SetHorizontalAlignment(HAlign_Center);
				}
			}

			if (UWrapBoxSlot* CardSlot = mOwnedArtifactBox->AddChildToWrapBox(Card))
			{
				CardSlot->SetPadding(FMargin(10.f, 6.f));
			}
		}
	}

	// 파티 유닛 카드 — 헤더 + 슬롯별 스킬 줄
	if (mOwnedUnitBox != nullptr)
	{
		mOwnedUnitBox->ClearChildren();
		for (const FShopOwnedUnitUI& Unit : Shop.mOwnedUnits)
		{
			UVerticalBox* Card = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
			if (Card == nullptr)
			{
				continue;
			}

			if (UTextBlock* Header = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass()))
			{
				Header->SetText(FText::Format(
					LOCTEXT("[{0}] {1} Lv.{2}", "[{0}] {1} Lv.{2}"),
					FText::AsNumber(Unit.mUnitIndex),
					UEnum::GetDisplayValueAsText(Unit.mJobType),
					FText::AsNumber(Unit.mLevel)));
				SetCardFontSize(Header, CardHeaderFontSize);
				Card->AddChildToVerticalBox(Header);
			}

			for (int32 SlotIndex = 0; SlotIndex < Unit.mSkillSlots.Num(); ++SlotIndex)
			{
				const FShopOwnedSkillSlotUI& SkillSlot = Unit.mSkillSlots[SlotIndex];
				if (UTextBlock* SlotText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass()))
				{
					SlotText->SetText(FText::Format(
						LOCTEXT("S{0}: {1}", "S{0}: {1}"),
						FText::AsNumber(SlotIndex),
						SkillSlot.mIsEmpty ? LOCTEXT("-", "-") : SkillSlot.mName));
					// 빈 슬롯은 흐리게 — 구매로 채울 자리
					SlotText->SetColorAndOpacity(FSlateColor(SkillSlot.mIsEmpty
						? FLinearColor(1.f, 1.f, 1.f, 0.35f) : FLinearColor::White));
					SetCardFontSize(SlotText, CardTextFontSize);
					if (UVerticalBoxSlot* TextSlot = Card->AddChildToVerticalBox(SlotText))
					{
						TextSlot->SetPadding(FMargin(12.f, 2.f, 0.f, 0.f));
					}
				}
			}

			if (UWrapBoxSlot* CardSlot = mOwnedUnitBox->AddChildToWrapBox(Card))
			{
				CardSlot->SetPadding(FMargin(10.f, 6.f));
			}
		}
	}
}

/** @brief 위젯 생명주기 종료 시 UIModel 델리게이트를 먼저 끊고 부모 정리를 따른다. */
void UShopUIWidgetBase::NativeDestruct()
{
	UnbindUIModel();
	Super::NativeDestruct();
}

#undef LOCTEXT_NAMESPACE
