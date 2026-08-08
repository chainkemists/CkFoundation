#pragma once

#include "CkResolverSource_Fragment_Data.h"

#include "CkEcs/Fragments/ReplicatedObjects/CkReplicatedObjects_Fragment_Params.h"

#include "CkEcs/Handle/CkDebugCallstack_Macros.h"

#include "CkEcs/Signal/CkSignal_Macros.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_ResolverSource_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // --------------------------------------------------------------------------------------------------------------------

    using FFragment_ResolverSource_Params = FCk_ResolverSource_Spec;

    // --------------------------------------------------------------------------------------------------------------------

    struct CKRESOLVER_API FFragment_ResolverSource
    {
    public:
        CK_GENERATED_BODY(FFragment_ResolverSource);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKRESOLVER_API FFragment_ResolverSource_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_ResolverSource_Requests);

    public:
        friend class FProcessor_ResolverSource_HandleRequests;
        friend class UCk_Utils_ResolverSource_UE;

    public:
        using ResolverRequestType = FCk_Request_ResolverSource_InitiateNewResolution;
        using ResolverRequestList = TArray<ResolverRequestType>;

    private:
        ResolverRequestList _ResolverRequests;

    public:
        CK_PROPERTY_GET(_ResolverRequests);
    };

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKRESOLVER_API, ResolverSource_OnNewResolverDataBundle,
        FCk_Delegate_ResolverSource_OnNewResolverDataBundle,
        FCk_Handle_ResolverSource,
        FCk_Handle,
        FCk_Handle_ResolverDataBundle);

    // --------------------------------------------------------------------------------------------------------------------

    CK_ECS_DEFINE_CALLSTACK_FRAGMENT_FOR(FFragment_ResolverSource_Requests);
}

// --------------------------------------------------------------------------------------------------------------------
