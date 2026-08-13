#include "UI/Combat/CombatLayoutHUDWidget.h"

#include "Blueprint/UserWidget.h"
#include "Engine/Texture2D.h"
#include "TimerManager.h"
#include "UI/Combat/CombatUIModel.h"
#include "UI/Combat/SkillCutInWidget.h"

namespace
{
	constexpr int32 SkillCutInZOrder = 800;
	constexpr float HudSafetyMarginSeconds = 0.25f;

	TSoftObjectPtr<UTexture2D> MakeSkillCutInTexture(const TCHAR* ObjectPath)
	{
		return TSoftObjectPtr<UTexture2D>(FSoftObjectPath(ObjectPath));
	}

	TSoftObjectPtr<UTexture2D> ResolveRosterCasterPlate(
		const FCombatSkillCutInRequest& Request)
	{
		struct FCasterPlateRule
		{
			const TCHAR* IdentityNeedle;
			const TCHAR* ObjectPath;
		};
		static const FCasterPlateRule PlayerRules[] = {
			{ TEXT("Barbarian"), TEXT("/Game/SVN/OutSideAsset/AICreation/UI/P_RD/SkillCutIn/Generated/T_SkillCutIn_Roster_Mercenary_Barbarian_v1.T_SkillCutIn_Roster_Mercenary_Barbarian_v1") },
			{ TEXT("Druid"), TEXT("/Game/SVN/OutSideAsset/AICreation/UI/P_RD/SkillCutIn/Generated/T_SkillCutIn_Roster_Mercenary_Druid_v1.T_SkillCutIn_Roster_Mercenary_Druid_v1") },
			{ TEXT("Mage"), TEXT("/Game/SVN/OutSideAsset/AICreation/UI/P_RD/SkillCutIn/Generated/T_SkillCutIn_Roster_Mercenary_Mage_v1.T_SkillCutIn_Roster_Mercenary_Mage_v1") },
			{ TEXT("Ranger"), TEXT("/Game/SVN/OutSideAsset/AICreation/UI/P_RD/SkillCutIn/Generated/T_SkillCutIn_Roster_Mercenary_Ranger_v1.T_SkillCutIn_Roster_Mercenary_Ranger_v1") },
			{ TEXT("Rogue"), TEXT("/Game/SVN/OutSideAsset/AICreation/UI/P_RD/SkillCutIn/Generated/T_SkillCutIn_Roster_Mercenary_Rogue_v1.T_SkillCutIn_Roster_Mercenary_Rogue_v1") },
		};
		static const FCasterPlateRule MonsterRules[] = {
			{ TEXT("Eagle"), TEXT("/Game/SVN/OutSideAsset/AICreation/UI/P_RD/SkillCutIn/Generated/T_SkillCutIn_Roster_Monster_Eagle_v1.T_SkillCutIn_Roster_Monster_Eagle_v1") },
			{ TEXT("Golem"), TEXT("/Game/SVN/OutSideAsset/AICreation/UI/P_RD/SkillCutIn/Generated/T_SkillCutIn_Roster_Monster_Golem_v1.T_SkillCutIn_Roster_Monster_Golem_v1") },
			{ TEXT("Leshy"), TEXT("/Game/SVN/OutSideAsset/AICreation/UI/P_RD/SkillCutIn/Generated/T_SkillCutIn_Roster_Monster_Leshy_v1.T_SkillCutIn_Roster_Monster_Leshy_v1") },
			{ TEXT("Mushroom"), TEXT("/Game/SVN/OutSideAsset/AICreation/UI/P_RD/SkillCutIn/Generated/T_SkillCutIn_Roster_Monster_Mushroom_v1.T_SkillCutIn_Roster_Monster_Mushroom_v1") },
			{ TEXT("Spider"), TEXT("/Game/SVN/OutSideAsset/AICreation/UI/P_RD/SkillCutIn/Generated/T_SkillCutIn_Roster_Monster_Spider_v1.T_SkillCutIn_Roster_Monster_Spider_v1") },
		};

		const FCasterPlateRule* Rules = Request.bIsPlayerCaster ? PlayerRules : MonsterRules;
		const int32 RuleCount = Request.bIsPlayerCaster
			? UE_ARRAY_COUNT(PlayerRules) : UE_ARRAY_COUNT(MonsterRules);
		for (int32 RuleIndex = 0; RuleIndex < RuleCount; ++RuleIndex)
		{
			if (Request.CasterIdentity.Contains(
				Rules[RuleIndex].IdentityNeedle, ESearchCase::IgnoreCase))
			{
				return MakeSkillCutInTexture(Rules[RuleIndex].ObjectPath);
			}
		}

		// Knight and Slime are the already approved faction defaults.
		return Request.bIsPlayerCaster
			? MakeSkillCutInTexture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/P_RD/SkillCutIn/Generated/T_SkillCutIn_MasterDuelSingle_Knight_1672x941_v1.T_SkillCutIn_MasterDuelSingle_Knight_1672x941_v1"))
			: MakeSkillCutInTexture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/P_RD/SkillCutIn/Generated/T_SkillCutIn_MasterDuelMonster_Character_v2.T_SkillCutIn_MasterDuelMonster_Character_v2"));
	}

