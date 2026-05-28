#include "CkGameplayCamera_Processor.h"

#include "CkCamera/CkCamera_Log.h"
#include "CkCamera/GameplayCamera/CameraModifier/CkCameraModifier_EntityScript.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/EntityScript/CkEntityScript_Utils.h"
#include "CkEcs/Handle/CkHandle_Utils.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkEcsExt/Transform/CkTransform_Fragment.h"

CK_REGISTER_PROCESSOR(ck::FProcessor_GameplayCamera_HandleRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_GameplayCamera_ComposeProfile);
CK_REGISTER_PROCESSOR(ck::FProcessor_GameplayCamera_UpdatePOV);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
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

        auto MutableDirector = InHandle;

        auto NewModifier = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(MutableDirector);
        UCk_Utils_Handle_UE::Set_DebugName(NewModifier, ModifierClass->GetFName());

        ck::TUtils_CameraModifier_OwningCamera::AddOrReplace(NewModifier, InHandle);
        NewModifier.Add<ck::FFragment_CameraModifier_Params>(ModifierClass);
        NewModifier.Add<ck::FFragment_CameraModifier_Blend>();

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

        ck::FUtils_RecordOfCameraModifiers::ForEach_ValidEntry(MutableDirector,
        [&](FCk_Handle_CameraModifier InModifier)
        {
            if (NOT InModifier.Has<ck::FFragment_CameraModifier_Params>())
            { return; }

            if (InModifier.Get<ck::FFragment_CameraModifier_Params>().Get_ModifierClass() != ModifierClass)
            { return; }

            InModifier.AddOrGet<ck::FTag_CameraModifier_PendingExit>();

            auto ModifierToDestroy = InModifier;
            UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(ModifierToDestroy);
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
        // M0: reset to a default profile each frame. M1 walks FFragment_RecordOfCameraModifiers and
        // dispatches DoContributeToProfile on each active modifier's script, weighted by its blend alpha.
        InCurrent._ComposedProfile = FCk_GameplayCamera_Profile{};
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

        auto ViewInfo = FMinimalViewInfo{};
        ViewInfo.FOV = Profile.Get_FOV();

        if (InHandle.Has<ck::FFragment_Transform>())
        {
            const auto Xf       = InHandle.Get<ck::FFragment_Transform>().Get_Transform();
            const auto PivotLoc = Xf.GetLocation();
            const auto Rot      = Xf.Rotator();
            const auto BackDir  = Rot.Vector();

            // M0: trivial third-person framing. M1 replaces this with the full POV pipeline
            // (attach -> lookat -> boom -> framing -> transform -> collision -> noise -> DoF).
            ViewInfo.Location = PivotLoc - (BackDir * Profile.Get_BoomArmLength()) + Profile.Get_FramingOffset();
            ViewInfo.Rotation = Rot;
        }

        InCurrent._ViewInfo = ViewInfo;
    }
}

// --------------------------------------------------------------------------------------------------------------------
