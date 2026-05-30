#pragma once

#include "CkCamera/Camera/CkCamera_Fragment.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // ----------------------------------------------------------------------------------------------------------------
    // Spawns/destroys layer entities in response to Add/Remove requests. Attaches the layer EntityScript synchronously
    // (mirroring UCk_Utils_SmState_UE::Create) — see the note in CkCamera_Fragment.h.
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
        static auto DoHandleRequest(
            HandleType InHandle,
            const FCk_Request_Camera_AddLayer& InRequest) -> void;

        static auto DoHandleRequest(
            HandleType InHandle,
            const FCk_Request_Camera_RemoveLayer& InRequest) -> void;
    };

    // ----------------------------------------------------------------------------------------------------------------
    // Per-frame layer lifecycle: prunes fully-blended-out layers (destroying their acquired modifiers first), dispatches
    // Tick on tickable layers, resolves the dominant layer + its look-at, and refreshes the composed-profile cache from
    // the tuner attributes. Does NOT advance the blend alpha — FProcessor_CameraLayer_Blend (FGroup_Gameplay_TimeDelta)
    // owns that, running earlier in the frame so the attributes are recomputed before the POV reads them.
    // ----------------------------------------------------------------------------------------------------------------
    class CKCAMERA_API FProcessor_CameraLayer_Lifecycle : public ck_exp::TProcessor<
            FProcessor_CameraLayer_Lifecycle,
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
        static auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Camera_Current& InCurrent) -> void;
    };

    // ----------------------------------------------------------------------------------------------------------------
    // Runs the POV pipeline against the composed profile (the cached assembly) and writes the resolved FMinimalViewInfo.
    // ----------------------------------------------------------------------------------------------------------------
    class CKCAMERA_API FProcessor_Camera_UpdatePOV : public ck_exp::TProcessor<
            FProcessor_Camera_UpdatePOV,
            FCk_Handle_Camera,
            TReadWrite<FFragment_Camera_Current>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group    = FGroup_Gameplay_Camera;
        using RunAfter = TDepList<FProcessor_CameraLayer_Lifecycle>;

    public:
        using TProcessor::TProcessor;

    public:
        static auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Camera_Current& InCurrent) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
