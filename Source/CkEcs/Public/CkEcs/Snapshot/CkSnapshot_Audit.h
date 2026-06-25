#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkThirdParty/entt-3.16.0/src/entt/core/type_info.hpp"

#include "Containers/Map.h"
#include "Containers/UnrealString.h"

// --------------------------------------------------------------------------------------------------------------------
// Snapshot classification audit — the runtime half of the holder/record forced-choice mechanism.
//
// The compile-time half (the required T_Policy on the holder/record base templates) makes an UNCLASSIFIED type a
// compile error. This audit covers the residual: a type CLASSIFIED ck::FSnapshotPolicy_RoundTrip whose .cpp
// CK_REGISTER_SNAPSHOTABLE was forgotten — which would silently drop it from every save/load.
//
// Each CK_DEFINE_*_ROUNDTRIP macro emits a static-init "announce" (below) recording the type as expected-registered.
// At capture time the snapshot verifies every announced type is actually in the fragment registry; any that is not
// is returned and CK_ENSURE'd by name. Because the snapshot AutoTests capture a world, this is a CI gate.
//
// The announce records only the entt type hash + a display name — NO snapshot Archive dependency — so it is safe to
// emit from the widely-included fragment headers (unlike CK_REGISTER_SNAPSHOTABLE itself, which must live in a .cpp).
// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class FCk_Snapshot_FragmentRegistry;
}

namespace ck::snapshot_audit
{
    // Records a RoundTrip holder/record type that is EXPECTED to be registered snapshotable. Idempotent by hash
    // (a header-emitted registrant runs once per including TU; duplicates collapse).
    CKECS_API auto Register_Expected(uint32 InEnttHash, const TCHAR* InDisplayName) -> void;

    // Pure comparator (testable, no globals): returns the display names of expected types absent from InRegistry.
    CKECS_API auto Audit_Compare(const TMap<uint32, FString>& InExpected, const FCk_Snapshot_FragmentRegistry& InRegistry) -> TArray<FString>;

    // Compares the process-wide announced set against the live fragment registry. Returns the offenders and, if any,
    // fires a CK_ENSURE naming them. Run once per process at capture time.
    CKECS_API auto Audit_ExpectedAreRegistered() -> TArray<FString>;
}

// --------------------------------------------------------------------------------------------------------------------

// Emitted by the CK_DEFINE_*_ROUNDTRIP macros immediately after the struct definition. _Type_ must be a complete type
// in scope (it is — the struct was just defined). The anonymous-namespace static runs Register_Expected at static-init.
#define CK_SNAPSHOT_ANNOUNCE_EXPECTED(_Type_)                                                            \
    namespace                                                                                            \
    {                                                                                                    \
        struct CK_CONCAT(FCk_SnapAnnounceExpected_, _Type_)                                              \
        {                                                                                                \
            CK_CONCAT(FCk_SnapAnnounceExpected_, _Type_)()                                               \
            {                                                                                            \
                ck::snapshot_audit::Register_Expected(entt::type_hash<_Type_>::value(), TEXT(#_Type_));  \
            }                                                                                            \
        };                                                                                               \
        static const CK_CONCAT(FCk_SnapAnnounceExpected_, _Type_) CK_CONCAT(_CkSnapAnnounceReg_, _Type_){}; \
    }

// --------------------------------------------------------------------------------------------------------------------
