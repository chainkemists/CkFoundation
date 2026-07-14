#include "CkPersistence_ReDrive_Processor.h"

#include "CkCore/Ensure/CkEnsure.h"

#include "CkEcs/Scheduler/CkProcessorRegistration.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_Persistence_ReDriveOnRestore);

namespace ck_persistence_redrive
{
    // Past this, a payload type whose SeedContainer keeps returning NotAdded is dropped LOUDLY — its owner/driver
    // precondition was never met. Mirrors the net dispatcher's pending-apply timeout.
#if UE_BUILD_SHIPPING
    constexpr auto TimeoutSeconds = 2.0f;
#else
    constexpr auto TimeoutSeconds = 5.0f;
#endif
}

namespace ck
{
    auto
        FProcessor_Persistence_ReDriveOnRestore::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle) const
        -> void
    {
        // Populate the remaining-set on first sight. The fragment then persists (emptied) as the done-marker —
        // FTag_Snapshot_JustRestored never clears, so we cannot key completion on the fragment's removal.
        if (ck::Is_NOT_Valid(InHandle))
        { return; }

        if (NOT InHandle.Has<FFragment_Persistence_ReDrivePending>())
        {
            auto& NewPending = InHandle.Add<FFragment_Persistence_ReDrivePending>();
            for (const auto* Type : FCk_ReplicatedFragmentHandlerRegistry::Get_ReDriveHandlerTypes())
            { NewPending._Remaining.Emplace(Type); }
        }

        auto& Pending = InHandle.Get<FFragment_Persistence_ReDrivePending>();
        if (Pending._Remaining.IsEmpty())
        { return; } // already fully re-driven — the emptied fragment is the done-marker

        Pending._PendingForSeconds += InDeltaT.Get_Seconds();

        auto StillRemaining = TArray<TWeakObjectPtr<const UScriptStruct>>{};

        for (const auto& TypePtr : Pending._Remaining)
        {
            const auto* Type = TypePtr.Get();
            if (Type == nullptr)
            { continue; } // type GC'd — drop

            const auto* Handler = FCk_ReplicatedFragmentHandlerRegistry::Resolve(Type);
            if (Handler == nullptr || NOT Handler->Produce || NOT Handler->SeedContainer)
            { continue; } // registry churn — drop (only participating handlers were ever added)

            auto Payload = Handler->Produce(InHandle);
            if (NOT Payload.IsSet())
            { continue; } // feature absent on this entity — drop (a SET-but-empty payload still seeds)

            const auto AddedOrNot = Handler->SeedContainer(InHandle, Payload.GetValue());
            if (AddedOrNot == ECk_AddedOrNot::NotAdded)
            { StillRemaining.Emplace(TypePtr); } // precondition unmet this tick — retry
            // Added / AlreadyExists => the ContainerRef exists, the Replicate processor pushes from here => drop
        }

        // Timeout: past the window, drop the stragglers LOUDLY so a never-met precondition is not silent.
        CK_ENSURE_IF_NOT(StillRemaining.IsEmpty() || Pending._PendingForSeconds < ck_persistence_redrive::TimeoutSeconds,
            TEXT("Persistence restore re-drive timed out for Entity [{}]: [{}] payload type(s) never seeded "
                 "(owner/driver precondition unmet)"), InHandle, StillRemaining.Num())
        {
            StillRemaining.Reset();
        }

        Pending._Remaining = MoveTemp(StillRemaining);
    }
}

// --------------------------------------------------------------------------------------------------------------------
