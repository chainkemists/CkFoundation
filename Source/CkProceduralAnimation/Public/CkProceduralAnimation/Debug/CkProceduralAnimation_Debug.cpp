#include "CkProceduralAnimation/Debug/CkProceduralAnimation_Debug.h"

#include "CkProceduralAnimation/BodyPose/CkProceduralBodyPose_Fragment.h"
#include "CkProceduralAnimation/BodyPose/CkProceduralBodyPose_Utils.h"
#include "CkProceduralAnimation/Gait/CkProceduralGait_Fragment.h"
#include "CkProceduralAnimation/Gait/CkProceduralGait_Utils.h"
#include "CkProceduralAnimation/Leg/CkProceduralLeg_Fragment.h"
#include "CkProceduralAnimation/Rig/CkProceduralRig_Fragment.h"
#include "CkProceduralAnimation/Rig/CkProceduralRig_Utils.h"
#include "CkProceduralAnimation/SurfaceMotion/CkSurfaceMotion_Fragment.h"
#include "CkProceduralAnimation/SurfaceMotion/CkSurfaceMotion_Utils.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Ensure/CkEnsure.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Subsystem/CkEcsWorld_Subsystem.h"

#include "CkEcsExt/Transform/CkTransform_Fragment.h"

#include <Engine/World.h>

