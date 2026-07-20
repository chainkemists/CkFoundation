#pragma once

#include "CkCore/Macros/CkMacros.h"

#include <Engine/DataAsset.h>
#include <Engine/Texture2D.h>

#include "CkPoi_DisplayDefinition.generated.h"

// --------------------------------------------------------------------------------------------------------------------

// Presentation-side description of how a POI should LOOK. This asset is OPAQUE to CkPoi/CkCompass — the runtime
// modules only carry a soft reference to it and never load or dereference it. UI layers (compass ribbon widget,
// future minimap, world-space indicators) resolve and interpret it. Keeping visuals here (and out of the Params
// fragment) is what keeps the data layer UI-agnostic.
UCLASS(BlueprintType)
class CKPOI_API UCk_Poi_DisplayDefinition_PDA : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Poi_DisplayDefinition_PDA);

private:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Display",
              meta = (AllowPrivateAccess = true))
    TSoftObjectPtr<UTexture2D> _Icon;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Display",
              meta = (AllowPrivateAccess = true))
    FLinearColor _Tint = FLinearColor::White;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Display",
              meta = (AllowPrivateAccess = true))
    FVector2D _SizeHint = FVector2D{32.0, 32.0};

public:
    CK_PROPERTY_GET(_Icon);
    CK_PROPERTY_GET(_Tint);
    CK_PROPERTY_GET(_SizeHint);
};

// --------------------------------------------------------------------------------------------------------------------
