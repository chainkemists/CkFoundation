#include "CkIsmProxy_EditorProcessor.h"

#include "CkCore/Debug/CkDebugDraw_Utils.h"
#include "CkCore/EditorOnly/CkEditorOnly_Utils.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/Scheduler/CkProcessorRegistration.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_IsmProxy_Preview_EditorTime);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_IsmProxy_Preview_EditorTime::
        DoTick(
            TimeType InDeltaT)
        -> void
    {
        TProcessor::DoTick(InDeltaT);

#if WITH_EDITOR
        UCk_Utils_EditorOnly_UE::Request_RedrawLevelEditingViewports();
#endif
    }

    auto
        FProcessor_IsmProxy_Preview_EditorTime::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_IsmProxy_Params& InParams,
            const FFragment_Transform& InTransform)
        -> void
    {
#if WITH_EDITOR
        auto* EditorWorld = UCk_Utils_EditorOnly_UE::Get_OpenedEditorLevelWorld();
        if (ck::Is_NOT_Valid(EditorWorld))
        { return; }

        // Indicator box size is intentionally fixed — it signals "ISM instance authored here"
        // without depending on the actual mesh bounds. Decision D8 tracks the richer mesh-preview
        // path as a follow-up.
        constexpr auto BoxHalfExtentBase = 25.0f;
        const auto BoxHalfExtent = FVector{BoxHalfExtentBase} * InParams.Get_ScaleMultiplier();

        const auto& Transform = InTransform.Get_Transform();
        const auto Location = Transform.GetLocation() + Transform.TransformVector(InParams.Get_LocalLocationOffset());
        const auto Rotation = Transform.GetRotation() * InParams.Get_LocalRotationOffset().Quaternion();

        const auto Color = FLinearColor{0.2f, 0.8f, 1.0f, 1.0f};
        constexpr auto Thickness = 1.5f;
        constexpr auto Duration = 0.0f;

        UCk_Utils_DebugDraw_UE::DrawDebugWireframeBox(
            EditorWorld,
            Location,
            BoxHalfExtent,
            Rotation,
            Color,
            Duration,
            Thickness);
#endif
    }
}

// --------------------------------------------------------------------------------------------------------------------
