#include "CkCamera_Processor.h"

#include "CkCamera/CkCamera_Log.h"
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
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include <GameFramework/Pawn.h>
#include <GameFramework/PlayerController.h>

#include "CkEcsExt/Transform/CkTransform_Fragment.h"
#include "CkEcsExt/Transform/CkTransform_Utils.h"

CK_REGISTER_PROCESSOR(ck::FProcessor_Camera_HandleRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_CameraLayer_Lifecycle);
CK_REGISTER_PROCESSOR(ck::FProcessor_Camera_UpdatePOV);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Constant-speed blend rate (units/sec) to traverse [0,1] in InTime. 0 time → ~instant.
    static auto DoGet_BlendRateFromTime(FCk_Time InTime) -> float
    {
        const auto Seconds = static_cast<float>(InTime.Get_Seconds());
        return Seconds > KINDA_SMALL_NUMBER ? (1.0f / Seconds) : 1000.0f;
    }

    // ================================================================================================================
    // HANDLE REQUESTS
    // ================================================================================================================

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
                DoHandleRequest(InHandle, InRequest);
            }), policy::DontResetContainer{});
        });
    }

    auto
        FProcessor_Camera_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            const FCk_Request_Camera_AddLayer& InRequest)
        -> void
    {
        const auto LayerClass = InRequest.Get_LayerClass();

        CK_ENSURE_IF_NOT(ck::IsValid(LayerClass),
            TEXT("AddLayer: invalid layer class on camera [{}]"), InHandle)
        { return; }

        const auto Priority = InRequest.Get_Priority();

        // OneOnly: blend out any existing layer at the SAME priority (eviction). Done before the new layer is
        // connected to the Record so it can't match itself.
        if (InRequest.Get_StackingBehavior() == ECk_Camera_StackingBehavior::OneOnly)
        {
            ck::FUtils_RecordOfCameraLayers::ForEach_ValidEntry(InHandle,
            [&](FCk_Handle_CameraLayer InSibling)
            {
                // The persistent base layer is never evicted (it lives at the lowest priority, so this also
                // wouldn't match, but guard explicitly).
                if (InSibling.Get<FFragment_CameraLayer_Params>().Get_IsDefault())
                { return; }

                if (InSibling.Get<FFragment_CameraLayer_Params>().Get_Priority() != Priority)
                { return; }

                auto& SiblingBlend = InSibling.Get<FFragment_CameraLayer_Blend>();
                SiblingBlend.Set_TargetAlpha(0.0f);
                SiblingBlend.Set_BlendRate(DoGet_BlendRateFromTime(InRequest.Get_BlendInTime()));
            });
        }

        // Entity creation (spawn + owning-camera back-ref + fragments + record connect + EntityScript) lives in the
        // utils Create (mirrors UCk_Utils_SmTask_UE::Create). The request-specific config (priority, blend rate,
        // look-at) is applied here.
        auto TypedLayer = ::UCk_Utils_CameraLayer_UE::Create(InHandle, LayerClass);

        if (ck::Is_NOT_Valid(TypedLayer))
        { return; }

        {
            auto& Params = TypedLayer.Get<FFragment_CameraLayer_Params>();
            Params.Set_Priority(Priority);
            Params.Set_LookAtTarget(InRequest.Get_LookAtTarget());
        }
        {
            auto& Blend = TypedLayer.Get<FFragment_CameraLayer_Blend>();
            Blend.Set_BlendRate(DoGet_BlendRateFromTime(InRequest.Get_BlendInTime()));
        }

        camera::VeryVerbose(TEXT("[Camera] AddLayer [{}] -> entity [{}] on camera [{}]"),
            LayerClass, TypedLayer, InHandle);
    }

    auto
        FProcessor_Camera_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            const FCk_Request_Camera_RemoveLayer& InRequest)
        -> void
    {
        const auto LayerClass = InRequest.Get_LayerClass();

        // Begin blend-out; the lifecycle processor prunes (destroys) the entity once its alpha reaches 0.
        ck::FUtils_RecordOfCameraLayers::ForEach_ValidEntry(InHandle,
        [&](FCk_Handle_CameraLayer InLayer)
        {
            // The persistent base layer is never removed.
            if (InLayer.Get<ck::FFragment_CameraLayer_Params>().Get_IsDefault())
            { return; }

            if (InLayer.Get<ck::FFragment_CameraLayer_Params>().Get_LayerClass() != LayerClass)
            { return; }

            auto& Blend = InLayer.Get<ck::FFragment_CameraLayer_Blend>();
            Blend.Set_TargetAlpha(0.0f);
            Blend.Set_BlendRate(DoGet_BlendRateFromTime(InRequest.Get_BlendOutTime()));
        });
    }

    // ================================================================================================================
    // LAYER LIFECYCLE
    // ================================================================================================================

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

            // Fully blended out → prune the layer (deferred destroy). The layer's acquired attribute modifiers are
            // removed in UCk_CameraLayer_EntityScript::ExitLayer (driven by EndPlay), so they don't outlive it.
            // The persistent base layer is never pruned (it stays pinned at full blend).
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

        // Refresh the composed-profile cache from the (already-recomputed-this-frame) tuner attributes.
        InCurrent._ComposedProfile = ::UCk_Utils_Camera_UE::Get_Profile(InHandle);

        // Resolve the dominant layer (its class is observable via Utils) + its look-at target (for auto-reorient).
        InCurrent._DominantLookAt.Reset();
        InCurrent._DominantLayerClass = nullptr;
        if (ck::IsValid(Dominant))
        {
            const auto& DominantParams = Dominant.Get<FFragment_CameraLayer_Params>();
            InCurrent._DominantLayerClass = DominantParams.Get_LayerClass();

            if (const auto& LookAt = DominantParams.Get_LookAtTarget();
                ck::IsValid(LookAt))
            {
                InCurrent._DominantLookAt = UCk_Utils_Transform_UE::Get_EntityCurrentLocation(LookAt);
            }
        }
    }

    // ================================================================================================================
    // UPDATE POV
    // ================================================================================================================

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

        ck::camera::FPov::Run(Profile, Input, InCurrent._PovState);

        // The orientation intention is a per-frame DELTA — consume it once applied. Input (OnLook) only fires while
        // the mouse is moving, so without this the last delta would keep being re-applied every frame after the mouse
        // stops, drifting the camera. While the mouse is moving, input re-pushes a fresh intention each frame.
        InCurrent._OrientationIntention = FVector::ZeroVector;

        auto ViewInfo = FMinimalViewInfo{};
        ViewInfo.Location    = InCurrent._PovState._CameraTransform.GetLocation();
        ViewInfo.Rotation    = InCurrent._PovState._CameraTransform.Rotator();
        ViewInfo.FOV         = Sensor.Get_FOV();
        ViewInfo.DesiredFOV  = Sensor.Get_FOV();
        if (Sensor.Get_ConstrainAspectRatio())
        {
            ViewInfo.bConstrainAspectRatio = true;
            ViewInfo.AspectRatio          = Sensor.Get_AspectRatio();
        }

        InCurrent._ViewInfo = ViewInfo;

        // Camera-authoritative control rotation: when opted in (player view), publish the resolved view rotation to
        // the local PlayerController so control-rotation consumers (facing / aim / movement) follow the camera. This
        // replaces the per-frame SM task BusterBlock used to run. Guarded to the local controller (the director only
        // exists on the local client anyway). Does NOT feed back into the POV — the camera reads input intention, not
        // control rotation.
        if (InHandle.Get<FFragment_Camera_Params>().Get_Params().Get_DriveControllerControlRotation())
        {
            if (auto* Pawn = Cast<APawn>(UCk_Utils_OwningActor_UE::Get_EntityOwningActor(InHandle));
                ck::IsValid(Pawn, ck::IsValid_Policy_NullptrOnly{}))
            {
                if (auto* PC = Cast<APlayerController>(Pawn->GetController());
                    ck::IsValid(PC, ck::IsValid_Policy_NullptrOnly{}) && PC->IsLocalController())
                {
                    PC->SetControlRotation(ViewInfo.Rotation);
                }
            }
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------
