#pragma once

#include "CkCore/Enums/CkEnums.h"
#include "CkCore/Format/CkFormat.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_Typesafe.h"

#include "CkPoiDisplayDefinition/CkPoi_DisplayDefinition.h"

#include <GameplayTagContainer.h>

#include "CkPoiDisplayDefinition_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

// Projector-agnostic off-screen behavior: the compass clamps to its arc edge, a future minimap clamps to its
// border, a world-space indicator clamps to the viewport — all reading this one policy.
UENUM(BlueprintType)
enum class ECk_Poi_OffscreenPolicy : uint8
{
    Hide,
    ClampToEdge
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Poi_OffscreenPolicy);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKPOIDISPLAYDEFINITION_API FCk_Handle_PoiDisplayDefinition : public FCk_Handle_TypeSafe { GENERATED_BODY()  CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_PoiDisplayDefinition); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_PoiDisplayDefinition);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKPOIDISPLAYDEFINITION_API FCk_Fragment_PoiDisplayDefinition_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_PoiDisplayDefinition_ParamsData);

private:
    // Which projector-side consumer this definition is FOR (e.g. Poi.Consumer.Compass, Poi.Consumer.Minimap).
    // One entity carries at most one definition per consumer — Create keys its children by this tag.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, Categories = "Poi.Consumer"))
    FGameplayTag _Consumer;

    // OPAQUE to gameplay — never loaded or dereferenced by the data layer. Presentation layers resolve it.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TSoftObjectPtr<UCk_Poi_DisplayDefinition_PDA> _DisplayAsset;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    int32 _Priority = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_Poi_OffscreenPolicy _OffscreenPolicy = ECk_Poi_OffscreenPolicy::Hide;

public:
    CK_PROPERTY_GET(_Consumer);
    CK_PROPERTY(_DisplayAsset);
    CK_PROPERTY(_Priority);
    CK_PROPERTY(_OffscreenPolicy);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_PoiDisplayDefinition_ParamsData, _Consumer);
};

// --------------------------------------------------------------------------------------------------------------------
