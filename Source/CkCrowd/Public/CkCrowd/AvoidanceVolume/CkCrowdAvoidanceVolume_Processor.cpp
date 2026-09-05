#include "CkCrowd/AvoidanceVolume/CkCrowdAvoidanceVolume_Processor.h"

#include "CkCrowd/Agent/CkCrowdAgent_Neighbors_Fragment.h"
#include "CkCrowd/Agent/CkCrowdAgent_PathRefresh_Processor.h"
#include "CkCrowd/CkCrowd_NavGameplayTags.h"

#include "CkCore/Validation/CkIsValid.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"
#include "CkEcsExt/SceneNode/CkSceneNode_Utils.h"
#include "CkEcsExt/Transform/CkTransform_Utils.h"
#include "CkNavigation/NavSurface/CkNavSurface_Utils.h"
#include "CkNavigation/Revision/CkNavigationRevision_Subsystem.h"
#include "CkShapes/Box/CkShapeBox_Utils.h"
#include "CkShapes/CkShapes_Common.h"
#include "CkSpatialQuery/Probe/CkProbe_Utils.h"

CK_REGISTER_PROCESSOR(ck::FProcessor_CrowdAvoidanceVolume_Setup);
CK_REGISTER_PROCESSOR(ck::FProcessor_CrowdAvoidanceVolume_Monitor);
CK_REGISTER_PROCESSOR(ck::FProcessor_CrowdAvoidanceVolume_EndPlay);

namespace ck_crowd_avoidance_volume
{
    constexpr auto TransformDriftTolerance = 0.01f;

    // The painted area, named the way the provider-neutral surface names areas. The paint is raised
    // by TAG and each provider resolves that tag its own way, which is the whole of what lets one
    // painter serve both.
    auto Get_NavAreaTag(
        ECk_CrowdAvoidanceVolume_TraversalPolicy InTraversalPolicy)
        -> FGameplayTag
    {
        switch (InTraversalPolicy)
        {
            case ECk_CrowdAvoidanceVolume_TraversalPolicy::AvoidIfPossible:
                return TAG_Nav_Area_Crowd_AvoidanceVolume.GetTag();
            case ECk_CrowdAvoidanceVolume_TraversalPolicy::HardExclude:
                return TAG_Nav_Area_Crowd_AvoidanceVolume_HardExclude.GetTag();
            case ECk_CrowdAvoidanceVolume_TraversalPolicy::CostOnly:
                return TAG_Nav_Area_Crowd_AvoidanceVolume_CostOnly.GetTag();
            default:
                return {};
        }
    }
}

namespace ck
{
    auto FProcessor_CrowdAvoidanceVolume_Monitor::Release_Runtime(
        FFragment_CrowdAvoidanceVolume_ProbeRef& InRuntime) -> void
    {
        if (ck::IsValid(InRuntime._Markup))
        { UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(InRuntime._Markup); }
        InRuntime._Markup = {};

        if (ck::IsValid(InRuntime._ProbeChild))
        { UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(InRuntime._ProbeChild); }
        InRuntime._ProbeChild = {};
        InRuntime._AuthoredObb = {};
        InRuntime._PaintedObb = {};
        InRuntime._AuthoredTransform = FTransform::Identity;
        InRuntime._TraversalPolicy = ECk_CrowdAvoidanceVolume_TraversalPolicy::AvoidIfPossible;
        InRuntime._SecondsSincePaint = 0.0f;
        InRuntime._ConfirmationSerial = 0;
        InRuntime._ConfirmedOnMesh = false;
    }

