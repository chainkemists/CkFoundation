#include "CkProceduralAnimation/Debug/CkProceduralAnimation_Debug.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Subsystem/CkEcsWorld_Subsystem.h"
#include "CkEcsExt/Transform/CkTransform_Fragment.h"
#include "CkProceduralAnimation/Gait/CkProceduralGait_Fragment.h"
#include "CkProceduralAnimation/Rig/CkProceduralRig_Fragment.h"
#include "CkProceduralAnimation/SurfaceMotion/CkSurfaceMotion_Fragment.h"
#include <Engine/World.h>

namespace ck_procedural_animation_debug
{
    auto
        Get_IsActive(
            const FCk_Handle& InHandle)
        -> bool
    {
        return ck::IsValid(InHandle) && NOT InHandle.Has<ck::FTag_DestroyEntity_Initiate>();
    }

    auto
        Get_HasPendingTransform(
            const FCk_Handle& InHandle)
        -> bool
    {
        if (NOT InHandle.Has<ck::FFragment_Transform_Requests>())
        { return false; }
        const auto& Requests = InHandle.Get<ck::FFragment_Transform_Requests>();
        return NOT Requests.Get_LocationRequests().IsEmpty() || NOT Requests.Get_RotationRequests().IsEmpty()
            || Requests.Get_ScaleRequests().IsSet() || NOT Requests.Get_ForceRefreshRequests().IsEmpty();
    }

    auto
        TryGet_PartTransform(
            const FCk_Handle& InHandle,
            FTransform& OutTransform,
            bool& OutPending)
        -> bool
    {
        if (NOT Get_IsActive(InHandle) || NOT InHandle.Has<ck::FFragment_Transform>())
        { return false; }
        OutTransform = InHandle.Get<ck::FFragment_Transform>().Get_Transform();
        OutPending |= Get_HasPendingTransform(InHandle);
        return true;
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ProceduralAnimation_Debug_UE::
    Get_Snapshot(
        const FCk_Handle& InHandle)
    -> FCk_ProceduralAnimation_DebugSnapshot
{
    const auto OnGameThread = IsInGameThread();
    CK_ENSURE_IF_NOT(OnGameThread, TEXT("Procedural animation diagnostics must be copied on the game thread."))
    { return {}; }
    if (NOT OnGameThread || NOT ck_procedural_animation_debug::Get_IsActive(InHandle)
        || NOT InHandle.Has<ck::FFragment_ProceduralGait_Current>()
        || NOT InHandle.Has<ck::FFragment_ProceduralGait_Params>()
        || NOT InHandle.Has<ck::FFragment_Transform>())
    { return {}; }

    const auto& Gait = InHandle.Get<ck::FFragment_ProceduralGait_Current>();
    auto Snapshot = Gait._DebugSnapshot;
    Snapshot.Set_EntityName(InHandle.Get_DebugName()).Set_EntityId(InHandle.Get_Entity().ToString()).Set_Available(true)
        .Set_GaitReady(Gait._Ready).Set_GaitFailed(Gait._Failed)
        .Set_GaitFresh(Snapshot.Get_HasAcceptedSample() && Snapshot.Get_FrameNumber() == GFrameCounter);

    const auto HasMotion = InHandle.Has<ck::FFragment_SurfaceMotion_Current>()
        && InHandle.Has<ck::FFragment_SurfaceMotion_Params>();
    Snapshot.Set_HasSurfaceMotion(HasMotion);
    if (HasMotion)
    {
        const auto& Motion = InHandle.Get<ck::FFragment_SurfaceMotion_Current>();
        Snapshot.Set_MotionMatchesGaitFrame(Snapshot.Get_HasAcceptedSample()
            && Motion._Ready && Motion._DebugFrameNumber == Snapshot.Get_FrameNumber());
    }

    const auto HasRig = InHandle.Has<ck::FFragment_ProceduralRig_Current>()
        && InHandle.Has<ck::FFragment_ProceduralRig_Params>();
    Snapshot.Set_HasRig(HasRig);
    if (NOT HasRig)
    { return Snapshot; }
    const auto& Rig = InHandle.Get<ck::FFragment_ProceduralRig_Current>();
    const auto& RigParams = InHandle.Get<ck::FFragment_ProceduralRig_Params>();
    Snapshot.Set_RigReady(Rig._Ready).Set_RigFailure(Rig._Failure)
        .Set_RigMatchesGaitSequence(Snapshot.Get_HasAcceptedSample()
            && Rig._DebugGaitSequence == Snapshot.Get_Sequence());
    auto Pending = false;
    for (auto Index = 0; Index < RigParams.Get_Legs().Num(); ++Index)
    {
        if (NOT Rig._GaitLegIndices.IsValidIndex(Index))
        { continue; }
        const auto GaitIndex = Rig._GaitLegIndices[Index];
        if (NOT Snapshot.Get_Legs().IsValidIndex(GaitIndex))
        { continue; }
        const auto& Params = RigParams.Get_Legs()[Index];
        auto& Leg = Snapshot.Get_Legs()[GaitIndex];
        Leg.Set_HasRig(true);
        auto Pose = FTransform::Identity;
        if (ck_procedural_animation_debug::TryGet_PartTransform(Params.Get_Upper(), Pose, Pending))
        { Leg.Set_UpperAvailable(true).Set_UpperTransform(Pose); }
        if (ck_procedural_animation_debug::TryGet_PartTransform(Params.Get_Lower(), Pose, Pending))
        { Leg.Set_LowerAvailable(true).Set_LowerTransform(Pose); }
        if (ck_procedural_animation_debug::TryGet_PartTransform(Params.Get_Foot(), Pose, Pending))
        { Leg.Set_FootAvailable(true).Set_FootTransform(Pose); }
    }
    Snapshot.Set_RigPosePending(Pending);
    return Snapshot;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ProceduralAnimation_Debug_UE::
    Get_Entities(
        UWorld* InWorld)
    -> TArray<FCk_Handle>
{
    auto Entities = TArray<FCk_Handle>{};
    const auto OnGameThread = IsInGameThread();
    CK_ENSURE_IF_NOT(OnGameThread, TEXT("Procedural animation discovery must run on the game thread."))
    { return Entities; }
    if (NOT OnGameThread || ck::Is_NOT_Valid(InWorld) || NOT InWorld->HasBegunPlay())
    { return Entities; }
    auto* Subsystem = InWorld->GetSubsystem<UCk_EcsWorld_Subsystem_UE>();
    if (ck::Is_NOT_Valid(Subsystem))
    { return Entities; }
    const auto TransientEntity = Subsystem->Get_TransientEntity();
    if (ck::Is_NOT_Valid(TransientEntity))
    { return Entities; }
    auto Registry = Subsystem->Get_Registry();
    Registry.View<ck::FFragment_ProceduralGait_Current>().ForEach(
        [&](FCk_Entity InEntity, const ck::FFragment_ProceduralGait_Current& InCurrent)
        {
            auto Handle = ck::MakeHandle(InEntity, TransientEntity);
            if (ck_procedural_animation_debug::Get_IsActive(Handle)
                && Handle.Has<ck::FFragment_ProceduralGait_Params>() && Handle.Has<ck::FFragment_Transform>())
            { Entities.Add(Handle); }
        });
    return Entities;
}
