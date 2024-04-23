#pragma once
#include "CkCore/Format/CkFormat.h"

#include "CkNet_Common.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Net_ReplicationType : uint8
{
    LocalOnly,
    LocalAndHost,
    HostOnly,
    ClientsOnly,
    All
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Net_ReplicationType);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Net_NetModeType : uint8
{
    None  ,
    Client,
    Host  ,
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Net_NetModeType);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Net_NetExecutionPolicy : uint8
{
    // Prefer running on Server. If in SinglePlayer, run locally
    PreferHost,
    // Run on owning client and Server
    LocalAndHost,
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Net_NetExecutionPolicy);
