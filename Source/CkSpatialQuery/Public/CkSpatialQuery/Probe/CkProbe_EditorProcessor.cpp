#include "CkProbe_EditorProcessor.h"

#include "CkCore/Debug/CkDebugDraw_Utils.h"
#include "CkCore/EditorOnly/CkEditorOnly_Utils.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/Scheduler/CkProcessorRegistration.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_Probe_Preview_Box_EditorTime);
CK_REGISTER_PROCESSOR(ck::FProcessor_Probe_Preview_Sphere_EditorTime);
CK_REGISTER_PROCESSOR(ck::FProcessor_Probe_Preview_Capsule_EditorTime);
CK_REGISTER_PROCESSOR(ck::FProcessor_Probe_Preview_Cylinder_EditorTime);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    namespace probe_editor_preview
    {
        constexpr auto Duration = 0.0f;
        constexpr auto Segments = 12;

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
#if WITH_EDITOR
        auto* EditorWorld = UCk_Utils_EditorOnly_UE::Get_OpenedEditorLevelWorld();
        if (ck::Is_NOT_Valid(EditorWorld))
        { return; }

        probe_editor_preview::DrawProbeShape(
            EditorWorld, InTransform.Get_Transform(), InShape, InDebugInfo);
#endif
    }

    // --------------------------------------------------------------------------------------------------------------------
    // Explicit instantiations so the registrations above link.

    template class TProcessor_Probe_Preview_EditorTime<FFragment_ShapeBox_Current>;
    template class TProcessor_Probe_Preview_EditorTime<FFragment_ShapeSphere_Current>;
    template class TProcessor_Probe_Preview_EditorTime<FFragment_ShapeCapsule_Current>;
    template class TProcessor_Probe_Preview_EditorTime<FFragment_ShapeCylinder_Current>;
}

// --------------------------------------------------------------------------------------------------------------------
