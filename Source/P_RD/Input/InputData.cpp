// Fill out your copyright notice in the Description page of Project Settings.


#include "Input/InputData.h"

UInputData::UInputData()
{
}

TObjectPtr<UInputAction> UInputData::FindAction(const FString& Name)	const
{
	return mActions.FindRef(Name);
}

// CDO가 기본으로 생성되며 입력 에셋들은 모두 로딩이 되어 있는 상태이므로
// 필요하면 CDO를 이용해서 이 에셋에 접근해서 사용한다.
UCombatInputData::UCombatInputData()
{
	static ConstructorHelpers::FObjectFinder<UInputMappingContext>	InputContext(TEXT("/Script/EnhancedInput.InputMappingContext'/Game/BP/Input/Combat/IMC_Camera.IMC_Camera'"));

	if (InputContext.Succeeded())
		mContext = InputContext.Object;

	static ConstructorHelpers::FObjectFinder<UInputAction>	ClickMoveAction(TEXT("/Script/EnhancedInput.InputAction'/Game/BP/Input/Combat/IA_CameraClickMove.IA_CameraClickMove'"));

	if (ClickMoveAction.Succeeded())
		mActions.Add(TEXT("ClickMove"), ClickMoveAction.Object);

	static ConstructorHelpers::FObjectFinder<UInputAction>	ZoomAction(TEXT("/Script/EnhancedInput.InputAction'/Game/BP/Input/Combat/IA_CameraZoom.IA_CameraZoom'"));

	if (ClickMoveAction.Succeeded())
		mActions.Add(TEXT("Zoom"), ZoomAction.Object);

	static ConstructorHelpers::FObjectFinder<UInputAction>	TouchMoveAction(TEXT("/Script/EnhancedInput.InputAction'/Game/BP/Input/Combat/IA_CameraTouchMove.IA_CameraTouchMove'"));

	if (ClickMoveAction.Succeeded())
		mActions.Add(TEXT("TouchMove"), TouchMoveAction.Object);

	// =========================================
	// ZoomTest
	static ConstructorHelpers::FObjectFinder<UInputAction>	FirstTouchAction(TEXT("/Script/EnhancedInput.InputAction'/Game/BP/Input/Combat/IA_FirstTouch.IA_FirstTouch'"));

	if (FirstTouchAction.Succeeded())
		mActions.Add(TEXT("FirstTouch"), FirstTouchAction.Object);

	static ConstructorHelpers::FObjectFinder<UInputAction>	SecondTouchAction(TEXT("/Script/EnhancedInput.InputAction'/Game/BP/Input/Combat/IA_SecondTouch.IA_SecondTouch'"));

	if (SecondTouchAction.Succeeded())
		mActions.Add(TEXT("SecondTouch"), SecondTouchAction.Object);


}