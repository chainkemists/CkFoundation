#pragma once

#include "CkCore/Macros/CkMacros.h"

#include <Materials/MaterialInterface.h>

#include "CkGraphics_Common.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKGRAPHICS_API FCk_MeshMaterialOverride
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_MeshMaterialOverride);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = 0, UIMin = 0))
    int32 _MaterialSlot = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TObjectPtr<UMaterialInterface> _ReplacementMaterial;

public:
    CK_PROPERTY_GET(_MaterialSlot);
    CK_PROPERTY_GET(_ReplacementMaterial);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_MeshMaterialOverride, _MaterialSlot, _ReplacementMaterial);
};

// --------------------------------------------------------------------------------------------------------------------