    auto FProcessor_CrowdAvoidanceVolume_Setup::ForEachEntity(
        TimeType InDeltaT,
        HandleType InHandle,
        const FFragment_CrowdAvoidanceVolume_Params& InParams,
        const FFragment_Transform& InTransform,
        FFragment_CrowdAvoidanceVolume_ProbeRef& InRuntime) const -> void
    {
        auto Volume = InHandle;
        const auto& AuthoredTransform = InTransform.Get_Transform();
        const auto Obb = crowd_avoidance_volume::MakeObb(AuthoredTransform, InParams.Get_HalfExtents());
        const auto Influence = InParams.Get_InfluenceRange();
        const auto PathPlanningClearance = InParams.Get_PathPlanningClearance();
        const auto TraversalPolicy = InParams.Get_TraversalPolicy();
        const auto NavAreaTag = ck_crowd_avoidance_volume::Get_NavAreaTag(TraversalPolicy);
        const auto PaintedObb = Obb.ExpandedXY(PathPlanningClearance);
        const auto IsValidInput = Obb.IsFiniteAndPositive() && FMath::IsFinite(Influence) && Influence >= 0.0f &&
            PaintedObb.IsFiniteAndPositive() &&
            FMath::IsFinite(Obb._WorldHalfExtents.X + Influence) &&
            FMath::IsFinite(Obb._WorldHalfExtents.Y + Influence) &&
            NavAreaTag.IsValid();
        CK_ENSURE_IF_NOT(IsValidInput,
            TEXT("CrowdAvoidanceVolume [{}] requires a finite static Transform, positive world extents, non-negative influence, and a valid traversal policy."),
            Volume)
        { }
        if (NOT IsValidInput)
        {
            FProcessor_CrowdAvoidanceVolume_Monitor::Release_Runtime(InRuntime);
            Volume.Try_Remove<FTag_CrowdAvoidanceVolume_NeedsSetup>();
            Volume.AddOrGet<FTag_CrowdAvoidanceVolume_Invalid>();
            return;
        }

        auto ProbeChild = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(Volume);
        const auto HasProbeChild = ck::IsValid(ProbeChild);
        CK_ENSURE_IF_NOT(HasProbeChild, TEXT("CrowdAvoidanceVolume [{}] failed to create its probe child."), Volume)
        { }
        if (NOT HasProbeChild)
        {
            Volume.Try_Remove<FTag_CrowdAvoidanceVolume_NeedsSetup>();
            Volume.AddOrGet<FTag_CrowdAvoidanceVolume_Invalid>();
            return;
        }

        const auto Cleanup = [&](FCk_Handle_NavSurfaceMarkup InMarkup) -> void
        {
            if (ck::IsValid(InMarkup))
            { UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(InMarkup); }
            UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(ProbeChild);
            FProcessor_CrowdAvoidanceVolume_Monitor::Release_Runtime(InRuntime);
            Volume.Try_Remove<FTag_CrowdAvoidanceVolume_NeedsSetup>();
            Volume.AddOrGet<FTag_CrowdAvoidanceVolume_Invalid>();
        };

        auto ProbeTransform = UCk_Utils_Transform_UE::Add(ProbeChild, AuthoredTransform, ECk_Replication::DoesNotReplicate);
        const auto HasProbeTransform = ck::IsValid(ProbeTransform);
        CK_ENSURE_IF_NOT(HasProbeTransform, TEXT("CrowdAvoidanceVolume [{}] failed to add its probe Transform."), Volume)
        { }
        if (NOT HasProbeTransform)
        { Cleanup({}); return; }

        // The child inherits AuthoredTransform from its scene-node parent. Keep the shape in
        // authored local space so parent scale is applied exactly once.
        const auto ProbeDimensions = FCk_ShapeBox_Dimensions{FVector{
            InParams.Get_HalfExtents().X + Influence,
            InParams.Get_HalfExtents().Y + Influence,
            InParams.Get_HalfExtents().Z}};
        const auto ProbeShape = UCk_Utils_ShapeBox_UE::Add(ProbeChild, FCk_Fragment_ShapeBox_ParamsData{ProbeDimensions});
        const auto HasProbeShape = ck::IsValid(ProbeShape);
        CK_ENSURE_IF_NOT(HasProbeShape, TEXT("CrowdAvoidanceVolume [{}] failed to add its probe box."), Volume)
        { }
        if (NOT HasProbeShape)
        { Cleanup({}); return; }

        auto ProbeParams = FCk_Fragment_Probe_ParamsData{TAG_Crowd_AvoidanceVolume};
        ProbeParams.Set_Filter(FGameplayTagContainer{TAG_Crowd_Agent});
        ProbeParams.Set_ContextOverlapPolicy(ECk_Probe_ContextOverlapPolicy::Any);
        ProbeParams.Set_MotionType(ECk_MotionType::Kinematic);
        const auto Probe = UCk_Utils_Probe_UE::Add(ProbeTransform, ProbeParams, FCk_Probe_DebugInfo{});
        const auto HasProbe = ck::IsValid(Probe);
        CK_ENSURE_IF_NOT(HasProbe, TEXT("CrowdAvoidanceVolume [{}] failed to add its probe feature."), Volume)
        { }
        if (NOT HasProbe)
        { Cleanup({}); return; }

        auto VolumeTransform = UCk_Utils_Transform_UE::Cast(Volume);
        const auto HasVolumeTransform = ck::IsValid(VolumeTransform);
        CK_ENSURE_IF_NOT(HasVolumeTransform, TEXT("CrowdAvoidanceVolume [{}] lost its Transform during setup."), Volume)
        { }
        if (NOT HasVolumeTransform)
        { Cleanup({}); return; }

        const auto ProbeNode = UCk_Utils_SceneNode_UE::Add(ProbeTransform, VolumeTransform, FTransform::Identity);
        const auto HasProbeNode = ck::IsValid(ProbeNode);
        CK_ENSURE_IF_NOT(HasProbeNode, TEXT("CrowdAvoidanceVolume [{}] failed to attach its probe child."), Volume)
        { }
        if (NOT HasProbeNode)
        { Cleanup({}); return; }

        auto MarkupRequest = FCk_Request_NavSurface_AreaMarkup{
            FCk_AnyShape{FCk_ShapeBox_Dimensions{PaintedObb._WorldHalfExtents}},
            NavAreaTag};
        MarkupRequest.Set_WorldTransform(PaintedObb._YawTransform);

        auto Markup = UCk_Utils_NavSurface_UE::Request_AreaMarkup(
            UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(Volume),
            MarkupRequest,
            {});
        const auto HasMarkup = ck::IsValid(Markup);
        CK_ENSURE_IF_NOT(HasMarkup, TEXT("CrowdAvoidanceVolume [{}] failed to register nav-area markup."), Volume)
        { }
        if (NOT HasMarkup)
        { Cleanup(Markup); return; }

        InRuntime._ProbeChild = Probe;
        InRuntime._Markup = Markup;
        InRuntime._AuthoredObb = Obb;
        InRuntime._PaintedObb = PaintedObb;
        InRuntime._AuthoredTransform = AuthoredTransform;
        InRuntime._TraversalPolicy = TraversalPolicy;
        InRuntime._SecondsSincePaint = 0.0f;
        InRuntime._ConfirmationSerial = 0;
        InRuntime._ConfirmedOnMesh = false;
        Volume.Add<FTag_CrowdAvoidanceVolume_HasRuntime>();
        Volume.Try_Remove<FTag_CrowdAvoidanceVolume_NeedsSetup>();
    }

