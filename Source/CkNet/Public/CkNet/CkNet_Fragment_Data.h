#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkNet/CkNet_Common.h"

#include "CkNet_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Net_EntityNetRole : uint8
{
    None,
    Authority,
    Proxy
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT()
struct FCk_Net_ConnectionSettings
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Net_ConnectionSettings);

private:
    ECk_Net_NetModeType _NetMode = ECk_Net_NetModeType::None;
    ECk_Net_EntityNetRole _NetRole = ECk_Net_EntityNetRole::None;

public:
    CK_PROPERTY_GET(_NetMode);
    CK_PROPERTY_GET(_NetRole);

    CK_DEFINE_CONSTRUCTORS(FCk_Net_ConnectionSettings, _NetMode, _NetRole);
};
