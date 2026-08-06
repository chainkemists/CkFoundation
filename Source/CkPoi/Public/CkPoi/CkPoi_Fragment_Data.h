#pragma once

#include "CkCore/Format/CkFormat.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_Typesafe.h"

#include <GameplayTagContainer.h>

#include "CkPoi_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKPOI_API FCk_Handle_Poi : public FCk_Handle_TypeSafe { GENERATED_BODY()  CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_Poi); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_Poi);

// --------------------------------------------------------------------------------------------------------------------

// Config for the POI meta-feature. Category is the entity's CkEntityTag identity (Poi.Category.*); Label (optional)
// composes a CkLabel role tag. Presentation (icon/priority/offscreen), range/fade, and state all live in their own
// modules now (CkPoiDisplayDefinition / CkVisibleRange / CkEntityTag).
USTRUCT(BlueprintType)
struct CKPOI_API FCk_Poi_Spec
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Poi_Spec);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, Categories = "Poi.Category"))
    FGameplayTag _Category;

    // Optional entity-role label (CkLabel). Unset = no label composed.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FGameplayTag _Label;

public:
    CK_PROPERTY_GET(_Category);
    CK_PROPERTY(_Label);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Poi_Spec, _Category);
};

// --------------------------------------------------------------------------------------------------------------------
