#include "CkTransform_EditorProcessor.h"

#include "CkCore/Debug/CkDebugDraw_Utils.h"
#include "CkCore/EditorOnly/CkEditorOnly_Utils.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/Scheduler/CkProcessorRegistration.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_Transform_Debug_EditorTime);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_Transform_Debug_EditorTime::
        DoTick(
            TimeType InDeltaT)
        -> void
    {
        TProcessor::DoTick(InDeltaT);

#if WITH_EDITOR
        // Force a viewport redraw once per tick so the persistent-debug gizmos drawn above are
        // actually refreshed in the level editor.
        UCk_Utils_EditorOnly_UE::Request_RedrawLevelEditingViewports();
#endif
    }

    auto
        FProcessor_Transform_Debug_EditorTime::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Transform& InTransform)
        -> void
    {
#if WITH_EDITOR
        auto* EditorWorld = UCk_Utils_EditorOnly_UE::Get_OpenedEditorLevelWorld();
        if (ck::Is_NOT_Valid(EditorWorld))
        { return; }

        constexpr auto AxisLength = 50.0f;
        constexpr auto AxisThickness = 2.0f;
        constexpr auto DrawAxisCones = true;
        constexpr auto ConeSize = 6.0f;
        constexpr auto Duration = 0.0f;

        UCk_Utils_DebugDraw_UE::DrawDebugTransformGizmo(
            EditorWorld,
            InTransform.Get_Transform(),
            AxisLength,
            AxisThickness,
            DrawAxisCones,
            ConeSize,
            Duration);
#endif
    }
}

// --------------------------------------------------------------------------------------------------------------------
