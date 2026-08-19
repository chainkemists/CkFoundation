#include "CkEcs/Persistence/CkPersistenceHydration.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Format/CkFormat.h"        // ck::Format_UE — the loss record names the owner
#include "CkCore/Validation/CkIsValid.h"   // ck::IsValid on the payload's script struct

#include "UObject/UObjectGlobals.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_PendingHydrationPayloads_UE::
    Add(
        FInstancedStruct InEntry)
    -> void
{
    _Entries.Add(MoveTemp(InEntry));
}

auto
    UCk_PendingHydrationPayloads_UE::
    Get_Entries()
    -> TArray<FInstancedStruct>&
{
    return _Entries;
}

auto
    UCk_PendingHydrationPayloads_UE::
    Get_Entries() const
    -> const TArray<FInstancedStruct>&
{
    return _Entries;
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FFragment_PendingHydration::
        Enqueue(
            UObject* InOuter,
            FInstancedStruct InEntry)
        -> void
    {
        if (NOT _Payloads.IsValid())
        {
            auto* Holder = NewObject<UCk_PendingHydrationPayloads_UE>(InOuter);
            _Payloads = TStrongObjectPtr<UCk_PendingHydrationPayloads_UE>{Holder};
        }

        CK_ENSURE_IF_NOT(_Payloads.IsValid(),
            TEXT("Failed to allocate the GC-safe pending-hydration payload holder"))
        { return; }

        _Payloads->Add(MoveTemp(InEntry));
    }

    auto
        FFragment_PendingHydration::
        Get_Entries()
        -> TArray<FInstancedStruct>&
    {
        CK_ENSURE_IF_NOT(_Payloads.IsValid(),
            TEXT("Pending-hydration fragment has no payload holder"))
        {
            static auto Empty = TArray<FInstancedStruct>{};
            return Empty;
        }

        return _Payloads->Get_Entries();
    }

    auto
        FFragment_PendingHydration::
        Get_Entries() const
        -> const TArray<FInstancedStruct>&
    {
        CK_ENSURE_IF_NOT(_Payloads.IsValid(),
            TEXT("Pending-hydration fragment has no payload holder"))
        {
            static const auto Empty = TArray<FInstancedStruct>{};
            return Empty;
        }

        return _Payloads->Get_Entries();
    }
}

// --------------------------------------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        DoAbandon_PendingHydration(
            FCk_Handle& InHandle)
        -> void
    {
        if (NOT InHandle.Has<FFragment_PendingHydration>())
        { return; }

        auto& Pending = InHandle.Get<FFragment_PendingHydration>();

        // Mid-dispatch: the loop over these entries is still on the stack. Claiming them now would count the
        // entry currently being applied — which the returning apply counts by its real outcome — and would free
        // the array being iterated. The dispatcher runs this again when it is safe.
        if (Pending._IsDispatchInFlight)
        {
            Pending._AbandonRequested = true;
            return;
        }

        auto Registry = InHandle.Get_RegistryView();
        auto* Outcomes = Registry.TryGetContext<FCtx_HydrationOutcomes>();

        const auto& Entries = Pending.Get_Entries();

        if (Outcomes != nullptr)
        {
            Outcomes->_DestroyedWithEntries += Entries.Num();

            for (const auto& Entry : Entries)
            {
                Outcomes->Record_Loss(
                    ck::IsValid(Entry.GetScriptStruct()) ? Entry.GetScriptStruct()->GetName() : FString{TEXT("<invalid type>")},
                    ck::Format_UE(TEXT("{}"), InHandle),
                    FString{TEXT("destroyed-with-entries")});
            }
        }

        InHandle.Try_Remove<FTag_Hydration_PendingApply>();
        InHandle.Try_Remove<FFragment_PendingHydration>();
    }
}
