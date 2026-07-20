#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Validation/CkIsValid.h"

#include <CoreMinimal.h>

#include "CkMinimap_WorldBounds.generated.h"

// --------------------------------------------------------------------------------------------------------------------

// Axis-aligned world-XY rectangle (cm) shared by the Minimap FixedBounds projection, the FogOfWar grid, and the
// MapLayer PDA. Axis-aligned by design — a rotated map texture is a presentation-layer rotation, not data.
USTRUCT(BlueprintType)
struct CKMINIMAP_API FCk_Minimap_WorldBounds
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Minimap_WorldBounds);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FVector2D _Center = FVector2D::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = 0))
    FVector2D _HalfExtents = FVector2D::ZeroVector;

public:
    CK_PROPERTY_GET(_Center);
    CK_PROPERTY_GET(_HalfExtents);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Minimap_WorldBounds, _Center, _HalfExtents);
};

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_CUSTOM_IS_VALID_INLINE(FCk_Minimap_WorldBounds, IsValid_Policy_Default,
[=](const FCk_Minimap_WorldBounds& InBounds)
{
    return InBounds.Get_HalfExtents().X > 0.0 && InBounds.Get_HalfExtents().Y > 0.0;
});

// --------------------------------------------------------------------------------------------------------------------
