#include "CkCrowd/AvoidanceVolume/CkCrowdAvoidanceVolume_Processor.h"

#include "CkCrowd/Agent/CkCrowdAgent_Neighbors_Fragment.h"
#include "CkCrowd/Agent/CkCrowdAgent_PathRefresh_Processor.h"
#include "CkCrowd/AvoidanceVolume/CkCrowdAvoidanceVolume_NavArea.h"

#include "CkCore/Validation/CkIsValid.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"
#include "CkEcsExt/SceneNode/CkSceneNode_Utils.h"
#include "CkEcsExt/Transform/CkTransform_Utils.h"
#include "CkNavigation/NavAreaMarkup/CkNavAreaMarkup_Utils.h"
#include "CkNavigation/Revision/CkNavigationRevision_Subsystem.h"
#include "CkShapes/Box/CkShapeBox_Utils.h"
#include "CkSpatialQuery/Probe/CkProbe_Utils.h"

#include <NavigationSystem.h>
#include <NavMesh/RecastNavMesh.h>

CK_REGISTER_PROCESSOR(ck::FProcessor_CrowdAvoidanceVolume_Setup);
CK_REGISTER_PROCESSOR(ck::FProcessor_CrowdAvoidanceVolume_Monitor);
CK_REGISTER_PROCESSOR(ck::FProcessor_CrowdAvoidanceVolume_EndPlay);

namespace ck_crowd_avoidance_volume
{
    constexpr auto TransformDriftTolerance = 0.01f;
}

