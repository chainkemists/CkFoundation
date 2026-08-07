#include "CkIskmProxy_StylizeMaskProcessor.h"

#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkIskmRenderer/CkIskmRenderer_Log.h"

#include "CkUsf/Stylize/CkUsf_Stylize_ProjectSettings.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_IskmProxy_StylizeMask_Sync);
CK_REGISTER_PROCESSOR(ck::FProcessor_IskmProxy_StylizeMask_DropAppliedOnOutline);
CK_REGISTER_PROCESSOR(ck::FProcessor_IskmProxy_StylizeMask_DropAppliedOnCelPattern);
CK_REGISTER_PROCESSOR(ck::FProcessor_IskmProxy_StylizeMask_Remove);
CK_REGISTER_PROCESSOR(ck::FProcessor_IskmProxy_StylizeMask_EndPlay);

// --------------------------------------------------------------------------------------------------------------------

namespace ck_iskm_stylize_mask_processor
{
    auto
        DoSetCustomDepthOnProxySkmcs(
            const ck::FFragment_IskmProxy_Current& InCurrent,
            bool InEnabled,
            int32 InStencilValue)
        -> void
    {
        const auto& ApplyTo = [&](USkeletalMeshComponent* InSkmc)
        {
            if (ck::Is_NOT_Valid(InSkmc))
            { return; }

            InSkmc->SetRenderCustomDepth(InEnabled);
            InSkmc->SetCustomDepthStencilValue(InStencilValue);
        };

        ApplyTo(InCurrent.Get_BaseSKMC().Get());

        for (const auto& Submesh : InCurrent.Get_SubmeshSKMCs())
        { ApplyTo(Submesh.Get()); }
    }

    // Undo, but only on the SKMCs still carrying what THIS feature wrote. Used where a higher-precedence
    // claim is present on the same shared byte: if that claim's own Sync has already overwritten the
    // value this is a no-op, and if it has not (a null subsystem, a disabled stencil contract, an
    // exhausted range) this is what stops the mesh keeping a stencil no processor is left tracking.
    auto
        DoDisableCustomDepthIfStillOurs(
            const ck::FFragment_IskmProxy_Current& InCurrent,
            int32 InStencilValue)
        -> void
    {
        const auto& DisableIfOurs = [&](USkeletalMeshComponent* InSkmc)
        {
            if (ck::Is_NOT_Valid(InSkmc))
            { return; }

            if (InSkmc->CustomDepthStencilValue != InStencilValue)
            { return; }

            InSkmc->SetRenderCustomDepth(false);
        };

        DisableIfOurs(InCurrent.Get_BaseSKMC().Get());

        for (const auto& Submesh : InCurrent.Get_SubmeshSKMCs())
        { DisableIfOurs(Submesh.Get()); }
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_IskmProxy_StylizeMask_Sync::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Usf_StylizeMaskTarget& InTarget,
            const FFragment_IskmProxy_Current& InCurrent)
        -> void
    {
        using namespace ck_iskm_stylize_mask_processor;

        if (ck::Is_NOT_Valid(InCurrent.Get_BaseSKMC().Get()))
        { return; } // Setup incomplete or SKMC released — nothing to flag yet

        // Project config rather than a per-world subsystem: the mask claims ONE byte value for the whole
        // project, and no world-local state narrows it. Read every frame so an editor-side change lands
        // without a restart, the way the cel contract's stencil base does.
        const auto StencilValue = UCk_Utils_Usf_Stylize_Settings_UE::Get_MaskStencilValue();

        // 0 is the engine's "nothing written here" — writing it would mark the meshes with the value that
        // means untagged and read as a silent success. The project setting clamps at 1, so this only fires
        // on an .ini hand-edited past the clamp.
        if (StencilValue <= 0)
        { return; }

        if (InHandle.Has<FFragment_IskmProxy_StylizeMaskApplied>())
        {
            const auto& Applied = InHandle.Get<FFragment_IskmProxy_StylizeMaskApplied>();

            if (Applied.Get_StencilValue() == StencilValue)
            {
                // Re-assert every frame: the setters early-out when unchanged, and this is what makes
                // late-attached outfit submeshes (and mesh swaps) inherit the mask automatically.
                DoSetCustomDepthOnProxySkmcs(InCurrent, true, StencilValue);
                return;
            }

            // The project's mask value moved: undo, then fall through to re-apply.
            DoSetCustomDepthOnProxySkmcs(InCurrent, false, 0);
            InHandle.Remove<FFragment_IskmProxy_StylizeMaskApplied>();
        }

        DoSetCustomDepthOnProxySkmcs(InCurrent, true, StencilValue);
        InHandle.AddOrGet<FFragment_IskmProxy_StylizeMaskApplied>() =
            FFragment_IskmProxy_StylizeMaskApplied{StencilValue};

        ck::iskm::VeryVerbose(TEXT("Applied stylize mask (custom depth) for ISKM Proxy [{}]"), InHandle);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_IskmProxy_StylizeMask_DropAppliedOnOutline::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_IskmProxy_StylizeMaskApplied& InApplied,
            const FFragment_Usf_OutlineTarget& InOutlineTarget,
            const FFragment_IskmProxy_Current& InCurrent)
        -> void
    {
        // UNDO before dropping the state that records what to undo. Dropping alone assumes the higher
        // claim's own Sync overwrites the byte in this same group — but that Sync can silently no-op (a
        // null subsystem, a disabled stencil contract, an exhausted range), and once this fragment is
        // gone neither _Remove nor _EndPlay matches the entity, so the SKMCs keep this feature's stencil
        // forever with nothing left that knows to clear it. Value-guarded, so a higher claim that HAS
        // already written sees a no-op.
        ck_iskm_stylize_mask_processor::DoDisableCustomDepthIfStillOurs(InCurrent, InApplied.Get_StencilValue());
        InHandle.Try_Remove<FFragment_IskmProxy_StylizeMaskApplied>();
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_IskmProxy_StylizeMask_DropAppliedOnCelPattern::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_IskmProxy_StylizeMaskApplied& InApplied,
            const FFragment_Usf_CelPatternTarget& InCelPatternTarget,
            const FFragment_IskmProxy_Current& InCurrent)
        -> void
    {
        // Undo first — same reasoning as the outline twin above.
        ck_iskm_stylize_mask_processor::DoDisableCustomDepthIfStillOurs(InCurrent, InApplied.Get_StencilValue());
        InHandle.Try_Remove<FFragment_IskmProxy_StylizeMaskApplied>();
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_IskmProxy_StylizeMask_Remove::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_IskmProxy_StylizeMaskApplied& InApplied,
            const FFragment_IskmProxy_Current& InCurrent)
        -> void
    {
        ck_iskm_stylize_mask_processor::DoSetCustomDepthOnProxySkmcs(InCurrent, false, 0);
        InHandle.Try_Remove<FFragment_IskmProxy_StylizeMaskApplied>();
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_IskmProxy_StylizeMask_EndPlay::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_IskmProxy_StylizeMaskApplied& InApplied,
            const FFragment_IskmProxy_Current& InCurrent)
        -> void
    {
        ck_iskm_stylize_mask_processor::DoSetCustomDepthOnProxySkmcs(InCurrent, false, 0);
        InHandle.Try_Remove<FFragment_IskmProxy_StylizeMaskApplied>();
    }
}

// --------------------------------------------------------------------------------------------------------------------
