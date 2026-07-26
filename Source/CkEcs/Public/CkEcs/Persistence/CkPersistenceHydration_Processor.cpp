#include "CkEcs/Persistence/CkPersistenceHydration_Processor.h"

#include "CkCore/Ensure/CkEnsure.h"

#include "CkEcs/Persistence/CkPersistenceHandlerRegistry.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_Hydration_Dispatch);

namespace ck::persistence_apply
{
    auto
        ApplyOne(
            FCk_Handle& InEntity,
            const FInstancedStruct& InData,
            const TOptional<FInstancedStruct>& InOldData,
            float& InOutPendingForSeconds,
            FCk_Time InDeltaT)
        -> EApplyOutcome
    {
        const auto* Handler = FCk_PersistenceHandlerRegistry::Resolve(InData.GetScriptStruct());
        if (Handler == nullptr || NOT Handler->HydrationApply)
        {
            // Registry churn, or a handler that never declared its load path. Drop rather than retry forever.
            return EApplyOutcome::DroppedTimeout;
        }

        const auto ApplyResult = Handler->HydrationApply(InEntity, InData, InOldData);
        if (ApplyResult == ECk_Persistence_ApplyResult::Applied)
        { return EApplyOutcome::Applied; }

        if (ApplyResult == ECk_Persistence_ApplyResult::Rejected)
        { return EApplyOutcome::DroppedRejected; }

        InOutPendingForSeconds += InDeltaT.Get_Seconds();

        if (InOutPendingForSeconds >= ck::PendingApplyTimeoutSeconds)
        {
            const auto TypeName = ck::IsValid(InData.GetScriptStruct())
                ? InData.GetScriptStruct()->GetName()
                : FString{TEXT("<invalid type>")};

            CK_TRIGGER_ENSURE(
                TEXT("Hydration payload [{}] on entity [{}] was never applied: Apply kept returning NotReady for "
                     "[{}]s — the feature it targets was never composed. Dropping the entry."),
                TypeName, InEntity, InOutPendingForSeconds);

            return EApplyOutcome::DroppedTimeout;
        }

        return EApplyOutcome::StillPending;
    }
}

namespace ck
{
    auto
        FProcessor_Hydration_Dispatch::
        DoTick(
            TimeType InDeltaT)
        -> void
    {
        // Runs LAST in FGroup_DeferredApply, so clearing FTag_EntityScript_ConstructedThisFrame AFTER the loop is
        // safe: both dispatchers have already skipped those entities this pass, and the pump re-dispatches them.
        TProcessor::DoTick(InDeltaT);
        _TransientEntity.Clear<FTag_EntityScript_ConstructedThisFrame>();
    }

    auto
        FProcessor_Hydration_Dispatch::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_PendingHydration& InPending) const
        -> void
    {
        // Defer entities composed this frame — same rationale as the net dispatcher.
        if (InHandle.Has<FTag_EntityScript_ConstructedThisFrame>())
        { return; }

        auto AnyStillPending = false;
        auto& Entries = InPending.Get_Entries();

        // Iterate back-to-front so applied/dropped entries can be removed in place.
        for (auto Index = Entries.Num() - 1; Index >= 0; --Index)
        {
            // No per-entry coalescing on the load path (unlike the net FastArray) — the Old side is always unset.
            const auto Outcome = ck::persistence_apply::ApplyOne(
                InHandle, Entries[Index], TOptional<FInstancedStruct>{}, InPending._PendingForSeconds, InDeltaT);

            if (Outcome == ck::persistence_apply::EApplyOutcome::StillPending)
            { AnyStillPending = true; }
            else
            { Entries.RemoveAt(Index); } // Applied or terminally dropped
        }

        if (NOT AnyStillPending)
        { InPending._PendingForSeconds = 0.0f; }

        if (Entries.IsEmpty())
        {
            InHandle.Remove<FTag_Hydration_PendingApply>();
            InHandle.Remove<FFragment_PendingHydration>();
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------
