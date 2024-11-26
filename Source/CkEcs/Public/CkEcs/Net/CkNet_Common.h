#pragma once
#include "CkCore/Format/CkFormat.h"

#include "CkNet_Common.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Net_ReplicationType : uint8
{
    // Replicates on machine(s) that locally owns the object (NOTE: the Player is Local to BOTH Client and Server)
    LocalOnly UMETA(DisplayName="Local Only (Has Authority)"),
    // Replicates on machine(s) that locally owns the object AND the Host
    LocalAndHost UMETA(DisplayName="Local (Has Authority) AND Host"),
    // Replicates on the Host only
    HostOnly,
    // Replicates on Clients only
    ClientsOnly,
    // Replicates on Client ONLY IF the Client has authority
    LocalClientOnly,
    // Replicates on REMOTE Clients only
    RemoteClientsOnly,
    All
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Net_ReplicationType);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Net_NetModeType : uint8
{
    Unknown,
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