// --------------------------------------------------------------------------------------------------------------------

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
        Get_HasGait(
            const FCk_Handle& InHandle)
        -> bool
    {
        return Get_IsActive(InHandle)
            && InHandle.Has<ck::FFragment_ProceduralGait>()
            && InHandle.Has<ck::FFragment_ProceduralGait_Tunables>()
            && InHandle.Has<ck::FFragment_ProceduralGait_Debug>()
            && InHandle.Has<ck::FFragment_Transform>();
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

    auto
        Get_Part(
            const FCk_Handle& InPart,
            bool& OutPending)
        -> FCk_ProceduralAnimation_DebugPart
    {
        auto Part = FCk_ProceduralAnimation_DebugPart{};
        auto Pose = FTransform::Identity;
        if (TryGet_PartTransform(InPart, Pose, OutPending))
        { Part.Set_Available(true).Set_Transform(Pose); }

        return Part;
    }

    auto
        DoAggregate_Status(
            ECk_ProceduralAnimation_Status InAggregate,
            ECk_ProceduralAnimation_Status InRig)
        -> ECk_ProceduralAnimation_Status
    {
        if (InAggregate == ECk_ProceduralAnimation_Status::Failed || InRig == ECk_ProceduralAnimation_Status::Failed)
        { return ECk_ProceduralAnimation_Status::Failed; }

        if (InAggregate == ECk_ProceduralAnimation_Status::PendingSetup || InRig == ECk_ProceduralAnimation_Status::PendingSetup)
        { return ECk_ProceduralAnimation_Status::PendingSetup; }

        return ECk_ProceduralAnimation_Status::Ready;
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

    if (NOT ck_procedural_animation_debug::Get_IsActive(InHandle))
    { return {}; }

    auto Body = InHandle;
    if (NOT ck_procedural_animation_debug::Get_HasGait(Body) && Body.Has<ck::FFragment_LifetimeOwner>())
    { Body = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InHandle); }

    if (NOT ck_procedural_animation_debug::Get_HasGait(Body))
    { return {}; }

    const auto& Gait = Body.Get<ck::FFragment_ProceduralGait>();
    const auto GaitHandle = UCk_Utils_ProceduralGait_UE::CastChecked(Body);
    auto Snapshot = Body.Get<ck::FFragment_ProceduralGait_Debug>()._Snapshot;
    Snapshot.Set_EntityName(Body.Get_DebugName())
        .Set_EntityId(Body.Get_Entity().ToString());
    Snapshot.Get_Status().Set_Available(true)
        .Set_GaitStatus(UCk_Utils_ProceduralGait_UE::Get_Status(GaitHandle))
        .Set_GaitFailure(UCk_Utils_ProceduralGait_UE::Get_Failure(GaitHandle));
    Snapshot.Get_Freshness().Set_GaitFresh(Snapshot.Get_Status().Get_HasAcceptedSample()
        && Snapshot.Get_Sample().Get_FrameNumber() == GFrameCounter);

    const auto HasMotion = Body.Has<ck::FFragment_SurfaceMotion>()
        && Body.Has<ck::FFragment_SurfaceMotion_Support>()
        && Body.Has<ck::FFragment_SurfaceMotion_Params>();
    Snapshot.Get_Status().Set_HasSurfaceMotion(HasMotion);
    if (HasMotion)
    {
        const auto MotionHandle = UCk_Utils_SurfaceMotion_UE::CastChecked(Body);
        Snapshot.Get_Freshness().Set_MotionMatchesGaitFrame(Snapshot.Get_Status().Get_HasAcceptedSample()
            && UCk_Utils_SurfaceMotion_UE::Get_Status(MotionHandle) == ECk_ProceduralAnimation_Status::Ready
            && Body.Get<ck::FFragment_SurfaceMotion_Support>()._EvaluatedFrame == Snapshot.Get_Sample().Get_FrameNumber());
    }

    if (UCk_Utils_ProceduralBodyPose_UE::Has(Body))
    {
        Snapshot.Get_BodyPose().Set_Composed(true)
            .Set_Status(UCk_Utils_ProceduralBodyPose_UE::Get_Status(UCk_Utils_ProceduralBodyPose_UE::CastChecked(Body)))
            .Set_Offset(Body.Get<ck::FFragment_ProceduralBodyPose>()._Offset)
            .Set_Conforms(Body.Has<ck::FFragment_ProceduralBodyPose_ConformState>());
        if (Body.Has<ck::FFragment_ProceduralBodyPose_ConformState>())
        {
            const auto& ConformState = Body.Get<ck::FFragment_ProceduralBodyPose_ConformState>();
            Snapshot.Get_BodyPose().Set_ConformTarget(ConformState.Get_HeldTarget())
                .Set_AppliedConformTarget(ConformState.Get_AppliedTarget());
        }
    }

    auto HasRig = false;
    auto RigStatus = ECk_ProceduralAnimation_Status::Ready;
    auto RigMatchesGaitSequence = Snapshot.Get_Status().Get_HasAcceptedSample();
    auto RigFailure = ECk_ProceduralRig_Failure::None;
    auto Pending = false;
    for (auto Index = 0; Index < Gait._Legs.Num(); ++Index)
    {
        const auto& Leg = Gait._Legs[Index];
        if (NOT Snapshot.Get_Legs().IsValidIndex(Index) || NOT ck_procedural_animation_debug::Get_IsActive(Leg))
        { continue; }

        auto& DebugLeg = Snapshot.Get_Legs()[Index];
        DebugLeg.Set_LegEntityId(Leg.Get_Entity().ToString());

        if (NOT UCk_Utils_ProceduralRig_UE::Has(Leg))
        { continue; }

        const auto RigHandle = UCk_Utils_ProceduralRig_UE::CastChecked(Leg);
        const auto& Rig = Leg.Get<ck::FFragment_ProceduralRig>();
        const auto& Chain = Leg.Get<ck::FFragment_ProceduralRig_Params>();
        const auto LegRigStatus = UCk_Utils_ProceduralRig_UE::Get_Status(RigHandle);
        const auto LegRigFailure = UCk_Utils_ProceduralRig_UE::Get_Failure(RigHandle);
        HasRig = true;
        RigStatus = ck_procedural_animation_debug::DoAggregate_Status(RigStatus, LegRigStatus);
        RigMatchesGaitSequence &= Rig._PosedSolveSequence == Snapshot.Get_Sample().Get_Sequence();
        if (RigFailure == ECk_ProceduralRig_Failure::None)
        { RigFailure = LegRigFailure; }

        const auto Segments = ck::algo::Transform<TArray<FCk_ProceduralAnimation_DebugPart>>(Chain.Get_Segments(),
        [&](const FCk_Handle_Transform& InSegment)
        {
            return ck_procedural_animation_debug::Get_Part(InSegment, Pending);
        });

        DebugLeg.Get_Rig().Set_Composed(true)
            .Set_Status(LegRigStatus)
            .Set_Failure(LegRigFailure)
            .Set_Segments(Segments)
            .Set_Foot(ck_procedural_animation_debug::Get_Part(Chain.Get_Foot(), Pending));
    }

    Snapshot.Get_Status().Set_HasRig(HasRig)
        .Set_RigStatus(HasRig ? RigStatus : ECk_ProceduralAnimation_Status::PendingSetup)
        .Set_RigFailure(RigFailure);
    Snapshot.Get_Freshness().Set_RigMatchesGaitSequence(HasRig && RigMatchesGaitSequence)
        .Set_RigPosePending(Pending);

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

    if (ck::Is_NOT_Valid(InWorld) || NOT InWorld->HasBegunPlay())
    { return Entities; }

    auto* Subsystem = InWorld->GetSubsystem<UCk_EcsWorld_Subsystem_UE>();
    if (ck::Is_NOT_Valid(Subsystem))
    { return Entities; }

    const auto TransientEntity = Subsystem->Get_TransientEntity();
    if (ck::Is_NOT_Valid(TransientEntity))
    { return Entities; }

    auto Registry = Subsystem->Get_Registry();
    Registry.View<ck::FFragment_ProceduralGait>().ForEach(
    [&](FCk_Entity InEntity, const ck::FFragment_ProceduralGait& InGaitComp)
    {
        auto Handle = ck::MakeHandle(InEntity, TransientEntity);
        if (ck_procedural_animation_debug::Get_HasGait(Handle))
        { Entities.Add(Handle); }
    });

    return Entities;
}

// --------------------------------------------------------------------------------------------------------------------
