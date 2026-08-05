#include "CkProbe_EditorProcessor.h"

#if WITH_EDITOR

#include "CkCore/Debug/CkDebugDraw_Utils.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/EditorSelectionOwner/CkEditorSelectionOwner_Utils.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkSpatialQuery/Settings/CkSpatialQuery_Settings.h"

#include <GameFramework/Actor.h>

// --------------------------------------------------------------------------------------------------------------------

// Retained by CkEntityVisualizer. Deliberately not registered: Duration=0 debug draw required
// four full probe scans and redraws every editor frame, even when no probe changed.

// --------------------------------------------------------------------------------------------------------------------

DECLARE_CYCLE_STAT(
    TEXT("Probe Preview::Selection Filter"),
    STAT_CkProbeEditorPreview_SelectionFilter,
    STATGROUP_CkProcessors_Details);
DECLARE_CYCLE_STAT(
    TEXT("Probe Preview::Draw"),
    STAT_CkProbeEditorPreview_Draw,
    STATGROUP_CkProcessors_Details);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    namespace probe_editor_preview
    {
        constexpr auto Duration = 0.0f;
        constexpr auto Segments = 12;

        auto
        ShouldDrawPreview(
            const FCk_Handle& InHandle) -> bool
        {
            if (UCk_Utils_SpatialQuery_Settings::Get_DebugPreviewAllProbes())
            { return true; }

            const auto* SelectionOwner = UCk_Utils_EditorSelectionOwner_UE::TryGet_SelectionOwner(InHandle);
            return ck::Is_NOT_Valid(SelectionOwner) ? true : SelectionOwner->IsSelected();
        }

        static auto
        DrawProbeShape(
            const UWorld* InWorld,
            const FTransform& InTransform,
            const FFragment_ShapeBox_Current& InShape,
            const FFragment_Probe_DebugInfo& InDebugInfo) -> void
        {
            UCk_Utils_DebugDraw_UE::DrawDebugBox(
                InWorld,
                InTransform.GetLocation(),
                InShape.Get_Dimensions().Get_HalfExtents(),
                InDebugInfo.Get_Color(),
                InTransform.GetRotation().Rotator(),
                Duration,
                InDebugInfo.Get_LineThickness());
        }

        static auto
        DrawProbeShape(
            const UWorld* InWorld,
            const FTransform& InTransform,
            const FFragment_ShapeSphere_Current& InShape,
            const FFragment_Probe_DebugInfo& InDebugInfo) -> void
        {
            UCk_Utils_DebugDraw_UE::DrawDebugSphere(
                InWorld,
                InTransform.GetLocation(),
                InShape.Get_Dimensions().Get_Radius(),
                Segments,
                InDebugInfo.Get_Color(),
                Duration,
                InDebugInfo.Get_LineThickness());
        }

        static auto
        DrawProbeShape(
            const UWorld* InWorld,
            const FTransform& InTransform,
            const FFragment_ShapeCapsule_Current& InShape,
            const FFragment_Probe_DebugInfo& InDebugInfo) -> void
        {
            UCk_Utils_DebugDraw_UE::DrawDebugCapsule(
                InWorld,
                InTransform.GetLocation(),
                InShape.Get_Dimensions().Get_HalfHeight(),
                InShape.Get_Dimensions().Get_Radius(),
                InTransform.GetRotation().Rotator(),
                InDebugInfo.Get_Color(),
                Duration,
                InDebugInfo.Get_LineThickness());
        }

        static auto
        DrawProbeShape(
            const UWorld* InWorld,
            const FTransform& InTransform,
            const FFragment_ShapeCylinder_Current& InShape,
            const FFragment_Probe_DebugInfo& InDebugInfo) -> void
        {
            const auto& Dimensions = InShape.Get_Dimensions();
            const auto UpAxis = InTransform.GetRotation().GetAxisZ();
            const auto Center = InTransform.GetLocation();
            const auto HalfVec = UpAxis * Dimensions.Get_HalfHeight();

            UCk_Utils_DebugDraw_UE::DrawDebugCylinder(
                InWorld,
                Center - HalfVec,
                Center + HalfVec,
                Dimensions.Get_Radius(),
                Segments,
                InDebugInfo.Get_Color(),
                Duration,
                InDebugInfo.Get_LineThickness());
        }
    }

    // --------------------------------------------------------------------------------------------------------------------

    template <typename T_ShapeFragment>
    auto
        TProcessor_Probe_Preview_EditorTime<T_ShapeFragment>::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const T_ShapeFragment& InShape,
            const FFragment_Probe_DebugInfo& InDebugInfo,
            const FFragment_Transform& InTransform)
        -> void
    {
        const auto ShouldDrawPreview = [&InHandle]()
        {
            SCOPE_CYCLE_COUNTER(STAT_CkProbeEditorPreview_SelectionFilter);
            return probe_editor_preview::ShouldDrawPreview(InHandle);
        }();

        if (NOT ShouldDrawPreview)
        { return; }

        SCOPE_CYCLE_COUNTER(STAT_CkProbeEditorPreview_Draw);

        auto* World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);
        if (ck::Is_NOT_Valid(World))
        { return; }

        probe_editor_preview::DrawProbeShape(
            World, InTransform.Get_Transform(), InShape, InDebugInfo);
    }

    // --------------------------------------------------------------------------------------------------------------------
    // Explicit instantiations so the registrations above link.

    template class TProcessor_Probe_Preview_EditorTime<FFragment_ShapeBox_Current>;
    template class TProcessor_Probe_Preview_EditorTime<FFragment_ShapeSphere_Current>;
    template class TProcessor_Probe_Preview_EditorTime<FFragment_ShapeCapsule_Current>;
    template class TProcessor_Probe_Preview_EditorTime<FFragment_ShapeCylinder_Current>;
}

#endif

// --------------------------------------------------------------------------------------------------------------------
