#pragma once

#include "CkResolverTarget_Fragment_Data.h"

#include "CkEcs/Fragments/ReplicatedObjects/CkReplicatedObjects_Fragment_Params.h"

#include "CkSignal/CkSignal_Macros.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_ResolverTarget_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // --------------------------------------------------------------------------------------------------------------------

    struct CKRESOLVER_API FFragment_ResolverTarget_Params
    {
    public:
        CK_GENERATED_BODY(FFragment_ResolverTarget_Params);

    public:
        using ParamsType = FCk_Fragment_ResolverTarget_ParamsData;

    private:
        ParamsType _Params;

    public:
        CK_PROPERTY_GET(_Params);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_ResolverTarget_Params, _Params);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKRESOLVER_API FFragment_ResolverTarget_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_ResolverTarget_Current);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKRESOLVER_API FFragment_ResolverTarget_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_ResolverTarget_Requests);

    public:
        friend class FProcessor_ResolverTarget_HandleRequests;
        friend class UCk_Utils_ResolverTarget_UE;

    public:
        using ResolverRequestType = FCk_Request_ResolverTarget_InitiateNewResolution;
        using ResolverRequestList = TArray<ResolverRequestType>;

    private:
        ResolverRequestList _ResolverRequests;

    private:
        CK_PROPERTY_GET(_ResolverRequests);
    };

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKRESOLVER_API, ResolverTarget_OnNewResolverDataBundle,
        FCk_Delegate_ResolverTarget_OnNewResolverDataBundle_MC,
        FCk_Handle_ResolverTarget,
        FCk_Handle_ResolverDataBundle);
}

// --------------------------------------------------------------------------------------------------------------------
