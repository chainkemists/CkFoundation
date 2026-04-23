#include "CkProbe_EditorProcessor.h"

#include "CkCore/Debug/CkDebugDraw_Utils.h"
#include "CkCore/EditorOnly/CkEditorOnly_Utils.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/Scheduler/CkProcessorRegistration.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_Probe_Preview_EditorTime);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_Probe_Preview_EditorTime::
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
        FProcessor_Probe_Preview_EditorTime::
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

        constexpr auto Radius = 30.0f;
        constexpr auto Segments = 12;
        const auto Color = FLinearColor{1.0f, 0.5f, 0.2f, 1.0f};
        constexpr auto Duration = 0.0f;
        constexpr auto Thickness = 1.5f;

        UCk_Utils_DebugDraw_UE::DrawDebugSphere(
            EditorWorld,
            InTransform.Get_Transform().GetLocation(),
            Radius,
            Segments,
            Color,
            Duration,
            Thickness);
#endif
    }
}

// --------------------------------------------------------------------------------------------------------------------
