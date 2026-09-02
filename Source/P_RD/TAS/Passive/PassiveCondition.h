/*****************************************************************//**
 * @file   PassiveCondition.h
 * @brief  패시브 조건 평가기 (피연산자 / 조건 / 판정 유틸)
 * @author 이문환
 * @date   2026-08-30
 *
 * @details
 * 패시브 발동 조건을 데이터로 기술하기 위한 타입.
 * 조건 하나는 "좌변 Op 우변" 식이며, 좌/우변은 FPassiveOperand(피연산자)로 표현.
 * 피연산자는 종류(Kind) / 출처(Source) / 배수로 구성되고, 조건 식과 효과 수치(Magnitude)에 공용.
 * 조건 배열은 AND로 판정(빈 배열 = 통과). OR은 미지원.
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "TAS/AttributeSet/TacticalAttributeSet.h"
#include "PassiveCondition.generated.h"

struct FPassiveActivateContext;
struct FDynamicPassiveData_Generic;
struct FTileIndex;
class UBoardCombatTargetSnapshotData;

/**
 * @brief 피연산자 종류
 *
 * @details
 * 값을 어디서 어떻게 구하는지 결정. Source(출처)와 조합해 "누구의 무엇"을 표현.
 */
UENUM()
enum class EPassiveOperandKind : uint8
{
	Const           UMETA(DisplayName = "고정값"),         // mConst 그대로
	Attribute       UMETA(DisplayName = "속성값"),         // Source 스냅샷의 mAttribute 현재값
	TagCount        UMETA(DisplayName = "태그 개수"),      // Source 스냅샷에서 mTag(하위 태그 포함) 개수 합
	Counter         UMETA(DisplayName = "카운터"),         // 런타임 상태의 mCounter
	Distance        UMETA(DisplayName = "거리"),           // Self 타일과 Target 타일 사이 거리
	TargetCount     UMETA(DisplayName = "타겟 수"),        // Ctx.mTargets 개수
	Captured        UMETA(DisplayName = "캡처값"),         // 런타임 상태의 mCaptures[mCaptureKey]
	MovedDistance   UMETA(DisplayName = "이동 거리"),      // 캡처 시점 타일과 현재 타일 사이 거리
	Team            UMETA(DisplayName = "팀 관계"),        // Self 기준 Target의 관계 (0 중립 / 1 아군 / 2 적)
	Custom          UMETA(DisplayName = "커스텀"),         // mCustomClass의 Resolve 결과
};

/**
 * @brief 피연산자 출처 (누구의 값인지)
 */
UENUM()
enum class EPassiveOperandSource : uint8
{
	Self            UMETA(DisplayName = "자신"),           // 패시브 소유자
	Target          UMETA(DisplayName = "대상"),           // Ctx.mTargets[TargetIndex]
};

/**
 * @brief 조건 비교 연산자
 */
UENUM()
enum class EPassiveCompareOp : uint8
{
	Less            UMETA(DisplayName = "<"),
	LessEqual       UMETA(DisplayName = "<="),
	Equal           UMETA(DisplayName = "=="),
	NotEqual        UMETA(DisplayName = "!="),
	GreaterEqual    UMETA(DisplayName = ">="),
	Greater         UMETA(DisplayName = ">"),
	ModuloZero      UMETA(DisplayName = "배수"),           // Lhs % Rhs == 0 (N번째마다)
};

/**
 * @brief 커스텀 피연산자 베이스
 *
 * @details
 * 기본 Kind로 표현할 수 없는 값을 C++ 서브클래스로 계산.
 * FPassiveOperand.mKind == Custom일 때 mCustomClass의 CDO로 Resolve 호출.
 * 상태를 갖지 않으므로 CDO 호출로 충분.
 */
UCLASS(Abstract)
class P_RD_API UPassiveCustomOperand : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * @brief 값 계산
	 * @param Ctx 발동 컨텍스트
	 * @param TargetIndex Ctx.mTargets 인덱스 (Self 기준이면 INDEX_NONE)
	 * @param State 패시브 런타임 상태
	 * @param OutValue 계산 결과
	 * @return 계산 성공 여부 (실패 시 조건 탈락)
	 */
	virtual bool Resolve(IN const FPassiveActivateContext& Ctx, IN int32 TargetIndex, IN const FDynamicPassiveData_Generic& State, OUT float& OutValue) const
		PURE_VIRTUAL(UPassiveCustomOperand::Resolve, return false;);
};

/**
 * @brief 피연산자
 *
 * @details
 * 조건 식의 좌/우변과 효과 수치에 공용으로 쓰는 값 정의.
 * Kind에 따라 필요한 필드만 에디터에 노출(EditCondition).
 * 최종값 = 계산값 x mMultiplier (Const, Team은 배수 미적용).
 */
USTRUCT(BlueprintType)
struct P_RD_API FPassiveOperand
{
	GENERATED_BODY()

	// 값 종류
	UPROPERTY(EditAnywhere, meta = (DisplayName = "종류"))
	EPassiveOperandKind mKind = EPassiveOperandKind::Const;