    auto FProcessor_CrowdAvoidanceVolume_Monitor::ForEachEntity(
        TimeType InDeltaT,
        HandleType InHandle,
        const FFragment_Transform& InTransform,
        FFragment_CrowdAvoidanceVolume_ProbeRef& InRuntime) const -> void
    {
        auto Volume = InHandle;
        const auto IsStatic = InTransform.Get_Transform().Equals(
            InRuntime._AuthoredTransform, ck_crowd_avoidance_volume::TransformDriftTolerance);
        CK_ENSURE_IF_NOT(IsStatic,
            TEXT("CrowdAvoidanceVolume [{}] moved after composition; destroy and recreate it instead of repainting nav tiles."),
            Volume)
        { }
        const auto HasMarkup = ck::IsValid(InRuntime._Markup);
        CK_ENSURE_IF_NOT(HasMarkup, TEXT("CrowdAvoidanceVolume [{}] lost its nav-area markup."), Volume)
        { }
        if (NOT IsStatic || NOT HasMarkup)
        {
            Release_Runtime(InRuntime);
            Volume.Try_Remove<FTag_CrowdAvoidanceVolume_HasRuntime>();
            Volume.Try_Remove<FTag_CrowdAvoidanceVolume_NeedsSetup>();
            Volume.AddOrGet<FTag_CrowdAvoidanceVolume_Invalid>();
            return;
        }

        InRuntime._SecondsSincePaint += static_cast<float>(InDeltaT.Get_Seconds());
        if (InRuntime._ConfirmedOnMesh)
        { return; }

        // The provider's own answer to "has my paint landed", asked of the markup rather than
        // sampled off Recast: a per-sample area probe can only ever confirm on a world planning on
        // Recast, and on a GroundNav world it never confirmed at all.
        if (NOT UCk_Utils_NavSurface_UE::Get_IsMarkupLive(InRuntime.Get_Markup()))
        { return; }

        InRuntime._ConfirmationSerial = FProcessor_CrowdAgent_PathRefresh::IssueConfirmationSerial();
        InRuntime._ConfirmedOnMesh = true;
    }

    auto FProcessor_CrowdAvoidanceVolume_EndPlay::DoTick(FCk_Time InDeltaT) -> void
    {
        _PendingRetirements.Reset();
        TProcessor::DoTick(InDeltaT);
        if (_PendingRetirements.IsEmpty())
        { return; }

        auto Transient = this->_TransientEntity;
        if (ck::Is_NOT_Valid(Transient))
        { return; }

        auto& Retirements = Transient.AddOrGet<FFragment_CrowdAvoidanceVolume_Retirements>();
        Retirements._Records.Append(MoveTemp(_PendingRetirements));
    }

    auto FProcessor_CrowdAvoidanceVolume_EndPlay::ForEachEntity(
        TimeType InDeltaT,
        HandleType InHandle,
        FFragment_CrowdAvoidanceVolume_ProbeRef& InRuntime) -> void
    {
        auto* World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);
        auto* RevisionSubsystem = IsValid(World)
            ? World->GetSubsystem<UCk_NavigationRevisionSubsystem_UE>()
            : nullptr;
        const auto CanTrackRetirement = IsValid(RevisionSubsystem) &&
            RevisionSubsystem->TryEnsureBound();
        if (ck::IsValid(InRuntime._Markup) &&
            InRuntime._ConfirmedOnMesh &&
            InRuntime._AuthoredObb.IsFiniteAndPositive() &&
            InRuntime._PaintedObb.IsFiniteAndPositive() &&
            CanTrackRetirement)
        {
            _PendingRetirements.Add(FCk_CrowdAvoidanceVolume_Retirement{
                static_cast<int64>(InHandle.Get_Entity().Get_ID()),
                InHandle.Get_DebugName(),
                InRuntime._AuthoredObb._YawTransform,
                InRuntime._TraversalPolicy,
                InRuntime._AuthoredObb,
                InRuntime._PaintedObb,
                InRuntime._ConfirmationSerial,
                RevisionSubsystem->Get_Revision()});
        }
        FProcessor_CrowdAvoidanceVolume_Monitor::Release_Runtime(InRuntime);
    }
}

// --------------------------------------------------------------------------------------------------------------------
