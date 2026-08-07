#include "CkIskmProxy_CelPatternProcessor.h"

#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkIskmRenderer/CkIskmRenderer_Log.h"

#include "CkUsf/Stylize/CkUsf_CelShadeSubsystem.h"

#include "Engine/World.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_IskmProxy_CelPattern_Sync);
CK_REGISTER_PROCESSOR(ck::FProcessor_IskmProxy_CelPattern_DropAppliedOnOutline);
CK_REGISTER_PROCESSOR(ck::FProcessor_IskmProxy_CelPattern_Remove);
CK_REGISTER_PROCESSOR(ck::FProcessor_IskmProxy_CelPattern_EndPlay);

// --------------------------------------------------------------------------------------------------------------------

namespace ck_iskm_cel_pattern_processor
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
    // value this is a no-op, and if it has not (a null subsystem, an exhausted range) this is what stops
    // the mesh keeping a stencil no processor is left tracking.
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
        FProcessor_IskmProxy_CelPattern_Sync::
        DoTick(
            TimeType InDeltaT)
        -> void
    {
        _World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(_TransientEntity);

        TProcessor::DoTick(InDeltaT);
    }

    auto
        FProcessor_IskmProxy_CelPattern_Sync::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Usf_CelPatternTarget& InTarget,
            const FFragment_IskmProxy_Current& InCurrent) const
        -> void
    {
        using namespace ck_iskm_cel_pattern_processor;

        if (ck::Is_NOT_Valid(InCurrent.Get_BaseSKMC().Get()))
        { return; } // Setup incomplete or SKMC released — nothing to flag yet

        if (ck::Is_NOT_Valid(_World.Get()))
        { return; }

        auto* CelShadeSubsystem = _World->GetSubsystem<UCkUsf_CelShadeSubsystem>();

        if (ck::Is_NOT_Valid(CelShadeSubsystem, ck::IsValid_Policy_NullptrOnly{}))
        { return; }

        // 0 means the stencil contract is switched off in this world's settings — writing it would mark the
        // meshes with the engine's "nothing here" value and read as a silent success.
        const auto StencilValue = CelShadeSubsystem->Get_StencilValueFor(InTarget.Get_Pattern());

        if (StencilValue == 0)
        { return; }

        if (InHandle.Has<FFragment_IskmProxy_CelPatternApplied>())
        {
            const auto& Applied = InHandle.Get<FFragment_IskmProxy_CelPatternApplied>();

            if (Applied.Get_Pattern() == InTarget.Get_Pattern() &&
                Applied.Get_StencilValue() == StencilValue)
            {
                // Re-assert every frame: the setters early-out when unchanged, and this is what makes
                // late-attached outfit submeshes (and mesh swaps) inherit the pattern automatically.
                DoSetCustomDepthOnProxySkmcs(InCurrent, true, StencilValue);
                return;
            }

            // Pattern (or stencil base) drift: undo, then fall through to re-apply.
            DoSetCustomDepthOnProxySkmcs(InCurrent, false, 0);
            InHandle.Remove<FFragment_IskmProxy_CelPatternApplied>();
        }

        DoSetCustomDepthOnProxySkmcs(InCurrent, true, StencilValue);
        InHandle.AddOrGet<FFragment_IskmProxy_CelPatternApplied>() =
            FFragment_IskmProxy_CelPatternApplied{InTarget.Get_Pattern(), StencilValue};

        ck::iskm::VeryVerbose(TEXT("Applied cel pattern (custom depth) for ISKM Proxy [{}]"), InHandle);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_IskmProxy_CelPattern_DropAppliedOnOutline::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_IskmProxy_CelPatternApplied& InApplied,
            const FFragment_Usf_OutlineTarget& InOutlineTarget,
            const FFragment_IskmProxy_Current& InCurrent)
        -> void
    {
        // UNDO before dropping the state that records what to undo. Dropping alone assumes the outline's
        // own Sync overwrites the byte in this same group — but that Sync can silently no-op (a null
        // subsystem, an exhausted stencil range), and once this fragment is gone neither _Remove nor
        // _EndPlay matches the entity, so the SKMCs keep this feature's stencil forever with nothing left
        // that knows to clear it. Value-guarded, so an outline that HAS already written sees a no-op.
        ck_iskm_cel_pattern_processor::DoDisableCustomDepthIfStillOurs(InCurrent, InApplied.Get_StencilValue());
        InHandle.Try_Remove<FFragment_IskmProxy_CelPatternApplied>();
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_IskmProxy_CelPattern_Remove::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_IskmProxy_CelPatternApplied& InApplied,
            const FFragment_IskmProxy_Current& InCurrent)
        -> void
    {
        ck_iskm_cel_pattern_processor::DoSetCustomDepthOnProxySkmcs(InCurrent, false, 0);
        InHandle.Try_Remove<FFragment_IskmProxy_CelPatternApplied>();
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_IskmProxy_CelPattern_EndPlay::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_IskmProxy_CelPatternApplied& InApplied,
            const FFragment_IskmProxy_Current& InCurrent)
        -> void
    {
        ck_iskm_cel_pattern_processor::DoSetCustomDepthOnProxySkmcs(InCurrent, false, 0);
        InHandle.Try_Remove<FFragment_IskmProxy_CelPatternApplied>();
    }
}

// --------------------------------------------------------------------------------------------------------------------
