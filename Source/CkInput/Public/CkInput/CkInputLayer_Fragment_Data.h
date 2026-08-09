#pragma once

#include "CkCore/Format/CkFormat.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkEcs/Request/CkRequest_Data.h"

#include "CkInput/CkInputSource_Fragment_Data.h"

#include <InputCoreTypes.h>

#include "CkInputLayer_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

// What a matched capture does to the events below it. There is no third state and no handled return: a layer whose
// own state decides whether it consumes expresses that by adding or removing captures, not by declining at delivery.
UENUM(BlueprintType)
enum class ECk_InputLayer_CaptureBehavior : uint8
{
    Consume,
    PassThrough
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_InputLayer_CaptureBehavior);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_InputLayer_CaptureMatch : uint8
{
    Key,
    CatchAll
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_InputLayer_CaptureMatch);

// --------------------------------------------------------------------------------------------------------------------

/**
 * What became of one routed event once the walk over the stack finished. These are the router's three terminal
 * paths and nothing else: an event either ended on a Consume capture (or on the recorded owner of its press),
 * fell off the bottom of the stack, or was a release whose press owner no longer exists.
 *
 * PassedThrough does not mean "nobody saw it" — every PassThrough capture on the way down was delivered. It
 * means no layer ended the walk, which is the property a consumer reasoning about masking actually needs.
 */
UENUM(BlueprintType)
enum class ECk_InputLayer_DeliveryOutcome : uint8
{
    ConsumedByLayer,
    PassedThrough,
    DroppedNoOwner
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_InputLayer_DeliveryOutcome);

// --------------------------------------------------------------------------------------------------------------------

// One declaration of interest. A capture is DATA: it names what the layer wants and what happens to the layers
// below when it matches. It carries no callback, so the "matched but declined" state that a callback's return
// value would express does not exist here and cannot silently eat a key.
USTRUCT(BlueprintType)
struct CKINPUT_API FCk_InputLayer_Capture
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_InputLayer_Capture);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_InputLayer_CaptureMatch _MatchMode = ECk_InputLayer_CaptureMatch::Key;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FKey _Key;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_InputLayer_CaptureBehavior _Behavior = ECk_InputLayer_CaptureBehavior::Consume;

public:
    CK_PROPERTY_GET(_MatchMode);
    CK_PROPERTY_GET(_Key);
    CK_PROPERTY_GET(_Behavior);

public:
    auto operator==(
        const FCk_InputLayer_Capture& InOther) const -> bool
    {
        return _MatchMode == InOther._MatchMode && _Key == InOther._Key && _Behavior == InOther._Behavior;
    }

    CK_DECL_AND_DEF_OPERATOR_NOT_EQUAL(FCk_InputLayer_Capture);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_InputLayer_Capture, _MatchMode, _Key, _Behavior);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta = (HasNativeMake, HasNativeBreak))
struct CKINPUT_API FCk_Handle_InputLayer : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_InputLayer); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_InputLayer);

// --------------------------------------------------------------------------------------------------------------------

/**
 * One routed event paired with what the router did with it. The event half is the verbatim inbox row — the same
 * physical fact a layer's delivery carries — so a consumer reading these never has to reconcile two versions of
 * the same press.
 *
 * The consuming layer is meaningful ONLY for ConsumedByLayer and is an invalid handle otherwise; there is no
 * layer to name when nothing ended the walk. It is also the one field that can be a handle to an entity that
 * died later in the frame, so read it through ck::IsValid rather than storing it.
 */
USTRUCT(BlueprintType)
struct CKINPUT_API FCk_InputLayer_RoutedEvent
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_InputLayer_RoutedEvent);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_InputSource_RawEvent _Event;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_InputLayer_DeliveryOutcome _Outcome = ECk_InputLayer_DeliveryOutcome::PassedThrough;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Handle_InputLayer _ConsumingLayer;

public:
    CK_PROPERTY_GET(_Event);
    CK_PROPERTY_GET(_Outcome);
    CK_PROPERTY(_ConsumingLayer);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_InputLayer_RoutedEvent, _Event, _Outcome);
};

// --------------------------------------------------------------------------------------------------------------------

// A layer's place in one input source's stack. Priority is explicit rather than implied by creation order because
// entities have no inherent order; two layers on the same source may not share one, and the collision is rejected
// at registration rather than tie-broken silently.
USTRUCT(BlueprintType)
struct CKINPUT_API FCk_Fragment_InputLayer_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_InputLayer_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Handle_InputSource _InputSource;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    int32 _Priority = 0;

public:
    CK_PROPERTY_GET(_InputSource);
    CK_PROPERTY_GET(_Priority);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_InputLayer_ParamsData, _InputSource, _Priority);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKINPUT_API FCk_Request_InputLayer_AddCapture : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_InputLayer_AddCapture);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_InputLayer_AddCapture);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_InputLayer_Capture _Capture;

public:
    CK_PROPERTY_GET(_Capture);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_InputLayer_AddCapture, _Capture);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKINPUT_API FCk_Request_InputLayer_RemoveCapture : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_InputLayer_RemoveCapture);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_InputLayer_RemoveCapture);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_InputLayer_CaptureMatch _MatchMode = ECk_InputLayer_CaptureMatch::Key;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FKey _Key;

public:
    CK_PROPERTY_GET(_MatchMode);
    CK_PROPERTY_GET(_Key);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_InputLayer_RemoveCapture, _MatchMode, _Key);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKINPUT_API FCk_Request_InputLayer_AddGlobalAction : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_InputLayer_AddGlobalAction);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_InputLayer_AddGlobalAction);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FKey _Key;

public:
    CK_PROPERTY_GET(_Key);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_InputLayer_AddGlobalAction, _Key);
};

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_ThreeParams(
    FCk_Delegate_InputLayer_CaptureTriggered,
    FCk_Handle_InputLayer, InLayer,
    FCk_InputSource_RawEvent, InEvent,
    FCk_InputLayer_Capture, InCapture);

// --------------------------------------------------------------------------------------------------------------------
