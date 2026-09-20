#pragma once

#include "CoreMinimal.h"

/** Screen-pixel thresholds shared by the board and its camera. */
namespace RDPointerGesture
{
constexpr float TouchPanSlop = 24.f;
constexpr float MousePanSlop = 3.f;
constexpr float PinchSlop = 3.f;
inline bool HasMoved(const FVector2D& Origin, const FVector2D& Position, bool Touch)
{
	return FVector2D::DistSquared(Origin, Position) > FMath::Square(Touch ? TouchPanSlop : MousePanSlop);
}
}

/** A second finger consumes the whole contact session, including its final release. */
struct FRDBoardTouchSession
{
	void Begin(uint32 Pointer)
	{
		if (Pointers.IsEmpty())
		{
			Primary = Pointer;
			Consumed = false;
		}
		Pointers.Add(Pointer);
		Consumed |= Pointers.Num() > 1;
	}
	void End(uint32 Pointer) { Pointers.Remove(Pointer); }
	bool CanTap(uint32 Pointer) const { return Primary == Pointer && !Consumed; }
	void Reset() { Pointers.Reset(); Primary = MAX_uint32; Consumed = false; }
	TSet<uint32> Pointers;
	uint32 Primary = MAX_uint32;
	bool Consumed = false;
};
