#pragma once

#include "CkCore/Enums/CkEnums.h"
#include "CkCore/Format/CkFormat.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_Typesafe.h"
#include "CkEcs/Request/CkRequest_Data.h"

#include <Engine/Texture2D.h>
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

    // Immutable post-Add, like _Priority below: which texture a definition draws is its identity, and every
    // adopter so far re-skins by tinting one authored icon rather than swapping the texture. There is no
    // Current copy and no request to change it — a definition that needs a different icon is a different
    // definition (compose a second one keyed to another consumer, or re-Create).
    //
    // Stays a soft reference the whole way through: this module never loads or dereferences it, so composing
    // a definition costs no texture load and a dedicated server never pays for one. The presentation layer
    // that draws the marker is what resolves it.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TSoftObjectPtr<UTexture2D> _Icon;

    // The two below are the AUTHORED SEED for the runtime display state: Add copies them into the Current
    // fragment, and every later change goes through Request_SetTint / Request_SetSizeHint. Read Current
    // (Get_Tint / Get_SizeHint), never these — Params is what the entity STARTED as.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FLinearColor _Tint = FLinearColor::White;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FVector2D _SizeHint = FVector2D{32.0, 32.0};

    // Immutable post-Add: projectors read these straight off Params every update, so there is no Current
    // copy and no request to change them.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    int32 _Priority = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_Poi_OffscreenPolicy _OffscreenPolicy = ECk_Poi_OffscreenPolicy::Hide;

public:
    CK_PROPERTY_GET(_Consumer);
    CK_PROPERTY(_Icon);
    CK_PROPERTY(_Tint);
    CK_PROPERTY(_SizeHint);
    CK_PROPERTY(_Priority);
    CK_PROPERTY(_OffscreenPolicy);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_PoiDisplayDefinition_ParamsData, _Consumer);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKPOIDISPLAYDEFINITION_API FCk_Request_PoiDisplayDefinition_SetTint : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_PoiDisplayDefinition_SetTint);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_PoiDisplayDefinition_SetTint);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FLinearColor _Tint = FLinearColor::White;

public:
    CK_PROPERTY_GET(_Tint);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_PoiDisplayDefinition_SetTint, _Tint);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKPOIDISPLAYDEFINITION_API FCk_Request_PoiDisplayDefinition_SetSizeHint : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_PoiDisplayDefinition_SetSizeHint);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_PoiDisplayDefinition_SetSizeHint);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FVector2D _SizeHint = FVector2D{32.0, 32.0};

public:
    CK_PROPERTY_GET(_SizeHint);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_PoiDisplayDefinition_SetSizeHint, _SizeHint);
};

// --------------------------------------------------------------------------------------------------------------------

// Fires once per request drain in which the display state actually changed — NOT once per request, so a
// caller that sets tint and size hint in the same frame wakes its consumers exactly once. Carries only the
// handle: consumers re-read whichever of Get_Tint / Get_SizeHint they care about.
DECLARE_DYNAMIC_DELEGATE_OneParam(
    FCk_Delegate_PoiDisplayDefinition_DisplayChanged,
    FCk_Handle_PoiDisplayDefinition, InHandle);

// --------------------------------------------------------------------------------------------------------------------
