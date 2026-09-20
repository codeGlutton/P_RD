#pragma once

#include "CoreMinimal.h"
#include "Framework/Application/IInputProcessor.h"
#include "Input/Events.h"
#include "InputCoreTypes.h"

// Slate routes real touch through the same down/up preprocessing path as the mouse.
// Keep accepted presses paired with their release, even if the lesson advances on press.
class FTutorialInputGate final : public IInputProcessor
{
  public:
	TFunction<bool()> IsActive;
	TFunction<bool()> IsGameFocused;
	TFunction<bool(const FVector2D &)> IsInViewport;
	TFunction<bool(const FVector2D &)> IsAllowed;

	void Tick(float, FSlateApplication &, TSharedRef<ICursor>) override
	{
	}
	bool HandleMouseButtonDownEvent(FSlateApplication &, const FPointerEvent &Event) override
	{
		if (!IsActive() || !IsInViewport(Event.GetScreenSpacePosition()))
			return false;
		const FString Pointer = PressKey(Event);
		const bool Primary = Event.IsTouchEvent() || Event.GetEffectingButton() == EKeys::LeftMouseButton;
		if (Primary && Accepted.IsEmpty() && IsAllowed(Event.GetScreenSpacePosition()))
		{
			Accepted.Add(Pointer);
			return false;
		}
		Rejected.Add(Pointer);
		return true;
	}
	bool HandleMouseButtonUpEvent(FSlateApplication &, const FPointerEvent &Event) override
	{
		const FString Pointer = PressKey(Event);
		if (Accepted.Remove(Pointer) > 0)
			return false;
		if (Rejected.Remove(Pointer) > 0)
			return true;
		return IsActive() && IsInViewport(Event.GetScreenSpacePosition());
	}
	bool HandleMouseButtonDoubleClickEvent(FSlateApplication &App, const FPointerEvent &Event) override
	{
		return HandleMouseButtonDownEvent(App, Event);
	}
	bool HandleMouseMoveEvent(FSlateApplication &, const FPointerEvent &Event) override
	{
		for (const auto &Press : Accepted)
			if (Press.StartsWith(PointerKey(Event)))
				return false;
		return IsActive() && IsInViewport(Event.GetScreenSpacePosition()) && !IsAllowed(Event.GetScreenSpacePosition());
	}
	bool HandleMouseWheelOrGestureEvent(FSlateApplication &, const FPointerEvent &Event, const FPointerEvent *) override
	{
		return IsActive() && IsInViewport(Event.GetScreenSpacePosition());
	}
	bool HandleKeyDownEvent(FSlateApplication &, const FKeyEvent &Event) override
	{
		// Window close remains an OS operation; game navigation/shortcuts are locked.
		return IsActive() && IsGameFocused() && !(Event.IsAltDown() && Event.GetKey() == EKeys::F4);
	}
	bool HandleKeyUpEvent(FSlateApplication &App, const FKeyEvent &Event) override
	{
		return HandleKeyDownEvent(App, Event);
	}
	bool HandleAnalogInputEvent(FSlateApplication &, const FAnalogInputEvent &) override
	{
		return IsActive() && IsGameFocused();
	}
	const TCHAR *GetDebugName() const override
	{
		return TEXT("FirstRoomTutorial");
	}

  private:
	static FString PointerKey(const FPointerEvent &Event)
	{
		return FString::Printf(TEXT("%u:%u:%d:"), Event.GetUserIndex(), Event.GetPointerIndex(), Event.IsTouchEvent());
	}
	static FString PressKey(const FPointerEvent &Event)
	{
		return PointerKey(Event) + Event.GetEffectingButton().ToString();
	}
	TSet<FString> Accepted;
	TSet<FString> Rejected;
};
