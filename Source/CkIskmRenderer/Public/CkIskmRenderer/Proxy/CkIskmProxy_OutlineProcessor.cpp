#include "CkIskmProxy_OutlineProcessor.h"

#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkIskmRenderer/CkIskmRenderer_Log.h"

#include "CkUsf/Outline/CkUsf_OutlinePreset.h"
#include "CkUsf/Outline/CkUsf_OutlineSubsystem.h"

#include "Engine/World.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_IskmProxy_Outline_Sync);
CK_REGISTER_PROCESSOR(ck::FProcessor_IskmProxy_Outline_Remove);
CK_REGISTER_PROCESSOR(ck::FProcessor_IskmProxy_Outline_EndPlay);

// --------------------------------------------------------------------------------------------------------------------

namespace ck_iskm_outline_processor
{
    auto
        DoSetCustomDepthOnProxySkmcs(
            const ck::FFragment_IskmProxy_Current& InCurrent,
            bool InEnabled,
            uint8 InStencilValue)
        -> void
    {
        const auto& ApplyTo = [&](USkeletalMeshComponent* InSkmc)
        {
            if (ck::Is_NOT_Valid(InSkmc))
            { return; }

            InSkmc->SetRenderCustomDepth(InEnabled);
            InSkmc->SetCustomDepthStencilValue(static_cast<int32>(InStencilValue));
        };

        ApplyTo(InCurrent.Get_BaseSKMC().Get());

        for (const auto& Submesh : InCurrent.Get_SubmeshSKMCs())
        { ApplyTo(Submesh.Get()); }
    }

    auto
        DoReleaseStencil(
            const FCk_Handle& InHandle,
            const ck::FFragment_IskmProxy_OutlineApplied& InApplied)
        -> void
    {
        const auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);
        if (ck::Is_NOT_Valid(World))
        { return; }

        if (auto* OutlineSubsystem = World->GetSubsystem<UCkUsf_OutlineSubsystem>();
            ck::IsValid(OutlineSubsystem))
        {
            OutlineSubsystem->Release_StencilFor(InApplied.Get_Preset().Get());
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_IskmProxy_Outline_Sync::
        DoTick(
            TimeType InDeltaT)
        -> void
    {
        _World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(_TransientEntity);

        TProcessor::DoTick(InDeltaT);
    }

    auto
        FProcessor_IskmProxy_Outline_Sync::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Usf_OutlineTarget& InTarget,
            const FFragment_IskmProxy_Current& InCurrent) const
        -> void
    {
        using namespace ck_iskm_outline_processor;

        auto* Preset = InTarget.Get_Preset().Get();

        if (ck::Is_NOT_Valid(Preset))
        { return; }

        if (ck::Is_NOT_Valid(InCurrent.Get_BaseSKMC().Get()))
        { return; } // Setup incomplete or SKMC released — nothing to flag yet

        if (InHandle.Has<FFragment_IskmProxy_OutlineApplied>())
        {
            const auto& Applied = InHandle.Get<FFragment_IskmProxy_OutlineApplied>();

            if (Applied.Get_Preset().Get() == Preset)
            {
                // Re-assert every frame: the setters early-out when unchanged, and this is what makes
                // late-attached outfit submeshes (and mesh swaps) inherit the outline automatically.
                DoSetCustomDepthOnProxySkmcs(InCurrent, true, Applied.Get_StencilValue());
                return;
            }

            // Preset drift: undo, then fall through to re-apply.
            DoSetCustomDepthOnProxySkmcs(InCurrent, false, 0);
            DoReleaseStencil(InHandle, Applied);
            InHandle.Remove<FFragment_IskmProxy_OutlineApplied>();
        }

        if (ck::Is_NOT_Valid(_World.Get()))
        { return; }

        auto* OutlineSubsystem = _World->GetSubsystem<UCkUsf_OutlineSubsystem>();
        if (ck::Is_NOT_Valid(OutlineSubsystem))
        { return; }

        const auto Stencil = OutlineSubsystem->Get_OrAllocate_StencilFor(Preset);
        if (Stencil == 0)
        { return; } // stencil range exhausted — already warned by the subsystem

        DoSetCustomDepthOnProxySkmcs(InCurrent, true, Stencil);
        InHandle.AddOrGet<FFragment_IskmProxy_OutlineApplied>() =
            FFragment_IskmProxy_OutlineApplied{TStrongObjectPtr{Preset}, Stencil};

        ck::iskm::VeryVerbose(TEXT("Applied outline (custom depth) for ISKM Proxy [{}]"), InHandle);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_IskmProxy_Outline_Remove::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_IskmProxy_OutlineApplied& InApplied,
            const FFragment_IskmProxy_Current& InCurrent)
        -> void
    {
        using namespace ck_iskm_outline_processor;

        DoSetCustomDepthOnProxySkmcs(InCurrent, false, 0);
        DoReleaseStencil(InHandle, InApplied);
        InHandle.Remove<FFragment_IskmProxy_OutlineApplied>();
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_IskmProxy_Outline_EndPlay::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_IskmProxy_OutlineApplied& InApplied,
            const FFragment_IskmProxy_Current& InCurrent)
        -> void
    {
        using namespace ck_iskm_outline_processor;

        DoSetCustomDepthOnProxySkmcs(InCurrent, false, 0);
        DoReleaseStencil(InHandle, InApplied);
        InHandle.Remove<FFragment_IskmProxy_OutlineApplied>();
    }
}

// --------------------------------------------------------------------------------------------------------------------
