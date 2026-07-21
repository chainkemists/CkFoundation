#pragma once

#include "CkCore/Enums/CkEnums.h"
#include "CkCore/Format/CkFormat.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_Typesafe.h"
#include "CkEcs/Request/CkRequest_Data.h"

#include "CkPoi/CkPoi_DisplayDefinition.h"

#include <GameplayTagContainer.h>

#include "CkPoi_Fragment_Data.generated.h"

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

// Persistence payload for a Poi child entity (v3 rebuild+hydrate: Produce on save, HydrationApply on load).
// NAMING DEVIATION (deliberate): save-only payloads are normally FCk_SaveData_<Feature>, but persistence handlers
// key payloads by type path — renaming the struct later breaks old saves' payload mapping. Phase 4 commits this
// type to the net wire as well (Register_NetAndSave_*), so it carries the FCk_RepData_ name from day one.
USTRUCT()
struct CKPOI_API FCk_RepData_Poi
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_RepData_Poi);

private:
    UPROPERTY()
    ECk_EnableDisable _EnableDisable = ECk_EnableDisable::Enable;

    UPROPERTY()
    FGameplayTagContainer _StateTags;

public:
    CK_PROPERTY(_EnableDisable);
    CK_PROPERTY(_StateTags);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKPOI_API FCk_Handle_Poi : public FCk_Handle_TypeSafe { GENERATED_BODY()  CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_Poi); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_Poi);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKPOI_API FCk_Fragment_Poi_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_Poi_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, Categories = "Poi.Category"))
    FGameplayTag _Category;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FText _DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    int32 _Priority = 0;

    // 0 = unlimited. World units (cm). Projectors (compass/minimap) cull POIs beyond this range.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = 0))
    float _MaxVisibleRange = 0.0f;

    // 0 = none. World units (cm). Projectors cull POIs NEARER than this range — the legacy
    // "ShowAfterDistance" behavior (e.g. a tracked NPC that only appears once it is 10m+ away).
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = 0))
    float _MinVisibleRange = 0.0f;

    // 0 = hard cut at the range boundaries. Width (cm) of the fade band INSIDE each active
    // visible-range boundary: projectors ramp the entry's FadeAlpha 0→1 across it (fade-in just
    // past MinVisibleRange, fade-out approaching MaxVisibleRange). Boundaries that are 0/off
    // never fade.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = 0))
    float _RangeFadeBandCm = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_Poi_OffscreenPolicy _OffscreenPolicy = ECk_Poi_OffscreenPolicy::Hide;

    // Offset from the owning entity's Transform (owner-space). The POI's world position is
    // OwnerTransform.TransformPosition(_RelativeLocation).
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FVector _RelativeLocation = FVector::ZeroVector;

    // OPAQUE to CkPoi/CkCompass — never loaded or dereferenced by the data layer. Presentation layers resolve it.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TSoftObjectPtr<UCk_Poi_DisplayDefinition_PDA> _DisplayAsset;

public:
    CK_PROPERTY_GET(_Category);
    CK_PROPERTY(_DisplayName);
    CK_PROPERTY(_Priority);
    CK_PROPERTY(_MaxVisibleRange);
    CK_PROPERTY(_MinVisibleRange);
    CK_PROPERTY(_RangeFadeBandCm);
    CK_PROPERTY(_OffscreenPolicy);
    CK_PROPERTY(_RelativeLocation);
    CK_PROPERTY(_DisplayAsset);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_Poi_ParamsData, _Category);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKPOI_API FCk_Request_Poi_EnableDisable : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Poi_EnableDisable);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_Poi_EnableDisable);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_EnableDisable _EnableDisable = ECk_EnableDisable::Enable;

public:
    CK_PROPERTY_GET(_EnableDisable);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_Poi_EnableDisable, _EnableDisable);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKPOI_API FCk_Request_Poi_AddStateTag : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Poi_AddStateTag);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_Poi_AddStateTag);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FGameplayTag _Tag;

public:
    CK_PROPERTY_GET(_Tag);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_Poi_AddStateTag, _Tag);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKPOI_API FCk_Request_Poi_RemoveStateTag : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Poi_RemoveStateTag);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_Poi_RemoveStateTag);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FGameplayTag _Tag;

public:
    CK_PROPERTY_GET(_Tag);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_Poi_RemoveStateTag, _Tag);
};

// --------------------------------------------------------------------------------------------------------------------

// Composite replace-all. Public API convenience AND the load-hydration vehicle: restored state rides the request
// FIFO so it lands AFTER any state-seeding requests a replayed Construct enqueued (rebuild+hydrate contract).
USTRUCT(BlueprintType)
struct CKPOI_API FCk_Request_Poi_SetStateTags : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Poi_SetStateTags);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_Poi_SetStateTags);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FGameplayTagContainer _Tags;

public:
    CK_PROPERTY_GET(_Tags);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_Poi_SetStateTags, _Tags);
};

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_Poi_StateChanged,
    FCk_Handle_Poi, InHandle,
    FGameplayTagContainer, InStateTags);

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_Poi_EnableDisable,
    FCk_Handle_Poi, InHandle,
    ECk_EnableDisable, InEnableDisable);

// --------------------------------------------------------------------------------------------------------------------
