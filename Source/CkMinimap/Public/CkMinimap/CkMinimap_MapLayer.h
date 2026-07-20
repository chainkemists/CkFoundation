#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkMinimap/CkMinimap_WorldBounds.h"

#include <Engine/DataAsset.h>
#include <Engine/Texture2D.h>

#include "CkMinimap_MapLayer.generated.h"

// --------------------------------------------------------------------------------------------------------------------

// The map-imagery contract: which world rectangle a baked map texture covers. _Bounds IS data — it feeds the
// FixedBounds projection and the FogOfWar grid. _MapTexture is OPAQUE to the data layer (never loaded or
// dereferenced by CkMinimap's runtime — same doctrine as CkPoi's _DisplayAsset); presentation layers resolve
// it. A future baker editor module writes these assets from scene captures.
UCLASS(BlueprintType)
class CKMINIMAP_API UCk_Minimap_MapLayer_PDA : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Minimap_MapLayer_PDA);

private:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map",
              meta = (AllowPrivateAccess = true))
    FCk_Minimap_WorldBounds _Bounds;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map",
              meta = (AllowPrivateAccess = true))
    TSoftObjectPtr<UTexture2D> _MapTexture;

public:
    CK_PROPERTY_GET(_Bounds);
    CK_PROPERTY_GET(_MapTexture);
};

// --------------------------------------------------------------------------------------------------------------------
