#pragma once

#include "CkCamera/GameplayCamera/CkGameplayCamera_Fragment.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // ----------------------------------------------------------------------------------------------------------------
    // Spawns/destroys modifier entities in response to Add/Remove requests. Attaches the modifier EntityScript
    // synchronously (mirroring UCk_Utils_SmState_UE::Create) — see the note in CkGameplayCamera_Fragment.h.
    // ----------------------------------------------------------------------------------------------------------------
    class CKCAMERA_API FProcessor_GameplayCamera_HandleRequests : public ck_exp::TProcessor<
            FProcessor_GameplayCamera_HandleRequests,
            FCk_Handle_GameplayCamera,
            TReadOnly<FFragment_GameplayCamera_Requests>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_Camera;
        using MarkedDirtyBy = FFragment_GameplayCamera_Requests;

    public:
        using TProcessor::TProcessor;

    public:
        auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_GameplayCamera_Requests& InRequestsComp) const -> void;

    private:
        auto DoHandleRequest(
            HandleType InHandle,
            const FCk_Request_GameplayCamera_AddModifier& InRequest) const -> void;

        auto DoHandleRequest(
            HandleType InHandle,
            const FCk_Request_GameplayCamera_RemoveModifier& InRequest) const -> void;
    };

    // ----------------------------------------------------------------------------------------------------------------
    // Composes the active modifiers' contributions into the director's _ComposedProfile.
    // M0: resets to default each frame. M1: walks the modifier Record and dispatches DoContributeToProfile.
    // ----------------------------------------------------------------------------------------------------------------
    class CKCAMERA_API FProcessor_GameplayCamera_ComposeProfile : public ck_exp::TProcessor<
            FProcessor_GameplayCamera_ComposeProfile,
            FCk_Handle_GameplayCamera,
            TReadWrite<FFragment_GameplayCamera_Current>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group    = FGroup_Gameplay_Camera;
        using RunAfter = TDepList<FProcessor_GameplayCamera_HandleRequests>;

    public:
        using TProcessor::TProcessor;

    public:
        auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_GameplayCamera_Current& InCurrent) const -> void;
    };

    // ----------------------------------------------------------------------------------------------------------------
    // Runs the POV pipeline against the composed profile and writes the resolved FMinimalViewInfo.
    // M0: a trivial third-person POV derived from the director's transform + composed profile.
    // ----------------------------------------------------------------------------------------------------------------
    class CKCAMERA_API FProcessor_GameplayCamera_UpdatePOV : public ck_exp::TProcessor<
            FProcessor_GameplayCamera_UpdatePOV,
            FCk_Handle_GameplayCamera,
            TReadWrite<FFragment_GameplayCamera_Current>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group    = FGroup_Gameplay_Camera;
        using RunAfter = TDepList<FProcessor_GameplayCamera_ComposeProfile>;

    public:
        using TProcessor::TProcessor;

    public:
        auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_GameplayCamera_Current& InCurrent) const -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
