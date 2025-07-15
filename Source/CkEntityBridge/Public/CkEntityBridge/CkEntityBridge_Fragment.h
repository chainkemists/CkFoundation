#pragma once

#include "CkEntityBridge_Fragment_Data.h"
#include "CkEcs/Signal/CkSignal_Macros.h"

#include <variant>

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    struct CKENTITYBRIDGE_API FFragment_EntityBridge_Requests
    {
        CK_GENERATED_BODY(FFragment_EntityBridge_Requests);

    public:
        friend class UCk_Utils_EntityBridge_UE;
        friend class FProcessor_EntityBridge_HandleRequests;

    public:
        using RequestType = FCk_Request_EntityBridge_SpawnEntity;

    private:
        RequestType _Request;

    public:
        CK_PROPERTY_GET(_Request);
    };

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKENTITYBRIDGE_API,
        OnEntitySpawned,
        FCk_Delegate_EntityBridge_OnEntitySpawned_MC,
        FCk_Handle,
        FInstancedStruct);
}

// --------------------------------------------------------------------------------------------------------------------