	// 값 출처 (Attribute, TagCount, Captured, MovedDistance에 사용)
	UPROPERTY(EditAnywhere, meta = (DisplayName = "출처", EditCondition = "mKind == EPassiveOperandKind::Attribute || mKind == EPassiveOperandKind::TagCount || mKind == EPassiveOperandKind::Captured || mKind == EPassiveOperandKind::MovedDistance", EditConditionHides))
	EPassiveOperandSource mSource = EPassiveOperandSource::Self;

	// 고정값 (Const)
	UPROPERTY(EditAnywhere, meta = (DisplayName = "고정값", EditCondition = "mKind == EPassiveOperandKind::Const", EditConditionHides))
	float mConst = 0.f;

	// 읽을 속성 (Attribute)
	UPROPERTY(EditAnywhere, meta = (DisplayName = "속성", EditCondition = "mKind == EPassiveOperandKind::Attribute", EditConditionHides))
	FTacticalAttribute mAttribute;

	// 집계할 태그 (TagCount, 하위 태그 포함)
	UPROPERTY(EditAnywhere, meta = (DisplayName = "태그", EditCondition = "mKind == EPassiveOperandKind::TagCount", EditConditionHides))
	FGameplayTag mTag;

	// 캡처 키 (Captured)
	UPROPERTY(EditAnywhere, meta = (DisplayName = "캡처 키", EditCondition = "mKind == EPassiveOperandKind::Captured", EditConditionHides))
	FName mCaptureKey;

	// 커스텀 계산 클래스 (Custom)
	UPROPERTY(EditAnywhere, meta = (DisplayName = "커스텀 클래스", EditCondition = "mKind == EPassiveOperandKind::Custom", EditConditionHides))
	TSubclassOf<UPassiveCustomOperand> mCustomClass;

	// 계산값에 곱할 배수 (Const, Team 제외)
	UPROPERTY(EditAnywhere, meta = (DisplayName = "배수", EditCondition = "mKind != EPassiveOperandKind::Const && mKind != EPassiveOperandKind::Team", EditConditionHides))
	float mMultiplier = 1.f;

	/**
	 * @brief 값 계산
	 * @param Ctx 발동 컨텍스트
	 * @param TargetIndex Ctx.mTargets 인덱스 (Target 출처가 아니면 무시)
	 * @param State 패시브 런타임 상태
	 * @param OutValue 계산 결과 (배수 적용 후)
	 * @return 계산 성공 여부 (스냅샷 없음, 캡처 없음 등이면 false)
	 */
	bool Resolve(IN const FPassiveActivateContext& Ctx, IN int32 TargetIndex, IN const FDynamicPassiveData_Generic& State, OUT float& OutValue) const;
};

/**
 * @brief 조건 하나 ("좌변 Op 우변")
 *
 * @details
 * 양쪽 피연산자를 계산해 비교. 어느 한쪽이라도 계산 실패면 탈락.
 */
USTRUCT(BlueprintType)
struct P_RD_API FPassiveCondition
{
	GENERATED_BODY()

	// 좌변
	UPROPERTY(EditAnywhere, meta = (DisplayName = "좌변"))
	FPassiveOperand mLhs;

	// 비교 연산자
	UPROPERTY(EditAnywhere, meta = (DisplayName = "연산자"))
	EPassiveCompareOp mOp = EPassiveCompareOp::GreaterEqual;

	// 우변
	UPROPERTY(EditAnywhere, meta = (DisplayName = "우변"))
	FPassiveOperand mRhs;

	/**
	 * @brief 조건 판정 (피연산자 계산 + Compare)
	 * @param Ctx 발동 컨텍스트
	 * @param TargetIndex Ctx.mTargets 인덱스
	 * @param State 패시브 런타임 상태
	 * @return 통과 여부
	 */
	bool Evaluate(IN const FPassiveActivateContext& Ctx, IN int32 TargetIndex, IN const FDynamicPassiveData_Generic& State) const;

	/**
	 * @brief 두 값 비교 (연산자 enum을 실제 비교로 변환)
	 */
	static bool Compare(IN float Lhs, IN EPassiveCompareOp Op, IN float Rhs);
};

/**
 * @brief 조건 판정 유틸
 */
namespace PassiveConditionUtils
{
	/**
	 * @brief 조건 배열 판정 (AND, 빈 배열 = 통과)
	 */
	P_RD_API bool EvaluateAll(IN const TArray<FPassiveCondition>& Conditions, IN const FPassiveActivateContext& Ctx, IN int32 TargetIndex, IN const FDynamicPassiveData_Generic& State);

	/**
	 * @brief 두 타일 사이 거리 (맨해튼 방식)
	 */
	P_RD_API int32 TileDistance(IN const FTileIndex& A, IN const FTileIndex& B);

	/**
	 * @brief 출처에 해당하는 스냅샷 (없으면 nullptr)
	 */
	P_RD_API const UBoardCombatTargetSnapshotData* GetSnapshot(IN const FPassiveActivateContext& Ctx, IN EPassiveOperandSource Source, IN int32 TargetIndex);
}
