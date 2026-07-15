#include "CkEcs/Persistence/CkPersistenceHydration.h"

#include "CkCore/Ensure/CkEnsure.h"

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
