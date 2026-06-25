#include "CkSnapshot_Audit.h"

#include "CkSnapshot_FragmentRegistry.h"

#include "CkCore/Ensure/CkEnsure.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::snapshot_audit
{
    namespace
    {
        // Expected RoundTrip type hash -> display name. Populated at static-init (before any capture), deduped by hash.
        auto Get_Expected() -> TMap<uint32, FString>&
        {
            static TMap<uint32, FString> Instance;
            return Instance;
        }
    }

    auto
        Register_Expected(
            uint32 InEnttHash,
            const TCHAR* InDisplayName)
        -> void
    {
        Get_Expected().Add(InEnttHash, FString{InDisplayName});
    }

    auto
        Audit_Compare(
            const TMap<uint32, FString>& InExpected,
            const FCk_Snapshot_FragmentRegistry& InRegistry)
        -> TArray<FString>
    {
        auto Offenders = TArray<FString>{};

        for (const auto& Pair : InExpected)
        {
            if (InRegistry.Find_ByEnttHash(Pair.Key) == nullptr)
            { Offenders.Emplace(Pair.Value); }
        }

        Offenders.Sort();
        return Offenders;
    }

    auto
        Audit_ExpectedAreRegistered()
        -> TArray<FString>
    {
        const auto Offenders = Audit_Compare(Get_Expected(), FCk_Snapshot_FragmentRegistry::Get());

        CK_ENSURE_IF_NOT(Offenders.IsEmpty(),
            TEXT("Snapshot audit: [{}] RoundTrip holder/record type(s) are classified RoundTrip but are NOT "
                "registered snapshotable — they would be silently dropped on save/load. Add a "
                "CK_REGISTER_SNAPSHOTABLE in each type's .cpp, or reclassify them _TRANSIENT. Offenders: [{}]"),
            Offenders.Num(), FString::Join(Offenders, TEXT(", ")))
        {}

        return Offenders;
    }
}

// --------------------------------------------------------------------------------------------------------------------
