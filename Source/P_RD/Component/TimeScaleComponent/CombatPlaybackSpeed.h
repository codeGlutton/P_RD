#pragma once

#include "CoreMinimal.h"

namespace CombatPlaybackSpeed
{
    inline constexpr int32 Max = 8;
    inline int32 Clamp(int32 Speed) { return FMath::Clamp(Speed, 1, Max); }
    inline int32 Next(int32 Speed) { return Clamp(Speed) % Max + 1; }
}