	FSkillCutInPresentationData MakeSkillCutInPresentation(
		const FCombatSkillCutInRequest& Request)
	{
		FSkillCutInPresentationData Presentation;

		if (Request.bIsPlayerCaster)
		{
			// V1 motion language, now with role-specific generated effects. The fixed
			// background, moving speed layer, caster stack and fixed impact are kept
			// in separate widget transforms so they cannot drift as one plate.
			Presentation.BackgroundTexture = MakeSkillCutInTexture(
				TEXT("/Game/SVN/OutSideAsset/AICreation/UI/P_RD/SkillCutIn/Generated/T_SkillCutIn_MasterDuelSingle_FixedBG_v3.T_SkillCutIn_MasterDuelSingle_FixedBG_v3"));
			Presentation.SpeedLinesTexture = MakeSkillCutInTexture(
				TEXT("/Game/SVN/OutSideAsset/AICreation/UI/P_RD/SkillCutIn/Generated/T_SkillCutIn_MasterDuelSingle_SpeedFX_v3.T_SkillCutIn_MasterDuelSingle_SpeedFX_v3"));
			Presentation.BodyTexture = ResolveRosterCasterPlate(Request);
			Presentation.ForegroundTexture = MakeSkillCutInTexture(
				TEXT("/Game/SVN/OutSideAsset/AICreation/UI/P_RD/SkillCutIn/Generated/T_SkillCutIn_MasterDuelSingle_ImpactFX_v3.T_SkillCutIn_MasterDuelSingle_ImpactFX_v3"));
			Presentation.AccentColor = FLinearColor(0.06f, 0.32f, 0.92f, 1.0f);
			Presentation.LayerRig = ESkillCutInLayerRig::MasterDuelSingle;
			Presentation.DurationSeconds = 0.82f;
			Presentation.FailSafeSeconds = 1.12f;
			Presentation.bMirror = true;
		}
		else
		{
			// Enemy uses the same single-plate motion grammar as the mercenary,
			// mirrored onto the right half. Background and effects remain on fixed
			// canvases while the generated caster plate moves independently.
			Presentation.BackgroundTexture = MakeSkillCutInTexture(
				TEXT("/Game/SVN/OutSideAsset/AICreation/UI/P_RD/SkillCutIn/Generated/T_SkillCutIn_MasterDuelMonster_FixedBG_v1.T_SkillCutIn_MasterDuelMonster_FixedBG_v1"));
			Presentation.SpeedLinesTexture = MakeSkillCutInTexture(
				TEXT("/Game/SVN/OutSideAsset/AICreation/UI/P_RD/SkillCutIn/Generated/T_SkillCutIn_MasterDuelMonster_SpeedFX_v1.T_SkillCutIn_MasterDuelMonster_SpeedFX_v1"));
			Presentation.BodyTexture = ResolveRosterCasterPlate(Request);
			Presentation.ForegroundTexture = MakeSkillCutInTexture(
				TEXT("/Game/SVN/OutSideAsset/AICreation/UI/P_RD/SkillCutIn/Generated/T_SkillCutIn_MasterDuelMonster_ImpactFX_v1.T_SkillCutIn_MasterDuelMonster_ImpactFX_v1"));
			Presentation.AccentColor = FLinearColor(0.78f, 0.05f, 0.035f, 1.0f);
			Presentation.LayerRig = ESkillCutInLayerRig::MasterDuelSingle;
			Presentation.DurationSeconds = 0.82f;
			Presentation.FailSafeSeconds = 1.12f;
			Presentation.bMirror = false;
		}

		return Presentation;
	}
}