namespace ck
{
    auto FProcessor_CrowdAvoidanceVolume_Monitor::Release_Runtime(
        FFragment_CrowdAvoidanceVolume_ProbeRef& InRuntime) -> void
    {
        if (InRuntime._Markup.IsValid())
        { UCk_Utils_NavAreaMarkup_UE::Request_Destroy(InRuntime._Markup.Get()); }
        InRuntime._Markup.Reset();

        if (ck::IsValid(InRuntime._ProbeChild))
        { UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(InRuntime._ProbeChild); }
        InRuntime._ProbeChild = {};
        InRuntime._AuthoredObb = {};
        InRuntime._PaintedObb = {};
        InRuntime._AuthoredTransform = FTransform::Identity;
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
        const auto PaintedObb = Obb.ExpandedXY(PathPlanningClearance);
        const auto IsValidInput = Obb.IsFiniteAndPositive() && FMath::IsFinite(Influence) && Influence >= 0.0f &&
            PaintedObb.IsFiniteAndPositive() &&
            FMath::IsFinite(Obb._WorldHalfExtents.X + Influence) &&
            FMath::IsFinite(Obb._WorldHalfExtents.Y + Influence);
        CK_ENSURE_IF_NOT(IsValidInput,
            TEXT("CrowdAvoidanceVolume [{}] requires a finite static Transform, positive world extents, and non-negative influence."),
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

        const auto Cleanup = [&](UCk_NavAreaMarkup_UE* InMarkup) -> void
        {
            UCk_Utils_NavAreaMarkup_UE::Request_Destroy(InMarkup);
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
        { Cleanup(nullptr); return; }

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
        { Cleanup(nullptr); return; }

        auto ProbeParams = FCk_Fragment_Probe_ParamsData{TAG_Crowd_AvoidanceVolume};
        ProbeParams.Set_Filter(FGameplayTagContainer{TAG_Crowd_Agent});
        ProbeParams.Set_ContextOverlapPolicy(ECk_Probe_ContextOverlapPolicy::Any);
        ProbeParams.Set_MotionType(ECk_MotionType::Kinematic);
        const auto Probe = UCk_Utils_Probe_UE::Add(ProbeTransform, ProbeParams, FCk_Probe_DebugInfo{});
        const auto HasProbe = ck::IsValid(Probe);
        CK_ENSURE_IF_NOT(HasProbe, TEXT("CrowdAvoidanceVolume [{}] failed to add its probe feature."), Volume)
        { }
        if (NOT HasProbe)
        { Cleanup(nullptr); return; }

        auto VolumeTransform = UCk_Utils_Transform_UE::Cast(Volume);
        const auto HasVolumeTransform = ck::IsValid(VolumeTransform);
        CK_ENSURE_IF_NOT(HasVolumeTransform, TEXT("CrowdAvoidanceVolume [{}] lost its Transform during setup."), Volume)
        { }
        if (NOT HasVolumeTransform)
        { Cleanup(nullptr); return; }

        const auto ProbeNode = UCk_Utils_SceneNode_UE::Add(ProbeTransform, VolumeTransform, FTransform::Identity);
        const auto HasProbeNode = ck::IsValid(ProbeNode);
        CK_ENSURE_IF_NOT(HasProbeNode, TEXT("CrowdAvoidanceVolume [{}] failed to attach its probe child."), Volume)
        { }
        if (NOT HasProbeNode)
        { Cleanup(nullptr); return; }

        auto GenericVolume = static_cast<FCk_Handle>(Volume);
        auto* Markup = UCk_Utils_NavAreaMarkup_UE::Request_Create(
            GenericVolume,
            PaintedObb._YawTransform,
            PaintedObb._WorldHalfExtents,
            UCk_NavArea_CrowdAvoidanceVolume::StaticClass());
        const auto HasMarkup = IsValid(Markup);
        CK_ENSURE_IF_NOT(HasMarkup, TEXT("CrowdAvoidanceVolume [{}] failed to register nav-area markup."), Volume)
        { }
        if (NOT HasMarkup)
        { Cleanup(Markup); return; }

        InRuntime._ProbeChild = Probe;
        InRuntime._Markup = Markup;
        InRuntime._AuthoredObb = Obb;
        InRuntime._PaintedObb = PaintedObb;
        InRuntime._AuthoredTransform = AuthoredTransform;
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
        const auto HasMarkup = InRuntime._Markup.IsValid();
        CK_ENSURE_IF_NOT(HasMarkup, TEXT("CrowdAvoidanceVolume [{}] lost its pooled nav-area markup."), Volume)
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

        auto* World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(Volume);
        auto* NavSystem = IsValid(World) ? UNavigationSystemV1::GetCurrent(World) : nullptr;
        auto* NavMesh = NavSystem != nullptr
            ? Cast<ARecastNavMesh>(NavSystem->GetDefaultNavDataInstance(FNavigationSystem::DontCreate))
            : nullptr;
        const auto AreaId = NavMesh != nullptr
            ? NavMesh->GetAreaID(UCk_NavArea_CrowdAvoidanceVolume::StaticClass())
            : INDEX_NONE;
        if (NavMesh == nullptr || AreaId == INDEX_NONE)
        { return; }

        const auto Samples = crowd_avoidance_volume::GetConfirmationSamplePoints(InRuntime._PaintedObb);
        const auto ProbeExtent = FVector{10.0f, 10.0f, InRuntime._PaintedObb._WorldHalfExtents.Z};
        auto IsConfirmed = NOT Samples.IsEmpty();
        for (const auto& Sample : Samples)
        {
            const auto Poly = NavMesh->FindNearestPoly(Sample, ProbeExtent);
            if (Poly != INVALID_NAVNODEREF && static_cast<int32>(NavMesh->GetPolyAreaID(Poly)) == AreaId)
            { continue; }
            IsConfirmed = false;
            break;
        }
        if (NOT IsConfirmed)
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
        if (InRuntime._Markup.IsValid() &&
            InRuntime._ConfirmedOnMesh &&
            InRuntime._AuthoredObb.IsFiniteAndPositive() &&
            InRuntime._PaintedObb.IsFiniteAndPositive() &&
            CanTrackRetirement)
        {
            _PendingRetirements.Add(FCk_CrowdAvoidanceVolume_Retirement{
                InRuntime._AuthoredObb,
                InRuntime._PaintedObb,
                RevisionSubsystem->Get_Revision()});
        }
        FProcessor_CrowdAvoidanceVolume_Monitor::Release_Runtime(InRuntime);
    }
}

// --------------------------------------------------------------------------------------------------------------------
