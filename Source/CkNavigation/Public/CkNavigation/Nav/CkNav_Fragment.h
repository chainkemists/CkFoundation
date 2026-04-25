#pragma once

#include "CkNav_Fragment_Data.h"

#include "CkEcs/Signal/CkSignal_Macros.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_Nav_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    CK_DEFINE_ECS_TAG(FTag_Nav_NeedsSetup);          // Entity wants crowd registration
    CK_DEFINE_ECS_TAG(FTag_Nav_CrowdRegistered);     // Registered with dtCrowd (index valid)
    CK_DEFINE_ECS_TAG(FTag_Nav_PathPending);         // Request queued / in flight
    CK_DEFINE_ECS_TAG(FTag_Nav_PathReady);           // Path available on entity
    CK_DEFINE_ECS_TAG(FTag_Nav_PathFailed);          // Last request failed
    CK_DEFINE_ECS_TAG(FTag_Nav_MoveTargetDirty);     // Crowd move target needs updating
    CK_DEFINE_ECS_TAG(FTag_Nav_CancelRequested);     // Consumer asked to stop pathing (Request_StopPath)

    // ----------------------------------------------------------------------------------------------------------------

    using FFragment_Nav_AgentParams   = FCk_Nav_AgentParams;
    using FFragment_Nav_PathResult    = FCk_Nav_PathResult;
    using FFragment_Nav_CrowdVelocity = FCk_Nav_CrowdVelocity;

    // ----------------------------------------------------------------------------------------------------------------

    struct CKNAVIGATION_API FFragment_Nav_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_Nav_Requests);

        friend class FProcessor_Nav_HandleRequests;
        friend class ::UCk_Utils_Nav_UE;

        using RequestType = FCk_Request_Nav_FindPath;

    private:
        TArray<RequestType> _Requests;

    public:
        CK_PROPERTY(_Requests);
    };

    // ----------------------------------------------------------------------------------------------------------------

    // Internal crowd state — NOT reflected. Holds the dtCrowd agent index assigned at registration.
    // Index of -1 is never stored: entities that fail registration get neither this fragment nor
    // FTag_Nav_CrowdRegistered (P3-7 + crowd-overflow handling per Pass-3 spec).
    struct CKNAVIGATION_API FFragment_Nav_CrowdAgent
    {
    public:
        CK_GENERATED_BODY(FFragment_Nav_CrowdAgent);

        friend struct ::FCk_Nav_Algorithm;
        friend class FProcessor_Nav_CrowdSetup;
        friend class FProcessor_Nav_CrowdPushPosition;
        friend class FProcessor_Nav_CrowdUpdateTarget;
        friend class FProcessor_Nav_CrowdReadVelocity;
        friend class FProcessor_Nav_CrowdEndPlay;
        friend class FProcessor_Nav_CrowdCancel;

    private:
        int32 _CrowdAgentIndex = -1;

    public:
        CK_PROPERTY_GET(_CrowdAgentIndex);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_Nav_CrowdAgent, _CrowdAgentIndex);
    };

    // ----------------------------------------------------------------------------------------------------------------

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKNAVIGATION_API,
        OnNavPathReady,
        FCk_Delegate_Nav_OnPathReady,
        FCk_Handle_NavAgent,
        FCk_Nav_PathResult);

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKNAVIGATION_API,
        OnNavPathFailed,
        FCk_Delegate_Nav_OnPathFailed,
        FCk_Handle_NavAgent);
}

// --------------------------------------------------------------------------------------------------------------------
