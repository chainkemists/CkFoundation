#include "CkCrowdAgent_Processor.h"

#include "CkCrowd/CkCrowd_Log.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkEcsExt/SceneNode/CkSceneNode_Utils.h"
#include "CkEcsExt/Transform/CkTransform_Utils.h"

#include "CkShapes/Cylinder/CkShapeCylinder_Utils.h"

#include "CkSpatialQuery/Probe/CkProbe_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_CrowdAgent_Setup);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_CrowdAgent_Setup::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_CrowdAgent_Params& InParams,
            const FFragment_Transform& InTransform,
            FFragment_CrowdAgent_ProbeRef& InProbeRef) const
        -> void
    {
        const auto Radius = InParams.Get_Radius();
        const auto Height = InParams.Get_Height();
        const auto HalfHeight = Height * 0.5f;
        const auto ProbeRadius = Radius + InParams.Get_SeparationLookahead();

        ck::crowd::VeryVerbose(TEXT("CrowdAgent setup: [{}] (radius={}, height={}, probe_radius={})"),
            InHandle, Radius, Height, ProbeRadius);

        // ---- Spawn the probe child entity ----
        // Owned by the agent so it cascade-destroys with the agent. The probe child carries its own
        // Transform (offset Z=+HalfHeight so the cylinder is centered on the agent's center, not its
        // feet); SceneNode-parented to the agent so apply-offset moves propagate without per-tick sync.
        // Mirror the Steering pattern: take an explicit non-const copy of InHandle for mutating ops
        // (Add/Try_Remove tag stamps below). The framework hands InHandle in a way the per-call
        // Steering body treats as const-by-convention; explicit copy keeps that convention.
        auto AgentNonConst = InHandle;

        auto ProbeChildEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(AgentNonConst);

        // Initial transform = agent's location + Z offset (HalfHeight). SceneNode propagation will
        // overwrite this each tick from the agent's current Transform; this is just so the first
        // frame before SceneNode runs has a sensible position.
        const auto& AgentXform = InTransform.Get_Transform();
        const auto InitialChildXform = FTransform{
            AgentXform.GetRotation(),
            AgentXform.GetLocation() + FVector{0.0, 0.0, HalfHeight},
            FVector::OneVector};

        auto ProbeChildTransform = UCk_Utils_Transform_UE::Add(
            ProbeChildEntity, InitialChildXform, ECk_Replication::DoesNotReplicate);

        // Cylinder shape — the probe asks Jolt for overlaps within this volume. ConvexRadius=0 is fine
        // for an agent-sized cylinder; the +SeparationLookahead reach gives the steering layer time to
        // start nudging before agents are physically touching.
        const auto CylinderDimensions = FCk_ShapeCylinder_Dimensions{HalfHeight, ProbeRadius};
        const auto CylinderParams = FCk_Fragment_ShapeCylinder_ParamsData{CylinderDimensions};
        UCk_Utils_ShapeCylinder_UE::Add(ProbeChildEntity, CylinderParams);

        // Probe filter: name AND filter both set to TAG_Crowd_Agent so any two crowd probes mutually
        // overlap. ContextOverlapPolicy=Any so probes from any context owner can match (different
        // gym stations, different lifetime owners). MotionType=Kinematic so Jolt allows the probe to
        // move every frame without the static-not-moved ensure firing.
        auto ProbeParams = FCk_Fragment_Probe_ParamsData{TAG_Crowd_Agent};
        ProbeParams.Set_Filter(FGameplayTagContainer{TAG_Crowd_Agent});
        ProbeParams.Set_ContextOverlapPolicy(ECk_Probe_ContextOverlapPolicy::Any);
        ProbeParams.Set_MotionType(ECk_MotionType::Kinematic);

        auto ProbeHandle = UCk_Utils_Probe_UE::Add(ProbeChildTransform, ProbeParams, FCk_Probe_DebugInfo{});

        // SceneNode-parent the probe child Transform to the agent's Transform so apply-offset moves
        // propagate. Local offset = +HalfHeight on Z; agent Transform is at FEET, probe shape is
        // centered, so this puts the cylinder's mid-height at agent.center.
        auto AgentTransform = UCk_Utils_Transform_UE::Cast(InHandle);
        const auto LocalOffset = FTransform{
            FRotator::ZeroRotator,
            FVector{0.0, 0.0, HalfHeight},
            FVector::OneVector};
        UCk_Utils_SceneNode_UE::Add(ProbeChildTransform, AgentTransform, LocalOffset);

        // Store probe handle for the NeighborSync processor's per-frame lookup.
        InProbeRef._ProbeChild = ProbeHandle;

        // Stamp HasProbe (excludes this view next tick) + clear NeedsSetup (other Setup-marked
        // processors may also depend on this tag; remove only after our work is done).
        AgentNonConst.Add<FTag_CrowdAgent_HasProbe>();
        AgentNonConst.Try_Remove<FTag_CrowdAgent_NeedsSetup>();
    }
}

// --------------------------------------------------------------------------------------------------------------------
