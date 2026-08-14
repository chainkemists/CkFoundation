#include "CkCamera_Processor.h"

#include "CkCamera/CkCamera_Log.h"
#include "CkCamera/CkCamera_Stats.h"
#include "CkCamera/Camera/CkCamera_Utils.h"
#include "CkCamera/Camera/CameraLayer/CkCameraLayer_Fragment.h"
#include "CkCamera/Camera/CameraLayer/CkCameraLayer_Utils.h"
#include "CkCamera/Camera/CameraLayer/EntityScripts/CkCameraLayer_EntityScript.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/EntityScript/CkEntityScript_Fragment.h"
#include "CkEcs/EntityScript/CkEntityScript_Utils.h"
#include "CkEcs/Handle/CkHandle_Utils.h"
#include "CkEcs/OwningActor/CkOwningActor_Utils.h"
#include "CkEcs/Request/CkRequest_Completion.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include <GameFramework/Pawn.h>
#include <GameFramework/PlayerController.h>

#include "CkEcsExt/Transform/CkTransform_Fragment.h"
#include "CkEcsExt/Transform/CkTransform_Utils.h"

CK_REGISTER_PROCESSOR(ck::FProcessor_Camera_HandleRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_Camera_CancelPendingRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_CameraLayer_Lifecycle);
CK_REGISTER_PROCESSOR(ck::FProcessor_Camera_UpdatePOV);

// --------------------------------------------------------------------------------------------------------------------

