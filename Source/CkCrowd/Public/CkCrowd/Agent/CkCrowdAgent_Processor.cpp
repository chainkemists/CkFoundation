#include "CkCrowdAgent_Processor.h"

#include "CkCrowd/CkCrowd_Log.h"
#include "CkCrowd/CkCrowd_Stats.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkEcsExt/SceneNode/CkSceneNode_Utils.h"
#include "CkEcsExt/Transform/CkTransform_Utils.h"

#include "CkShapes/Cylinder/CkShapeCylinder_Utils.h"

#include "CkSpatialQuery/Probe/CkProbe_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_CrowdAgent_Setup);

// --------------------------------------------------------------------------------------------------------------------

DECLARE_CYCLE_STAT(TEXT("Crowd::Setup"), STAT_CkCrowd_SetupProc, STATGROUP_CkCrowd);

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
        SCOPE_CYCLE_COUNTER(STAT_CkCrowd_SetupProc);

        const auto Radius = InParams.Get_Radius();
        const auto Height = InParams.Get_Height();
        const auto HalfHeight = Height * 0.5f;
        const auto ProbeRadius = Radius + InParams.Get_SeparationLookahead();

        ck::crowd::VeryVerbose(TEXT("CrowdAgent setup: [{}] (radius={}, height={}, probe_radius={})"),
            InHandle, Radius, Height, ProbeRadius);

        auto AgentNonConst = InHandle;

        // Owned by the agent so it cascade-destroys with it.
        auto ProbeChildEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(AgentNonConst);

        // Overwritten by SceneNode propagation every tick — this only covers the first frame.
        const auto& AgentXform = InTransform.Get_Transform();
        const auto InitialChildXform = FTransform{
            AgentXform.GetRotation(),
            AgentXform.GetLocation() + FVector{0.0, 0.0, HalfHeight},
            FVector::OneVector};

        auto ProbeChildTransform = UCk_Utils_Transform_UE::Add(
            ProbeChildEntity, InitialChildXform, ECk_Replication::DoesNotReplicate);

        // The +SeparationLookahead reach gives steering time to nudge before agents actually touch.
        const auto CylinderDimensions = FCk_ShapeCylinder_Dimensions{HalfHeight, ProbeRadius};
        const auto CylinderParams = FCk_Fragment_ShapeCylinder_ParamsData{CylinderDimensions};
        UCk_Utils_ShapeCylinder_UE::Add(ProbeChildEntity, CylinderParams);

        // Keep the probe NAME specific to agents while its filter also admits the local-steering
        // avoidance volumes. This prevents unrelated Crowd.Agent probe queries from receiving
        // volume entities while preserving mutual crowd-agent overlaps.
        auto ProbeParams = FCk_Fragment_Probe_ParamsData{TAG_Crowd_Agent};
        auto ProbeFilter = FGameplayTagContainer{TAG_Crowd_Agent};
        ProbeFilter.AddTag(TAG_Crowd_AvoidanceVolume);
        ProbeParams.Set_Filter(ProbeFilter);
        ProbeParams.Set_ContextOverlapPolicy(ECk_Probe_ContextOverlapPolicy::Any);
        ProbeParams.Set_MotionType(ECk_MotionType::Kinematic);

        auto ProbeHandle = UCk_Utils_Probe_UE::Add(ProbeChildTransform, ProbeParams, FCk_Probe_DebugInfo{});

        // The agent Transform sits at its FEET and the probe shape is centered, so the parented
        // child needs +HalfHeight on Z to put the cylinder's mid-height at the agent's center.
        auto AgentTransform = UCk_Utils_Transform_UE::Cast(InHandle);
        const auto LocalOffset = FTransform{
            FRotator::ZeroRotator,
            FVector{0.0, 0.0, HalfHeight},
            FVector::OneVector};
        UCk_Utils_SceneNode_UE::Add(ProbeChildTransform, AgentTransform, LocalOffset);

        InProbeRef._ProbeChild = ProbeHandle;

        // NeedsSetup is shared with other Setup-marked processors — clear it only once our own
        // work is done.
        AgentNonConst.Add<FTag_CrowdAgent_HasProbe>();
        AgentNonConst.Try_Remove<FTag_CrowdAgent_NeedsSetup>();
    }
}

// --------------------------------------------------------------------------------------------------------------------
