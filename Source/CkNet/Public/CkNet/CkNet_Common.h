#pragma once
#include "CkFormat/CkFormat.h"

#include "CkNet_Common.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Net_ReplicationType : uint8
{
    LocalOnly,
    LocalAndHost,
    HostOnly,
    ClientsOnly,
    ServerOnly,
    All
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Net_ReplicationType);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Net_NetRoleType : uint8
{
    None  ,
    Client,
    Host  ,
    Server UMETA(DisplayName = "Dedicated Server")
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Net_NetRoleType);

// --------------------------------------------------------------------------------------------------------------------
