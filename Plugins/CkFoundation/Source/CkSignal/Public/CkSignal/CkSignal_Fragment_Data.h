#pragma once
#include "CkFormat/CkFormat.h"

#include "CkSignal_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Signal_PayloadInFlight : uint8
{
    FireIfPayloadInFlight,
    IgnorePayloadInFlight
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Signal_PayloadInFlight);

// --------------------------------------------------------------------------------------------------------------------
