#pragma once

#include "CkCore/Format/CkFormat.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkEcs/Request/CkRequest_Data.h"

#include "CkIntentDebugHistory_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta = (HasNativeMake, HasNativeBreak))
struct CKINTENT_API FCk_Handle_IntentDebugHistory : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_IntentDebugHistory); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_IntentDebugHistory);

// --------------------------------------------------------------------------------------------------------------------

/**
 * How many frames the debug history retains. The sampler's ring stays the matcher's WORKING WINDOW (see the
 * module doc's anti-pattern 5); this capacity is the tooling-facing depth — a debugger timeline, a replay
 * scrubber — and the two are deliberately independent knobs.
 */
USTRUCT(BlueprintType)
struct CKINTENT_API FCk_Fragment_IntentDebugHistory_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_IntentDebugHistory_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    int32 _Capacity = 1800;

public:
    CK_PROPERTY_GET(_Capacity);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_IntentDebugHistory_ParamsData, _Capacity);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKINTENT_API FCk_Request_IntentDebugHistory_SetCapacity : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_IntentDebugHistory_SetCapacity);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_IntentDebugHistory_SetCapacity);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    int32 _Capacity = 1800;

public:
    CK_PROPERTY_GET(_Capacity);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_IntentDebugHistory_SetCapacity, _Capacity);
};

// --------------------------------------------------------------------------------------------------------------------
