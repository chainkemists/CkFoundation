#pragma once
#include "CkCore/Format/CkFormat.h"

#include "CkSignal_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Signal_BindingPolicy : uint8
{
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