DECLARE_CYCLE_STAT(TEXT("Camera::PovRun"), STAT_Camera_PovRun, STATGROUP_CkCamera);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    static auto DoGet_BlendRateFromTime(FCk_Time InTime) -> float
    {
        constexpr auto InstantBlendRate = 1000.0f;

        const auto Seconds = static_cast<float>(InTime.Get_Seconds());
        return Seconds > KINDA_SMALL_NUMBER ? (1.0f / Seconds) : InstantBlendRate;
    }

    // HANDLE REQUESTS

    auto
        FProcessor_Camera_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Camera_Requests& InRequestsComp) const
        -> void
    {
        InHandle.CopyAndRemove(InRequestsComp, [&](const FFragment_Camera_Requests& InRequests)
        {
            ck::algo::ForEachRequest(InRequests._Requests, ck::Visitor(
            [&](const auto& InRequest) -> void
            {
                auto Result = ECk_Request_OperationResult::Failed;
                const auto Guard = MakeCompletionGuard(InRequest, InHandle, Result);

                if (DoHandleRequest(InHandle, InRequest))
                { Result = ECk_Request_OperationResult::Succeeded; }
            }), policy::DontResetContainer{});
        });
    }

    auto
        FProcessor_Camera_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            const FCk_Request_Camera_AddLayer& InRequest)
        -> bool
    {
        const auto LayerClass = InRequest.Get_LayerClass();

        CK_ENSURE_IF_NOT(ck::IsValid(LayerClass),
            TEXT("AddLayer: invalid layer class on camera [{}]"), InHandle)
        { return false; }

        const auto Priority = InRequest.Get_Priority();

        // Must run BEFORE the new layer is connected to the Record, or it would evict itself.
        if (InRequest.Get_StackingBehavior() == ECk_Camera_StackingBehavior::OneOnly)
        {
            ck::FUtils_RecordOfCameraLayers::ForEach_ValidEntry(InHandle,
            [&](FCk_Handle_CameraLayer InSibling)
            {
                if (InSibling.Get<FFragment_CameraLayer_Params>().Get_IsDefault())
                { return; }

                if (InSibling.Get<FFragment_CameraLayer_Params>().Get_Priority() != Priority)
                { return; }

                auto& SiblingBlend = InSibling.Get<FFragment_CameraLayer_Blend>();
                SiblingBlend.Set_TargetAlpha(0.0f);
                SiblingBlend.Set_BlendRate(DoGet_BlendRateFromTime(InRequest.Get_BlendInTime()));
            });
        }

        auto TypedLayer = ::UCk_Utils_CameraLayer_UE::Create(InHandle, LayerClass);

        if (ck::Is_NOT_Valid(TypedLayer))
        { return false; }

        {
            auto& Params = TypedLayer.Get<FFragment_CameraLayer_Params>();
            Params.Set_Priority(Priority);
            Params.Set_CameraTarget(InRequest.Get_CameraTarget());
        }
        {
            auto& Blend = TypedLayer.Get<FFragment_CameraLayer_Blend>();
            Blend.Set_BlendRate(DoGet_BlendRateFromTime(InRequest.Get_BlendInTime()));
        }

        camera::VeryVerbose(TEXT("[Camera] AddLayer [{}] -> entity [{}] on camera [{}]"),
            LayerClass, TypedLayer, InHandle);

        return true;
    }

    auto
        FProcessor_Camera_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            const FCk_Request_Camera_RemoveLayer& InRequest)
        -> bool
    {
        const auto LayerClass = InRequest.Get_LayerClass();

        // Begin blend-out; the lifecycle processor prunes (destroys) the entity once its alpha reaches 0.
        ck::FUtils_RecordOfCameraLayers::ForEach_ValidEntry(InHandle,
        [&](FCk_Handle_CameraLayer InLayer)
        {
            if (InLayer.Get<ck::FFragment_CameraLayer_Params>().Get_IsDefault())
            { return; }

            if (InLayer.Get<ck::FFragment_CameraLayer_Params>().Get_LayerClass() != LayerClass)
            { return; }

            auto& Blend = InLayer.Get<ck::FFragment_CameraLayer_Blend>();
            Blend.Set_TargetAlpha(0.0f);
            Blend.Set_BlendRate(DoGet_BlendRateFromTime(InRequest.Get_BlendOutTime()));
        });

        return true;
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Camera_CancelPendingRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Camera_Requests& InRequestsComp)
        -> void
    {
        request::FireCancelledForPending(InHandle, InRequestsComp.Get_Requests());
    }

    // LAYER LIFECYCLE

    auto
        FProcessor_CameraLayer_Lifecycle::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Camera_Current& InCurrent)
        -> void
    {
        auto Dominant         = FCk_Handle_CameraLayer{};
        auto DominantAlpha    = -1.0f;
        auto DominantPriority = TNumericLimits<int32>::Lowest();

        FUtils_RecordOfCameraLayers::ForEach_ValidEntry(InHandle,
        [&](FCk_Handle_CameraLayer InLayer)
        {
            const auto& Blend = InLayer.Get<FFragment_CameraLayer_Blend>();
            const auto Alpha = Blend.Get_Alpha();

            // The acquired modifiers outlive this destroy unless ExitLayer (driven by EndPlay) removes them.
            if (Alpha <= 0.0f && Blend.Get_TargetAlpha() <= 0.0f)
            {
                if (NOT InLayer.Get<FFragment_CameraLayer_Params>().Get_IsDefault())
                { UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(InLayer); }
                return;
            }

            if (Alpha <= 0.0f)
            { return; }

            if (NOT InLayer.Has<FTag_CameraLayer_Active>())
            { return; }

            if (InLayer.Has<FFragment_EntityScript_Current>())
            {
                if (auto* Script = Cast<UCk_CameraLayer_EntityScript>(
                        InLayer.Get<FFragment_EntityScript_Current>().Get_Script().Get());
                    ck::IsValid(Script))
                {
                    if (Script->Get_TickMode() == ECk_Camera_TickMode::Tick)
                    { Script->Tick(InLayer, InDeltaT); }
                }
            }

            const auto Priority = InLayer.Get<FFragment_CameraLayer_Params>().Get_Priority();
            if (Priority > DominantPriority || (Priority == DominantPriority && Alpha >= DominantAlpha))
            {
                DominantPriority = Priority;
                DominantAlpha    = Alpha;
                Dominant         = InLayer;
            }
        });

        // The tuner attributes were already recomputed earlier this frame.
        InCurrent._ComposedProfile = ::UCk_Utils_Camera_UE::Get_Profile(InHandle);

        InCurrent._DominantLookAt.Reset();
        InCurrent._ViewTarget         = FCk_Camera_ViewTargetResolved{};
        InCurrent._DominantLayerClass = nullptr;
        if (ck::IsValid(Dominant))
        {
            const auto& DominantParams = Dominant.Get<FFragment_CameraLayer_Params>();
            InCurrent._DominantLayerClass = DominantParams.Get_LayerClass();

            const auto& CameraTarget = DominantParams.Get_CameraTarget();
            if (const auto& TargetHandle = CameraTarget.Get_Target();
                ck::IsValid(TargetHandle))
            {
                switch (CameraTarget.Get_Mode())
                {
                    case ECk_Camera_TargetMode::LookAt:
                    {
                        InCurrent._DominantLookAt = UCk_Utils_Transform_UE::Get_EntityCurrentLocation(TargetHandle);
                        break;
                    }
                    case ECk_Camera_TargetMode::ViewTarget:
                    {
                        InCurrent._ViewTarget._IsActive = true;
                        InCurrent._ViewTarget._Target   = UCk_Utils_Transform_UE::Get_EntityCurrentTransform(TargetHandle);
                        InCurrent._ViewTarget._Alpha    = DominantAlpha;
                        break;
                    }
                }
            }
        }
    }

    // UPDATE POV

    auto
        FProcessor_Camera_UpdatePOV::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Camera_Current& InCurrent)
        -> void
    {
        const auto& Profile = InCurrent.Get_ComposedProfile();
        const auto& Sensor  = Profile.Get_Sensor();

        // No anchor transform yet (e.g. director on an entity without a transform) — keep the last POV.
        if (NOT InHandle.Has<ck::FFragment_Transform>())
        { return; }

        auto Input = ck::camera::FPov_Input{};
        Input._AnchorTransform      = InHandle.Get<ck::FFragment_Transform>().Get_Transform();
        Input._OrientationIntention = InCurrent.Get_OrientationIntention();
        Input._DeltaSeconds         = static_cast<float>(InDeltaT.Get_Seconds());
        Input._LookAtLocation       = InCurrent._DominantLookAt;
        Input._World                = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);
        Input._TraceIgnoreActor     = UCk_Utils_OwningActor_UE::Get_EntityOwningActor(InHandle);

        {
            SCOPE_CYCLE_COUNTER(STAT_Camera_PovRun);
            ck::camera::FPov::Run(Profile, Input, InCurrent._PovState);
        }

        // The intention is a per-frame DELTA: consume it, or it keeps re-applying after the input source stops.
        InCurrent._OrientationIntention = FVector::ZeroVector;

        // Overrides boom/framing, not the anchor, and leaves FOV alone (the layer's FOV modifier eases that
        // independently). Inactive = the rig POV passes through untouched.
        auto FinalXf = InCurrent._PovState._CameraTransform;
        if (InCurrent._ViewTarget._IsActive)
        {
            FinalXf.Blend(InCurrent._PovState._CameraTransform, InCurrent._ViewTarget._Target, InCurrent._ViewTarget._Alpha);
        }

        auto ViewInfo = FMinimalViewInfo{};
        ViewInfo.Location    = FinalXf.GetLocation();
        ViewInfo.Rotation    = FinalXf.Rotator();
        ViewInfo.FOV         = Sensor.Get_FOV();
        ViewInfo.DesiredFOV  = Sensor.Get_FOV();
        if (Sensor.Get_ConstrainAspectRatio())
        {
            ViewInfo.bConstrainAspectRatio = true;
            ViewInfo.AspectRatio          = Sensor.Get_AspectRatio();
        }

        InCurrent._ViewInfo = ViewInfo;

        // Camera-authoritative control rotation. One-way by design: the camera reads input intention, never control
        // rotation, so this can never feed back into the POV.
        if (InHandle.Get<FFragment_Camera_Params>().Get_Params().Get_DriveControllerControlRotation())
        {
            if (auto* Pawn = Cast<APawn>(UCk_Utils_OwningActor_UE::Get_EntityOwningActor(InHandle));
                ck::IsValid(Pawn))
            {
                if (auto* PC = Cast<APlayerController>(Pawn->GetController());
                    ck::IsValid(PC) && PC->IsLocalController())
                {
                    PC->SetControlRotation(ViewInfo.Rotation);
                }
            }
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------
