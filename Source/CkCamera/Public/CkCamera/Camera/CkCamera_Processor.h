#pragma once

#include "CkCamera/Camera/CkCamera_Fragment.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // ----------------------------------------------------------------------------------------------------------------
    // Spawns/destroys modifier entities in response to Add/Remove requests. Attaches the modifier EntityScript
    // synchronously (mirroring UCk_Utils_SmState_UE::Create) — see the note in CkCamera_Fragment.h.
    // ----------------------------------------------------------------------------------------------------------------
    class CKCAMERA_API FProcessor_Camera_HandleRequests : public ck_exp::TProcessor<
            FProcessor_Camera_HandleRequests,
            FCk_Handle_Camera,
            TReadOnly<FFragment_Camera_Requests>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_Camera;
        using MarkedDirtyBy = FFragment_Camera_Requests;

    public:
        using TProcessor::TProcessor;

    public:
        auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Camera_Requests& InRequestsComp) const -> void;

    private:
        auto DoHandleRequest(
            HandleType InHandle,
            const FCk_Request_Camera_AddModifier& InRequest) const -> void;

        auto DoHandleRequest(
            HandleType InHandle,
            const FCk_Request_Camera_RemoveModifier& InRequest) const -> void;
    };

    // ----------------------------------------------------------------------------------------------------------------
    // Composes the active modifiers' contributions into the director's _ComposedProfile.
    // M0: resets to default each frame. M1: walks the modifier Record and dispatches DoContributeToProfile.
    // ----------------------------------------------------------------------------------------------------------------
    class CKCAMERA_API FProcessor_Camera_ComposeProfile : public ck_exp::TProcessor<
            FProcessor_Camera_ComposeProfile,
            FCk_Handle_Camera,
            TReadWrite<FFragment_Camera_Current>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group    = FGroup_Gameplay_Camera;
        using RunAfter = TDepList<FProcessor_Camera_HandleRequests>;

    public:
        using TProcessor::TProcessor;

    public:
        auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Camera_Current& InCurrent) const -> void;
    };

    // ----------------------------------------------------------------------------------------------------------------
    // Runs the POV pipeline against the composed profile and writes the resolved FMinimalViewInfo.
    // M0: a trivial third-person POV derived from the director's transform + composed profile.
    // ----------------------------------------------------------------------------------------------------------------
    class CKCAMERA_API FProcessor_Camera_UpdatePOV : public ck_exp::TProcessor<
            FProcessor_Camera_UpdatePOV,
            FCk_Handle_Camera,
            TReadWrite<FFragment_Camera_Current>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group    = FGroup_Gameplay_Camera;
        using RunAfter = TDepList<FProcessor_Camera_ComposeProfile>;

    public:
        using TProcessor::TProcessor;

    public:
        auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Camera_Current& InCurrent) const -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