bool UCombatLayoutHUDWidget::EnsureSkillCutInWidget()
{
	if (IsValid(mSkillCutInWidget) == false)
	{
		if (APlayerController* OwningPlayer = GetOwningPlayer())
		{
			mSkillCutInWidget = CreateWidget<USkillCutInWidget>(OwningPlayer);
		}
		else if (UWorld* World = GetWorld())
		{
			mSkillCutInWidget = CreateWidget<USkillCutInWidget>(World);
		}
	}

	if (IsValid(mSkillCutInWidget) == false)
	{
		return false;
	}

	if (mSkillCutInWidget->IsInViewport() == false)
	{
		mSkillCutInWidget->AddToViewport(SkillCutInZOrder);
		mSkillCutInWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
	return true;
}

void UCombatLayoutHUDWidget::HandlePrePlaySkillCutIn(
	const FCombatSkillCutInRequest& Request,
	TSharedPtr<FPresentationBarrier> Barrier)
{
	// 앞 컷인이 배리어를 붙잡은 동안에는 정상 흐름에서 다음 요청이
	// 오지 않지만, 별도 캐스터나 중복 브로드캐스트가 겹쳐도 두 번째
	// 스킬이 컷인 뒤에서 먼저 실행되면 안 된다. 새 배리어도 현재 컷인의
	// 종료 시점까지 함께 보관해 모든 본 실행을 같은 경계 뒤로 직렬화한다.
	if (mSkillCutInPlaying || mSkillCutInBarrier.IsValid())
	{
		UE_LOG(LogRD, Warning,
			TEXT("Skill cut-in request coalesced while another cut-in is playing (UnitId=%d, SkillIndex=%d)."),
			Request.UnitId,
			Request.SkillIndex);
		if (Barrier.IsValid())
		{
			mOverlappingSkillCutInBarriers.Add(MoveTemp(Barrier));
		}
		return;
	}

	mSkillCutInBarrier = MoveTemp(Barrier);
	if (EnsureSkillCutInWidget() == false)
	{
		UE_LOG(LogRD, Warning, TEXT("Skill cut-in widget could not be created; continuing skill immediately."));
		FinishSkillCutIn();
		return;
	}

	const FSkillCutInPresentationData Presentation = MakeSkillCutInPresentation(Request);
	mSkillCutInPlaying = true;
	const bool bStarted = mSkillCutInWidget->PlayCutIn(
		Presentation,
		FOnSkillCutInFinished::CreateUObject(this, &UCombatLayoutHUDWidget::FinishSkillCutIn));
	if (bStarted == false)
	{
		UE_LOG(LogRD, Warning, TEXT("Skill cut-in could not start; continuing skill immediately."));
		FinishSkillCutIn();
		return;
	}

	// PlayCutIn 이 동기적으로 완료를 통지하는 커스텀 위젯이어도 종료된
	// 상태에 다시 타이머를 걸지 않는다.
	if (mSkillCutInPlaying == false)
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		const float SafetyDelay = FMath::Max(
			Presentation.FailSafeSeconds + HudSafetyMarginSeconds,
			Presentation.DurationSeconds + HudSafetyMarginSeconds * 2.0f);
		World->GetTimerManager().SetTimer(
			mSkillCutInSafetyTimerHandle,
			FTimerDelegate::CreateUObject(this, &UCombatLayoutHUDWidget::FinishSkillCutIn),
			SafetyDelay,
			false);
		return;
	}

	// 타이머를 소유할 월드가 없으면 완료 보장을 할 수 없으므로 fail-open.
	FinishSkillCutIn();
}

void UCombatLayoutHUDWidget::FinishSkillCutIn()
{
	if (mSkillCutInPlaying == false && mSkillCutInBarrier.IsValid() == false)
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(mSkillCutInSafetyTimerHandle);
	}

	mSkillCutInPlaying = false;
	if (IsValid(mSkillCutInWidget))
	{
		mSkillCutInWidget->StopCutIn(/*bNotifyCompletion=*/false);
		mSkillCutInWidget->SetVisibility(ESlateVisibility::Collapsed);
	}

	// Reset 순간 스킬 본 실행이 동기적으로 재개될 수 있다. HUD 상태와
	// 위젯을 먼저 정리하고, 배리어 해제를 반드시 마지막 작업으로 둔다.
	TSharedPtr<FPresentationBarrier> PrimaryBarrier = MoveTemp(mSkillCutInBarrier);
	TArray<TSharedPtr<FPresentationBarrier>> OverlappingBarriers = MoveTemp(mOverlappingSkillCutInBarriers);
	mOverlappingSkillCutInBarriers.Reset();
	PrimaryBarrier.Reset();
	for (TSharedPtr<FPresentationBarrier>& OverlappingBarrier : OverlappingBarriers)
	{
		OverlappingBarrier.Reset();
	}
}
