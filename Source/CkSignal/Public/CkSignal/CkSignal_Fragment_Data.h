#pragma once
#include "CkCore/Format/CkFormat.h"

#include "CkSignal_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Signal_BindingPolicy : uint8
{
    // Note: you only receive the LAST payload if the Signal was fired multiple times AND you bound AFTER the Signal(s) were fired.
    // If you would like to receive ALL the payloads, then the Signal Payload needs to be an Array.
    FireIfPayloadInFlightThisFrame,
    // Note: you only receive the LAST payload if the Signal was fired multiple times AND you bound AFTER the Signal(s) were fired
    // If you would like to receive ALL the payloads, then the Signal Payload needs to be an Array.
    FireIfPayloadInFlight,
    IgnorePayloadInFlight
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Signal_BindingPolicy);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Signal_PostFireBehavior : uint8
{
    DoNothing,
    Unbind
};

// --------------------------------------------------------------------------------------------------------------------
