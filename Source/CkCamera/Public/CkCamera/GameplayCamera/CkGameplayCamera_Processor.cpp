#include "CkGameplayCamera_Processor.h"

#include "CkCamera/CkCamera_Log.h"
#include "CkCamera/GameplayCamera/CameraModifier/CkCameraModifier_EntityScript.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/EntityScript/CkEntityScript_Fragment.h"
#include "CkEcs/EntityScript/CkEntityScript_Utils.h"
#include "CkEcs/Handle/CkHandle_Utils.h"
#include "CkEcs/OwningActor/CkOwningActor_Utils.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkEcsExt/Transform/CkTransform_Fragment.h"

CK_REGISTER_PROCESSOR(ck::FProcessor_GameplayCamera_HandleRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_GameplayCamera_ComposeProfile);
CK_REGISTER_PROCESSOR(ck::FProcessor_GameplayCamera_UpdatePOV);

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
        FProcessor_GameplayCamera_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_GameplayCamera_Requests& InRequestsComp) const
        -> void
    {
        InHandle.CopyAndRemove(InRequestsComp, [&](const FFragment_GameplayCamera_Requests& InRequests)
        {
            ck::algo::ForEachRequest(InRequests._Requests, ck::Visitor(
            [&](const auto& InRequest) -> void
            {
                DoHandleRequest(InHandle, InRequest);
            }), policy::DontResetContainer{});
        });
    }

    auto
        FProcessor_GameplayCamera_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            const FCk_Request_GameplayCamera_AddModifier& InRequest) const
        -> void
    {
        const auto ModifierClass = InRequest.Get_ModifierClass();

        CK_ENSURE_IF_NOT(ck::IsValid(ModifierClass),
            TEXT("AddModifier: invalid modifier class on camera [{}]"), InHandle)
        { return; }

        const auto OrderingGroup = InRequest.Get_OrderingGroup();

        // OneOnly: blend out any existing modifier in the same ordering group (eviction). Done before the new
        // modifier is connected to the Record so it can't match itself.
        if (InRequest.Get_StackingBehavior() == ECk_GameplayCamera_StackingBehavior::OneOnly)
        {
            auto MutableForEvict = InHandle;
            ck::FUtils_RecordOfCameraModifiers::ForEach_ValidEntry(MutableForEvict,
            [&](FCk_Handle_CameraModifier InSibling)
            {
                if (NOT InSibling.Has<ck::FFragment_CameraModifier_Params>())
                { return; }
                if (InSibling.Get<ck::FFragment_CameraModifier_Params>().Get_OrderingGroup() != OrderingGroup)
                { return; }
                if (NOT InSibling.Has<ck::FFragment_CameraModifier_Blend>())
                { return; }

                auto& SiblingBlend = InSibling.Get<ck::FFragment_CameraModifier_Blend>();
                SiblingBlend.Set_TargetAlpha(0.0f);
                SiblingBlend.Set_BlendRate(DoGet_BlendRateFromTime(InRequest.Get_BlendInTime()));
                InSibling.AddOrGet<ck::FTag_CameraModifier_PendingExit>();
            });
        }

        auto MutableDirector = InHandle;

        auto NewModifier = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(MutableDirector);
        UCk_Utils_Handle_UE::Set_DebugName(NewModifier, ModifierClass->GetFName());

        ck::TUtils_CameraModifier_OwningCamera::AddOrReplace(NewModifier, InHandle);

        NewModifier.Add<ck::FFragment_CameraModifier_Params>(ModifierClass);
        {
            auto& Params = NewModifier.Get<ck::FFragment_CameraModifier_Params>();
            Params.Set_OrderingGroup(OrderingGroup);
            Params.Set_LookAtTarget(InRequest.Get_LookAtTarget());
        }

        NewModifier.Add<ck::FFragment_CameraModifier_Blend>();
        {
            auto& Blend = NewModifier.Get<ck::FFragment_CameraModifier_Blend>();
            Blend.Set_Alpha(0.0f);
            Blend.Set_TargetAlpha(1.0f);
            Blend.Set_BlendRate(DoGet_BlendRateFromTime(InRequest.Get_BlendInTime()));
        }

        auto TypedModifier = ck::StaticCast<FCk_Handle_CameraModifier>(NewModifier);

        ck::FUtils_RecordOfCameraModifiers::AddIfMissing(MutableDirector);
        ck::FUtils_RecordOfCameraModifiers::Request_Connect(
            MutableDirector, TypedModifier, ECk_Record_LabelRequirementPolicy::Optional);

        // Attach the modifier EntityScript synchronously (after the back-ref/params/record are in place, so
        // Construct sees them) — mirrors UCk_Utils_SmState_UE::Create. See the note in the fragment header.
        UCk_Utils_EntityScript_UE::Add(NewModifier, ModifierClass, FInstancedStruct{});

        camera::VeryVerbose(TEXT("[GameplayCamera] AddModifier [{}] -> entity [{}] on camera [{}]"),
            ModifierClass, TypedModifier, InHandle);
    }

    auto
        FProcessor_GameplayCamera_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            const FCk_Request_GameplayCamera_RemoveModifier& InRequest) const
        -> void
    {
        const auto ModifierClass = InRequest.Get_ModifierClass();
        auto MutableDirector = InHandle;

        // Begin blend-out; ComposeProfile prunes (destroys) the entity once its alpha reaches 0.
        ck::FUtils_RecordOfCameraModifiers::ForEach_ValidEntry(MutableDirector,
        [&](FCk_Handle_CameraModifier InModifier)
        {
            if (NOT InModifier.Has<ck::FFragment_CameraModifier_Params>())
            { return; }

            if (InModifier.Get<ck::FFragment_CameraModifier_Params>().Get_ModifierClass() != ModifierClass)
            { return; }

            if (NOT InModifier.Has<ck::FFragment_CameraModifier_Blend>())
            { return; }

            auto& Blend = InModifier.Get<ck::FFragment_CameraModifier_Blend>();
            Blend.Set_TargetAlpha(0.0f);
            Blend.Set_BlendRate(DoGet_BlendRateFromTime(InRequest.Get_BlendOutTime()));
            InModifier.AddOrGet<ck::FTag_CameraModifier_PendingExit>();
        });
    }

    // ================================================================================================================
    // COMPOSE PROFILE
    // ================================================================================================================

    auto
        FProcessor_GameplayCamera_ComposeProfile::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_GameplayCamera_Current& InCurrent) const
        -> void
    {
        // Start from a fresh default each frame; every active modifier contributes on top, weighted by its own
        // blend alpha. Per-modifier blend is ticked here (runs every frame); fully-blended-out modifiers are
        // pruned, and the dominant active modifier (highest alpha, later record order breaks ties) supplies the
        // look-at target for auto-reorient.
        auto Profile = FCk_GameplayCamera_Profile{};

        const auto DeltaSeconds = static_cast<float>(InDeltaT.Get_Seconds());

        auto Dominant      = FCk_Handle_CameraModifier{};
        auto DominantAlpha = -1.0f;

        FUtils_RecordOfCameraModifiers::ForEach_ValidEntry(InHandle,
        [&](FCk_Handle_CameraModifier InModifier)
        {
            if (NOT InModifier.Has<FFragment_CameraModifier_Blend>())
            { return; }

            auto& Blend = InModifier.Get<FFragment_CameraModifier_Blend>();
            Blend.Set_Alpha(FMath::FInterpConstantTo(
                Blend.Get_Alpha(), Blend.Get_TargetAlpha(), DeltaSeconds, Blend.Get_BlendRate()));

            const auto Alpha = Blend.Get_Alpha();

            // Fully blended out → prune (deferred destroy).
            if (Alpha <= 0.0f && Blend.Get_TargetAlpha() <= 0.0f)
            {
                auto ModifierToDestroy = InModifier;
                UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(ModifierToDestroy);
                return;
            }

            if (Alpha <= 0.0f)
            { return; }

            if (NOT InModifier.Has<FTag_CameraModifier_Active>())
            { return; }

            if (NOT InModifier.Has<FFragment_EntityScript_Current>())
            { return; }

            auto* Script = Cast<UCk_CameraModifier_EntityScript>(
                InModifier.Get<FFragment_EntityScript_Current>().Get_Script().Get());

            if (ck::Is_NOT_Valid(Script))
            { return; }

            if (Script->Get_TickMode() == ECk_GameplayCamera_TickMode::Tick)
            {
                Script->Tick(InModifier, InDeltaT);
            }

            Script->ContributeToProfile(InModifier, Profile, Alpha);

            if (Alpha >= DominantAlpha)
            {
                DominantAlpha = Alpha;
                Dominant      = InModifier;
            }
        });

        InCurrent._ComposedProfile = Profile;

        // Resolve the dominant modifier (its class is observable via Utils) + its look-at target (for auto-reorient).
        InCurrent._DominantLookAt.Reset();
        InCurrent._DominantModifierClass = nullptr;
        if (ck::IsValid(Dominant) && Dominant.Has<FFragment_CameraModifier_Params>())
        {
            const auto& DominantParams = Dominant.Get<FFragment_CameraModifier_Params>();
            InCurrent._DominantModifierClass = DominantParams.Get_ModifierClass();

            const auto LookAt = DominantParams.Get_LookAtTarget();
            if (ck::IsValid(LookAt) && LookAt.Has<ck::FFragment_Transform>())
            {
                InCurrent._DominantLookAt = LookAt.Get<ck::FFragment_Transform>().Get_Transform().GetLocation();
            }
        }
    }

    // ================================================================================================================
    // UPDATE POV
    // ================================================================================================================

    auto
        FProcessor_GameplayCamera_UpdatePOV::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_GameplayCamera_Current& InCurrent) const
        -> void
    {
        const auto& Profile = InCurrent.Get_ComposedProfile();
        const auto& Sensor  = Profile.Get_Sensor();

        // No anchor transform yet (e.g. director on an entity without a transform) — keep the last POV.
        if (NOT InHandle.Has<ck::FFragment_Transform>())
        { return; }

        auto Input = ck::gameplaycamera::FPov_Input{};
        Input._AnchorTransform      = InHandle.Get<ck::FFragment_Transform>().Get_Transform();
        Input._OrientationIntention = InCurrent.Get_OrientationIntention();
        Input._DeltaSeconds         = static_cast<float>(InDeltaT.Get_Seconds());
        Input._LookAtLocation       = InCurrent._DominantLookAt;
        Input._World                = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);
        Input._TraceIgnoreActor     = UCk_Utils_OwningActor_UE::Get_EntityOwningActor(InHandle);

        ck::gameplaycamera::FPov::Run(Profile, Input, InCurrent._PovState);

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
    }
}

// --------------------------------------------------------------------------------------------------------------------
