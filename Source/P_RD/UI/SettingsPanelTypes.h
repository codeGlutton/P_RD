#pragma once

/**
 * @file SettingsPanelTypes.h
 * @brief 설정 패널이 주고받는 값 모델·이벤트·검증 헬퍼 모음.
 *
 * 설정 위젯(SettingsPanelWidget)이 저장/적용 로직을 직접 갖지 않고, 값과 입력 이벤트만 다루도록
 * 그 "데이터 형태"를 여기 모았다. 실제 적용은 이 위젯을 여는 쪽(타이틀/인게임/게임모드)이 한다.
 */

#include "RDMinimal.h"

#include "SettingsPanelTypes.generated.h"

/** @brief 설정 패널이 어느 화면에서 열렸는지. InGame일 때만 "저장 후 종료/포기" 같은 런 액션을 보여준다. */
UENUM(BlueprintType)
enum class ESettingsPanelMode : uint8
{
	Title,    // 타이틀에서 열림(런 관련 액션 없음)
	InGame,   // 인게임에서 열림(런 종료/포기 등 추가)
};

/** @brief 그래픽 품질 단계. 정수로도 쓰여(0/1/2) 품질 요청 값으로 전달된다. */
UENUM(BlueprintType)
enum class ESettingsQualityLevel : uint8
{
	Low = 0,
	Medium = 1,
	High = 2,
};

/** @brief 설정 화면이 보여주고 편집하는 실제 값들(볼륨/진동/화면흔들림/품질). */
USTRUCT(BlueprintType)
struct P_RD_API FSettingsPanelValueModel
{
	GENERATED_BODY()

	UPROPERTY(Category = "Settings", EditAnywhere, BlueprintReadWrite)
	float mBgmVolume = 1.0f;   // 배경음 (0~1)

	UPROPERTY(Category = "Settings", EditAnywhere, BlueprintReadWrite)
	float mSfxVolume = 1.0f;   // 효과음 (0~1)

	UPROPERTY(Category = "Settings", EditAnywhere, BlueprintReadWrite)
	float mUiVolume = 1.0f;    // UI 사운드 (0~1)

	UPROPERTY(Category = "Settings", EditAnywhere, BlueprintReadWrite)
	bool mScreenShakeEnabled = true;   // 화면 흔들림 on/off

	UPROPERTY(Category = "Settings", EditAnywhere, BlueprintReadWrite)
	bool mVibrationEnabled = true;     // 진동(모바일) on/off

	UPROPERTY(Category = "Settings", EditAnywhere, BlueprintReadWrite)
	ESettingsQualityLevel mQualityLevel = ESettingsQualityLevel::Medium;

	UPROPERTY(Category = "Settings", EditAnywhere, BlueprintReadWrite)
	float mMasterVolume = 1.0f;   // 전체 음량 (0~1)

	UPROPERTY(Category = "Settings", EditAnywhere, BlueprintReadWrite)
	bool mEffectsEnabled = true;  // 전투 이펙트 on/off (수신 시스템 미구현 - 값/이벤트만)

	UPROPERTY(Category = "Settings", EditAnywhere, BlueprintReadWrite)
	int32 mFpsLimit = 60;         // FPS 제한(30/60) (수신 시스템 미구현 - 값/이벤트만)

	UPROPERTY(Category = "Settings", EditAnywhere, BlueprintReadWrite)
	bool mUseKoreanLanguage = false;   // 언어(한국어=true / English=false)
};

// 설정 입력이 바뀌면 위젯이 올려보내는 이벤트들. 받는 쪽(타이틀/탑바/게임모드)이 실제 적용.
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSettingsPanelEvent);                              // 버튼류(닫기/저장 등)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSettingsPanelFloatEvent, float, Value);  // 슬라이더(볼륨)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSettingsPanelBoolEvent, bool, Value);   // 체크박스(흔들림/진동)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSettingsPanelIntEvent, int32, Value);    // 품질 단계

namespace RDSettingsPanel
{
	/** @brief 볼륨 값을 0~1 범위로 자른다(슬라이더가 범위를 넘겨도 안전). */
	P_RD_API float NormalizeVolumeValue(float Value);

	/** @brief 품질 enum을 요청용 정수(0/1/2)로 바꾼다. */
	P_RD_API int32 ToQualityRequestValue(ESettingsQualityLevel QualityLevel);

	/** @brief 저장된 품질 enum을 가장 가까운 품질 단계로 되돌린다(패널 표시 동기화용). */
	P_RD_API ESettingsQualityLevel FromOverallQuality(int32 OverallQuality);

	/** @brief 값 모델 전체를 안전 범위로 정리한다(모든 볼륨 0~1 클램프). */
	P_RD_API FSettingsPanelValueModel SanitizeValueModel(const FSettingsPanelValueModel& ValueModel);
}
