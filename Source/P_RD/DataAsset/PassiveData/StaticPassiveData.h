/*****************************************************************//**
 * @file   UStaticPassiveData.h
 * @brief  패시브 정적 데이터
 * @author 김준형, 이문환
 * @date   2026-06-18
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "DataAsset/PrimaryAssetType.h"
#include "SRPGFramework/SRPGFrameworkType.h"
#include "TAS/Passive/PassiveCondition.h"
#include "StaticPassiveData.generated.h"

class UTacticalPassive;
class UTacticalEffect;

/**
 * @brief 수량 조건 - 발동 조건을 통과한 타겟이 얼마나 있어야 하는가
 */
UENUM(BlueprintType)
enum class EPassiveTargetQuantifier : uint8
{
	Any		UMETA(DisplayName = "Any"),		// 타겟이 하나라도 있으면 발동
	All		UMETA(DisplayName = "All"),		// 모든 타겟이 자격을 갖춰야 발동
};

/**
 * @brief 효과 적용 대상
 */
UENUM(BlueprintType)
enum class EPassiveEffectTarget : uint8
{
	Self	UMETA(DisplayName = "자신"),	// 패시브 소유자에게만 적용
	Targets	UMETA(DisplayName = "대상"),	// Ctx.mTargets 전부에 적용
};

/**
 * @brief 캡처 엔트리 (캡처 타이밍에 저장할 값 하나)
 *
 * @details
 * 캡처 타이밍 도달 시 피연산자를 평가해 키별로 저장.
 * 발동 조건의 Captured 피연산자가 같은 키로 읽음.
 * 피연산자 종류는 Attribute / TagCount만 허용.
 */
USTRUCT(BlueprintType)
struct P_RD_API FPassiveCaptureEntry
{
	GENERATED_BODY()

	// 저장 키 (Captured 피연산자의 캡처 키와 짝)
	UPROPERTY(EditAnywhere, meta = (DisplayName = "키"))
	FName mKey;

	// 저장할 값 정의
	UPROPERTY(EditAnywhere, meta = (DisplayName = "피연산자"))
	FPassiveOperand mOperand;
};

/**
 * @brief 효과 엔트리 (적용할 이펙트 하나)
 */
USTRUCT(BlueprintType)
struct P_RD_API FPassiveEffectEntry
{
	GENERATED_BODY()

	// 이펙트 종류 클래스 (속성, 연산, 지속정책은 이 클래스가 정의)
	UPROPERTY(EditAnywhere, meta = (DisplayName = "이펙트 클래스", AssetBundles = "Actor"))
	TSoftClassPtr<UTacticalEffect> mEffectClass;

	// 적용 수치 (피연산자로 계산. 고정값이면 Const)
	UPROPERTY(EditAnywhere, meta = (DisplayName = "수치"))
	FPassiveOperand mMagnitude;
};

/**
 *
 */
UCLASS()
class P_RD_API UStaticPassiveData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UStaticPassiveData();

	FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(SkillPrimaryAssetTypes::GetPassiveType(), GetFName());
	}

public:
	UPROPERTY(Category = "UI", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Description", MultiLine = true))
	FText mDescription;

	UPROPERTY(Category = "UI", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Icon", AssetBundles = "UI"))
	TSoftObjectPtr<UTexture2D> mIcon;

public:
	/**
	* @brief 패시브 발동 시점 (단일)
	*
	* @details
	* 이펙트를 적용(발동)할 시점(예: 공격 시작, 타격 전 등).
	* 주기형 버프는 여기에 시작 시점을, mDeactivateTimingTag에 끝 시점을 둔다.
	*/
	UPROPERTY(Category = "Passive", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Activate Timing"))
	FGameplayTag mActivateTimingTag;

	/**
	* @brief 패시브 해제 시점 (단일)
	*
	* @details
	* 적용 중인 이펙트를 제거(해제)할 시점(예: 공격 끝).
	*/
	UPROPERTY(Category = "Passive", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Deactivate Timing"))
	FGameplayTag mDeactivateTimingTag;

	/**
	* @brief 카운터 리셋 시점 (단일)
	*
	* @details
	* 도달 시 내부 카운터를 0으로 초기화. 비우면 리셋 없음.
	*/
	UPROPERTY(Category = "Passive", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Counter Reset Timing"))
	FGameplayTag mCounterResetTimingTag;

	/**
	* @brief 캡처 시점 (단일)
	*
	* @details
	* 도달 시 mCaptureOperands를 평가해 값을 저장. 비우면 캡처 없음.
	*/
	UPROPERTY(Category = "Passive", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Capture Timing"))
	FGameplayTag mCaptureTimingTag;

	/**
	* @brief 캡처 목록
	*
	* @details
	* 캡처 시점에 저장할 값 정의. 발동 조건의 Captured 피연산자가 키로 읽음.
	*/
	UPROPERTY(Category = "Passive", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Capture Operands"))
	TArray<FPassiveCaptureEntry> mCaptureOperands;

	/**
	* @brief 발동 조건 (AND)
	*
	* @details
	* 전부 통과해야 발동. 빈 배열 = 무조건 발동.
	*/
	UPROPERTY(Category = "Passive", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Conditions"))
	TArray<FPassiveCondition> mConditions;

	/**
	* @brief 수량 조건
	*
	* @details
	* 발동 조건을 통과한 타겟이 얼마나 있어야 발동하는지 여부.
	* Any=하나라도, All=전부.
	*/
	UPROPERTY(Category = "Passive", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Target Quantifier"))
	EPassiveTargetQuantifier mTargetQuantifier = EPassiveTargetQuantifier::Any;

	/**
	* @brief 효과 목록
	*
	* @details
	* 발동 시 순서대로 전부 적용. 수치는 피연산자로 계산.
	*/
	UPROPERTY(Category = "Passive", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Effects"))
	TArray<FPassiveEffectEntry> mEffects;

	/**
	* @brief 효과 적용 대상
	*/
	UPROPERTY(Category = "Passive", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Effect Target"))
	EPassiveEffectTarget mEffectTarget = EPassiveEffectTarget::Self;

	/**
	* @brief 패시브 클래스
	*
	* @details
	* 고정형은 제네릭(UTacticalPassive_AddStat), 계산형은 Nth 등.
	* 컴포넌트가 이 클래스로 런타임 패시브를 생성하고 본 데이터를 주입한다.
	*/
	UPROPERTY(Category = "Passive", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "PassiveClass", AssetBundles = "Actor"))
	TSoftClassPtr<UTacticalPassive> mPassiveClass;

	/**
	* @brief 적용할 이펙트 "종류" 클래스
	*
	* @details
	* 속성·연산(op)·지속정책은 이 이펙트 클래스가 정의. 양(magnitude)은 mMagnitude로 공급.
	*/
	UPROPERTY(Category = "Passive", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "EffectClass", AssetBundles = "Actor"))
	TSoftClassPtr<UTacticalEffect> mEffectClass;

	/**
	* @brief 적용 수치(양)
	*
	* @details
	* '+5' vs '+10'은 서로 다른 DA. 등급/레벨 스케일이 필요해지면 커브로 확장.
	*/
	UPROPERTY(Category = "Passive", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Magnitude"))
	float mMagnitude = 0.f;

	/**
	* @brief 계산형 패시브의 파라미터(예: 매 N회 발동). 고정형은 미사용.
	*/
	UPROPERTY(Category = "Passive", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Threshold"))
	int32 mThreshold = 0;
};
