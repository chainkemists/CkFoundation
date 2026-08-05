#include "CkTransform_EditorProcessor.h"

#if WITH_EDITOR

#include "CkCore/Debug/CkDebugDraw_Utils.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/EditorSelectionOwner/CkEditorSelectionOwner_Utils.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include <GameFramework/Actor.h>
#include <HAL/IConsoleManager.h>

// --------------------------------------------------------------------------------------------------------------------

// Retained by CkEntityVisualizer. Deliberately not registered: Duration=0 debug draw required a
// full transform scan and redraw every editor frame, even when nothing moved.

// --------------------------------------------------------------------------------------------------------------------

DECLARE_CYCLE_STAT(
    TEXT("Transform Preview::Selection Filter"),
    STAT_CkTransformEditorPreview_SelectionFilter,
    STATGROUP_CkProcessors_Details);
DECLARE_CYCLE_STAT(
    TEXT("Transform Preview::Draw"),
    STAT_CkTransformEditorPreview_Draw,
    STATGROUP_CkProcessors_Details);

// --------------------------------------------------------------------------------------------------------------------

namespace ck_transform_editor_processor
{
    TAutoConsoleVariable<int32> CVar_PreviewAllTransforms(
        TEXT("ck.EcsExt.PreviewAllTransforms"),
        0,
        TEXT("Draw editor transform previews for every preview entity. When disabled, selection-owned entities draw only for selected actors; owner-less entities remain visible."),
        ECVF_Default);

    auto
    ShouldDrawPreview(
        const FCk_Handle& InHandle) -> bool
    {
        if (CVar_PreviewAllTransforms.GetValueOnGameThread() != 0)
        { return true; }

        const auto* SelectionOwner = UCk_Utils_EditorSelectionOwner_UE::TryGet_SelectionOwner(InHandle);
        return ck::Is_NOT_Valid(SelectionOwner) ? true : SelectionOwner->IsSelected();
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_Transform_Preview_EditorTime::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Transform& InTransform)
        -> void
    {
        const auto ShouldDrawPreview = [&InHandle]()
        {
            SCOPE_CYCLE_COUNTER(STAT_CkTransformEditorPreview_SelectionFilter);
            return ck_transform_editor_processor::ShouldDrawPreview(InHandle);
        }();

        if (NOT ShouldDrawPreview)
        { return; }

        SCOPE_CYCLE_COUNTER(STAT_CkTransformEditorPreview_Draw);

        auto* World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);
        if (ck::Is_NOT_Valid(World))
        { return; }

        constexpr auto AxisLength = 50.0f;
        constexpr auto AxisThickness = 2.0f;
        constexpr auto DrawAxisCones = true;
        constexpr auto ConeSize = 6.0f;
        constexpr auto Duration = 0.0f;

        UCk_Utils_DebugDraw_UE::DrawDebugTransformGizmo(
            World,
            InTransform.Get_Transform(),
            AxisLength,
            AxisThickness,
            DrawAxisCones,
            ConeSize,
            Duration);
    }
}

#endif

// --------------------------------------------------------------------------------------------------------------------
