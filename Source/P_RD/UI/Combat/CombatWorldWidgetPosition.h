#pragma once

#include "Layout/Geometry.h"

namespace CombatWorldWidgetPosition
{
    // Projection returns DPI-scaled viewport coordinates. Canvas slots are local to
    // their actual parent, which may be offset by a safe area or additionally scaled.
    inline FVector2D ViewportToCanvas(const FVector2D& Position,
        const FGeometry& Viewport, const FGeometry& Canvas)
    {
        return Canvas.AbsoluteToLocal(Viewport.LocalToAbsolute(Position));
    }
}
